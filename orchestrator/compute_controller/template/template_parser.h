#include "template.h"
#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>


using namespace json_spirit;
using namespace std;

class Template_Parser{

public:
    static bool parse(Template& temp,string data);
   /* bool getNameFromJSON(string& name);
    bool getExpandableFromJSON(bool& expandable);
    bool getUriFromJSON(string& u);*/
    static bool parsePort(Template& temp,Object port /*oggetto porta da parsare*/);
    static bool parseCoreNumbers(Template& temp,Object CPUrequirements);
};