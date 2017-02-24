#ifndef PROJECT_CONFIGURATIONAGENT_PARSEREXCEPTION_H
#define PROJECT_CONFIGURATIONAGENT_PARSEREXCEPTION_H

#pragma once
#include <stdio.h>
#include <inttypes.h>
#include <string>
#include <cstring>
#include <exception>
using namespace std;
class NodeConfigurationParserException: public std::exception
{
private:
    string error;
public:
    NodeConfigurationParserException(string error):error(error){};
    NodeConfigurationParserException(){};

    virtual const char* what() const throw()
    {
        return error.c_str();
    }

    void setError(string error)
    {
        this->error=error;
    }

    virtual const char* getModule()
    {
        return "ConfigurationAgent-Parser";
    }

};


#endif //PROJECT_CONFIGURATIONAGENT_PARSEREXCEPTION_H
