#ifndef SQLiteManager_H_
#define SQLiteManager_H_ 1

#pragma once

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <sqlite3.h>
#include <list>
#include <string.h>
#include <ctime>

#include "../../../utils/logger.h"
#include "../../../utils/constants.h"

#include <assert.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/writer_template.h>
#include <json_spirit/writer.h>
#include <json_spirit/reader_template.h>
#include <json_spirit/reader.h>
#include <fstream>

using namespace std;
using namespace json_spirit;

typedef enum {
	_READ,
	_UPDATE,
	_DELETE,
	_CREATE
} opcode_t;

typedef struct {
	char *user, *pwd, *group, *token;
} user_info_t;

class SQLiteManager {

private:
	/**
	*	@brief: Database pointer
	*/
	static sqlite3 *db;

	/**
	*	@brief: Name of database
	*/
	static char *db_name;

	/**
	*	@brief:	Open a connection with given database db_name
	*/
	int connect(char *db_name);

	/**
	*	@brief: Disconnect from given database
	*/
	void disconnect();

public:

	SQLiteManager(char *db_name);

	~SQLiteManager();

	sqlite3* getDb();

	bool createTables();

	bool cleanTables();

	bool userExists(char *username, char *hash_pwd);
	int insertUser(char *user, char *pwd, char *group);
	int deleteUser(char *user);
	user_info_t *getUserByToken(const char *token);

	user_info_t *getUserByName(const char *username);
	user_info_t *getLoggedUserByName(const char *username);
	
	char *getGroup(const char *user);

	bool isLogged(char *username);

	int deleteGroup(char *group);

	bool usersExistForGroup(const char *group);

	int deleteUser(struct MHD_Connection *connection, char *username);

	int insertLogin(char *user, char *token, char *timestamp);

	bool resourceExists(const char *generic_resource);
	bool resourceExists(const char *generic_resource, const char *resource);

	int insertResource(char *generic_resource);
	int insertResource(char *generic_resource, const char *resource, char *owner);
	int updateResource(char *generic_resource, char *resource);
	int deleteResource(char *generic_resource, char *resource);

	bool isGenericResource(const char *generic_resource);

	int insertDefaultUsagePermissions(char *generic_resource, char *owner_p, char *group_p, char *all_p, char *admin_p);

	int insertUserCreationPermission(char *user, char *generic_resource, char *permission);

	void getAllowedResourcesNames(user_info_t *usr, opcode_t op, char *generic_resource, std::list<std::string> *resources);
	void getAllResourcesNames(char *generic_resource, std::list<std::string> *resources);
};

class SQLiteManagerException : public exception
{
public:
	virtual const char* what() const throw()
	{
		return "SQLiteManagerException";
	}
};

#endif
