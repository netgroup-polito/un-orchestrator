drop table testtable cascade;
create table testtable ( lm float, lsd float);
insert into testtable VALUES (ln(2), ln(2));
insert into testtable VALUES (ln(4), ln(2));
insert into testtable VALUES (ln(6), ln(2));
select * from testtable;



create extension plpythonu WITH SCHEMA pg_catalog ;

CREATE FUNCTION pylogrisk (zmu float, zsd float, linerate integer, cutoff float)
  RETURNS float
AS $$
from math import exp,pow,log,sqrt
from scipy.stats import lognorm
return lognorm.sf(linerate * cutoff, zsd, 0, exp(zmu)) * 100
$$ LANGUAGE plpythonu;



DROP FUNCTION sumfunc(float[],float[]) CASCADE;
DROP FUNCTION ffunc(float[]) CASCADE;
DROP FUNCTION dfunc(float[]) CASCADE;
CREATE FUNCTION sumfunc(acc float[], next float[]) RETURNS float[] AS 
'select ARRAY[acc[1] + exp(2*next[1]+next[2]^2)*(exp(next[2]^2-1)),
        acc[2] + exp(next[1]+(next[2]^2)/2)];'  
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

CREATE FUNCTION ffunc(float[]) RETURNS float[]
 AS 'select ARRAY[ln($1[2]) - ln($1[1] / ($1[2]^2)  +1)/2, sqrt(ln($1[1] / $1[2]^2   +1))];'  
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

CREATE FUNCTION ffuncrisk(float[]) RETURNS float
 AS 'select pylogrisk(ln($1[2]) - ln($1[1] / ($1[2]^2)  +1)/2, sqrt(ln($1[1] / $1[2]^2)   +1),100,0.95);'  
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

CREATE AGGREGATE lognormagg (float[]) (
sfunc = sumfunc, 
stype = float[],
finalfunc = ffunc,
initcond = '{0.0,0.0}'
);

CREATE AGGREGATE lognormrisk (float[]) (
sfunc = sumfunc, 
stype = float[],
finalfunc = ffuncrisk,
initcond = '{0.0,0.0}'
);




ACTIVATE;

CREATE CONTINUOUS VIEW view_mf1 WITH (max_age = '1 minute' ) AS SELECT AVG(lm::float) as lma,AVG(lsd::float) as lsda,count(*) FROM stream_mf1;

CREATE CONTINUOUS VIEW view_mf2 WITH (max_age = '1 minute' ) AS SELECT AVG(lm::float) as lma,AVG(lsd::float) as lsda,count(*) FROM stream_mf2;

CREATE CONTINUOUS VIEW view_mf3 WITH (max_age = '1 minute' ) AS SELECT AVG(lm::float) as lma,AVG(lsd::float) as lsda,count(*) FROM stream_mf3;

CREATE CONTINUOUS VIEW view_mf4 WITH (max_age = '1 minute' ) AS SELECT AVG(lm::float) as lma,AVG(lsd::float) as lsda,count(*) FROM stream_mf4;

create view mf14 as select * from view_mf1 union select * from view_mf2 union select * from view_mf3 union select * from view_mf4;

select lognormagg(ARRAY[lma,lsda])  from mf14;

CREATE AGGREGATE lognormrisk (float[]) ;

