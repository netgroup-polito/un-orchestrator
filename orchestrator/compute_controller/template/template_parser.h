#include "template.h"
#include <string>
#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>


using namespace json_spirit;
using namespace std;

class Template_Parser{

public:
    static bool setTemplateFromJson(Template & temp,Object obj);
    static bool parse(std::list<Template>& templates,string data,bool checkVnfTemplate);
    static bool parsePort(Template& temp,Object port);
    static bool parseCoreNumbers(Template& temp,Object CPUrequirements);
};