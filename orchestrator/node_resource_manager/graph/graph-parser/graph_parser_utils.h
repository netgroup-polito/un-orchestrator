#ifndef GRAPH_PARSER_UTILS_H_
#define GRAPH_PARSER_UTILS_H_ 1

#pragma once

#include <exception>
using namespace std;

typedef struct
{
	string local_ip;
	string remote_ip;
	string key;
	string ttl;
	bool safe=false;
} gre_info_t;

class GraphParserException: public std::exception
{
private:
	string error;
public:
	GraphParserException(string error):error(error){};
	GraphParserException(){};

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
		return "Graph-Parser";
	}

};

class MatchParserException: public GraphParserException
{
public:

	MatchParserException(string err):GraphParserException(err)
	{
	}

	MatchParserException():GraphParserException()
	{		
	}
	
	const char* getModule()
	{
		return "Match-Parser";
	}

};

#endif //GRAPH_PARSER_UTILS_H_
