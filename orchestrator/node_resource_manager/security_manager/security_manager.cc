#include "security_manager.h"

static const char LOG_MODULE_NAME[] = "Security-Manager";

SecurityManager::SecurityManager(SQLiteManager *dbm) : dbmanager(dbm) { }

SecurityManager::~SecurityManager() {
	// I do not pretend to be owner of these data, so I will leave deletion to others...
	dbmanager = NULL;
}

bool SecurityManager::isAuthenticated(struct MHD_Connection *connection, char *token) {
	int rc = 0, res = 0, idx = 0, count = 0;
	char *sql = "select count(*) from LOGIN where TOKEN = @token;";

	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(dbmanager->getDb(), sql, -1, &stmt, 0);

	if (rc == SQLITE_OK) {
		idx = sqlite3_bind_parameter_index(stmt, "@token");
		sqlite3_bind_text(stmt, idx, token, strlen(token), 0);

		res = sqlite3_step(stmt);

		if (res == SQLITE_ROW)
			count = sqlite3_column_int(stmt, 0);
	}

	return (count > 0);
}

/**
 *	Given a generic resource, permissions are checked for each single resource mapped to the generic one.
 *	Since aggregated operations are not currently supported, authorization is given only if the user
 *	is authorized for all the single resources.
 */
bool SecurityManager::isAuthorized(user_info_t *usr, opcode_t operation, const char *generic_resource) {
	int rc = 0, res = 0, idx = 0;
	const char *sql = "select * from CURRENT_RESOURCES_PERMISSIONS "	\
							"where GENERIC_RESOURCE = @generic_resource;";
	char *owner = NULL, *owner_perm = NULL, *group_perm = NULL, *all_perm = NULL, *admin_perm = NULL, *permissions = NULL;

	sqlite3_stmt *stmt;
	bool result = true;

	rc = sqlite3_prepare_v2(dbmanager->getDb(), sql, -1, &stmt, 0);

	if (rc == SQLITE_OK) {
		idx = sqlite3_bind_parameter_index(stmt, "@generic_resource");
		sqlite3_bind_text(stmt, idx, generic_resource, strlen(generic_resource), 0);

		do {
			res = sqlite3_step(stmt);

			if (res == SQLITE_ROW) {
				owner = (char *) sqlite3_column_text(stmt, 2);
				owner_perm = (char *) sqlite3_column_text(stmt, 3);
				group_perm = (char *) sqlite3_column_text(stmt, 4);
				all_perm = (char *) sqlite3_column_text(stmt, 5);
				admin_perm = (char *) sqlite3_column_text(stmt, 6);

				if(strcmp(usr->user, ADMIN) == 0)
					permissions = admin_perm;
				else if(strcmp(usr->user, owner) == 0)
					permissions = owner_perm;
				else {
					char *owner_group = dbmanager->getGroup(owner);

					if(strcmp(usr->group, owner_group) == 0)
						permissions = group_perm;
					else
						permissions = all_perm;
				}

				if(strncmp(permissions + operation, DENY, 1) == 0) {
					sqlite3_finalize(stmt);
					return false;
				}
			}
		} while(res != SQLITE_DONE && res != SQLITE_ERROR);
	}

	sqlite3_finalize(stmt);

	return result;
}

bool SecurityManager::isAuthorizedForCreation(char *user, const char *generic_resource, const char *resource) {
	assert(user != NULL && generic_resource != NULL && resource != NULL);

	int rc = 0, res = 0, idx = 0;
	char *sql = NULL, *permissions = NULL;
	sqlite3_stmt *stmt;

	// The resource I want to create must not exist in the database at the moment
	/* This check prevent the update of an existing graph!
	if(dbmanager->resourceExists(generic_resource, resource)) {
		ULOG_ERR("Cannot create new resource /%s/%s: it does already exist!", generic_resource, resource);
		return false;
	}
	*/
	sql = "select PERMISSION from USER_CREATION_PERMISSIONS "	\
			"where USER = @user and GENERIC_RESOURCE = @generic_resource;";

	rc = sqlite3_prepare_v2(dbmanager->getDb(), sql, -1, &stmt, 0);

	if (rc == SQLITE_OK) {
		idx = sqlite3_bind_parameter_index(stmt, "@user");
		sqlite3_bind_text(stmt, idx, user, strlen(user), 0);

		idx = sqlite3_bind_parameter_index(stmt, "@generic_resource");
		sqlite3_bind_text(stmt, idx, generic_resource, strlen(generic_resource), 0);

		res = sqlite3_step(stmt);

		if (res == SQLITE_ROW)
			permissions = (char *) sqlite3_column_text(stmt, 0);
	}
	res = (permissions != NULL && strncmp(permissions, ALLOW, 1) == 0);
	sqlite3_finalize(stmt);

	return res;
}

bool SecurityManager::isAuthorized(user_info_t *usr, opcode_t operation, const char *generic_resource, const char *resource) {
	assert(usr != NULL && usr->user != NULL && usr->pwd != NULL && usr->group != NULL && usr->token != NULL);

	int rc = 0, res = 0, idx = 0;
	char *sql = NULL, *owner = NULL, *owner_perm = NULL, *group_perm = NULL, *all_perm = NULL, *admin_perm = NULL, *permissions = NULL;

	sqlite3_stmt *stmt;
	bool result = true;

	if(operation == _CREATE) {
		ULOG_INFO("Received creation request for resource /%s/%s", generic_resource, resource);
		return isAuthorizedForCreation(usr->user, generic_resource, resource);
	}

	sql = "select * from CURRENT_RESOURCE_PERMISSIONS "	\
								"where GENERIC_RESOURCE = @generic_resource and RESOURCE = @resource;";

	rc = sqlite3_prepare_v2(dbmanager->getDb(), sql, -1, &stmt, 0);

	if (rc == SQLITE_OK) {
		idx = sqlite3_bind_parameter_index(stmt, "@generic_resource");
		sqlite3_bind_text(stmt, idx, generic_resource, strlen(generic_resource), 0);

		idx = sqlite3_bind_parameter_index(stmt, "@resource");
		sqlite3_bind_text(stmt, idx, resource, strlen(resource), 0);

		res = sqlite3_step(stmt);

		if (res == SQLITE_ROW) {
			owner = (char *) sqlite3_column_text(stmt, 2);
			owner_perm = (char *) sqlite3_column_text(stmt, 3);
			group_perm = (char *) sqlite3_column_text(stmt, 4);
			all_perm = (char *) sqlite3_column_text(stmt, 5);
			admin_perm = (char *) sqlite3_column_text(stmt, 6);

			if(strcmp(usr->user, ADMIN) == 0)
				permissions = admin_perm;
			else if(strcmp(usr->user, owner) == 0)
				permissions = owner_perm;
			else {
				char *owner_group = dbmanager->getGroup(owner);

				if(strcmp(usr->group, owner_group) == 0)
					permissions = group_perm;
				else
					permissions = all_perm;
			}

			result = (strncmp(permissions + operation, ALLOW, 1) == 0);
		}
	}

	sqlite3_finalize(stmt);

	return result;
}
