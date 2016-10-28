#include "graph_parser_utils.h"

string GraphParserUtils::graphID(string name_port)
{
	return nfId(name_port);
}

string GraphParserUtils::nfId(string id_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,id_port.c_str());
	char *pnt=strtok(tmp, delimiter);
	while( pnt!= NULL )
	{
		return string(pnt);
	}

	return "";
}

unsigned int GraphParserUtils::nfPort(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);
	unsigned int port = 0;

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				sscanf(pnt,"%u",&port);
				return port;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return port;
}

bool GraphParserUtils::nfIsPort(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);
	unsigned int port = 0;

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				sscanf(pnt,"%u",&port);
				return true;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return false;
}

string GraphParserUtils::epName(string name_port)
{
	char delimiter[] = ":";
	char tmp[BUFFER_SIZE];
	strcpy(tmp,name_port.c_str());
	char *pnt=strtok((char*)/*name_port.c_str()*/tmp, delimiter);

	int i = 0;
	while( pnt!= NULL )
	{
		switch(i)
		{
			case 1:
				return pnt;
			break;
		}

		pnt = strtok( NULL, delimiter );
		i++;
	}

	return "";
}

unsigned int GraphParserUtils::epPort(string name_port)
{
	return nfPort(name_port);
}
