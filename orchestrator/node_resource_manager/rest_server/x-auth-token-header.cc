#include "x-auth-token-header.h"

void XAuthToken::parse(const std::string& data)
{
	token_ = data;
}

void XAuthToken::write(std::ostream& os) const
{
	os << token_;
}