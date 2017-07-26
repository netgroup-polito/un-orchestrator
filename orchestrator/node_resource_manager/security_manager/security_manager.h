#ifndef SECURITY_MANAGER_H_
#define SECURITY_MANAGER_H_ 1

/**
*	Class that implements operations related to security for the REST server. The capabilities currently provided are:
*
*		- Authentication
*		- Authorization
*/


#pragma once

#define __STDC_FORMAT_MACROS

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sstream>

#include <sqlite3.h>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include "../../utils/constants.h"

#include "../database_manager/SQLite/SQLiteManager.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include <sstream>
#include <fstream>


class SecurityManager
{
private:
	SQLiteManager *dbmanager;

	bool isAuthorizedForCreation(char *user, const char *generic_resource, const char *resource);

public:
	SecurityManager(SQLiteManager *);
	~SecurityManager();

	int login(struct MHD_Connection *connection, void **con_cls);

	bool isAuthenticated(struct MHD_Connection *connection, char *token);

	bool isAuthorized(user_info_t *usr, opcode_t operation, const char *generic_resource);
	bool isAuthorized(user_info_t *usr, opcode_t operation, const char *generic_resource, const char *resource);
};

#endif
