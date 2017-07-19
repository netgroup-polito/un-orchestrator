#ifndef X_AUTH_TOKEN_H_
#define X_AUTH_TOKEN_H_ 1

#pragma once

#include <pistache/http_headers.h>
#include <pistache/net.h>

using namespace Pistache;
using namespace Pistache::Http;
using namespace std;

class XAuthToken : public Header::Header 
{
public:
	NAME("XAuth-Token");
	
	XAuthToken()
		: token_("")
	{}
	
	explicit XAuthToken(string token)
		: token_(token)
	{}
	
	void parse(const string& data);
	void write(ostream& os) const;
	
	string token() const { return token_; }
	
private:
	string token_;
};

#endif //X_AUTH_TOKEN_H_