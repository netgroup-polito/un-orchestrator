#ifndef PROJECT_CREATENAMESPACEPPPOE_H
#define PROJECT_CREATENAMESPACEPPPOE_H

#pragma once

#include <string>
#include <list>

using namespace std;

class CreateNamespacePPPoE {
private:
    string namespace_name;
    string pppPort; //is the ppp port
public:

    CreateNamespacePPPoE(string namespace_name, string pppPort): namespace_name(namespace_name), pppPort(pppPort)
    {
        //internal_if_to_ppp = namespace_name + ".i";
        //external_if = namespace_name + ".e";
    }

    ~CreateNamespacePPPoE();

    string getNamespaceName(){
      return namespace_name;
    };

    string getPPPport(){
        return pppPort;
    };

    /*string getExternalIf(){
        return external_if;
    };*/

};


#endif //PROJECT_CREATENAMESPACEPPPOE_H
