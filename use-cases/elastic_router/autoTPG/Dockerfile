FROM      ubuntu
MAINTAINER iMinds

RUN apt-get update
RUN apt-get install -y build-essential default-jdk ant python-dev wget unzip
RUN wget http://users.intec.ugent.be/unify/autoTPG-CONTROLLER.zip

RUN unzip autoTPG-CONTROLLER.zip
WORKDIR floodlight-plus/
RUN make

CMD ./floodlight.sh 


