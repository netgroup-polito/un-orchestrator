#ifndef PROJECT_CREATENAMESPACEPPPOE_H
#define PROJECT_CREATENAMESPACEPPPOE_H

#pragma once

#include <string>
#include <list>

using namespace std;

class CreateNamespacePPPoE {
private:
    string namespace_name;
    string internal_if_to_switch;
    //list<string,string> internal_ifs_to_switch;
    string internal_if_to_ppp;
    string external_if;
public:

    CreateNamespacePPPoE(/*string internal_if_to_switch,*/ string namespace_name): namespace_name(namespace_name)//internal_if_to_switch(internal_if_to_switch)
    {
        internal_if_to_ppp = namespace_name + ".i";
        external_if = namespace_name + ".e";
    }

    ~CreateNamespacePPPoE();

    string getNamespaceName(){
      return namespace_name;
    };

    /*void setInternalIfsToSwitch(){

    };*/

    /*list<string,string> getInternalIfsToSwitch(){
        return internal_ifs_to_switch;
    };*/

    string getInternalIfToSwitch(){
        return internal_if_to_switch;
    };

    string getInernalIfToPPP(){
        return internal_if_to_ppp;
    };

    string getExternalIf(){
        return external_if;
    };

};


#endif //PROJECT_CREATENAMESPACEPPPOE_H
