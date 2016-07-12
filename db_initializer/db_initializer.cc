#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

#include <string.h>

#include "../orchestrator/node_resource_manager/database_manager/SQLite/SQLiteManager.h"
#include "../orchestrator/node_resource_manager/database_manager/SQLite/INIReader.h"

#include "../orchestrator/utils/constants.h"
#include "../orchestrator/utils/logger.h"

bool initDB(SQLiteManager *dbm, char *pass)
{
	unsigned char *hash_token = new unsigned char[HASH_SIZE];
	char *hash_pwd = new char[BUFFER_SIZE];
	char *tmp = new char[HASH_SIZE];
	char *pwd = new char[HASH_SIZE];

	if(dbm->createTables()){
		strcpy(pwd, pass);

		SHA256((const unsigned char*)pwd, strlen(pwd), hash_token);

		for (int i = 0; i < HASH_SIZE; i++) {
			sprintf(tmp, "%.2x", hash_token[i]);
			strcat(hash_pwd, tmp);
	    }

		// insert generic resources
		dbm->insertResource(BASE_URL_GRAPH);
		dbm->insertResource(BASE_URL_USER);
		dbm->insertResource(BASE_URL_GROUP);

		// default permissions for NF-FGs
		dbm->insertDefaultUsagePermissions(BASE_URL_GRAPH,
				DEFAULT_NFFG_OWNER_PERMISSION,
				DEFAULT_NFFG_GROUP_PERMISSION,
				DEFAULT_NFFG_ALL_PERMISSION,
				DEFAULT_NFFG_ADMIN_PERMISSION);

		// default permissions for USERs
		dbm->insertDefaultUsagePermissions(BASE_URL_USER,
				DEFAULT_NFFG_OWNER_PERMISSION,
				DEFAULT_NFFG_GROUP_PERMISSION,
				DEFAULT_NFFG_ALL_PERMISSION,
				DEFAULT_NFFG_ADMIN_PERMISSION);

		// default permissions for GROUPs
		dbm->insertDefaultUsagePermissions(BASE_URL_GROUP,
				DEFAULT_NFFG_OWNER_PERMISSION,
				DEFAULT_NFFG_GROUP_PERMISSION,
				DEFAULT_NFFG_ALL_PERMISSION,
				DEFAULT_NFFG_ADMIN_PERMISSION);

		// default creation permissions for admin user
		dbm->insertUserCreationPermission(ADMIN, BASE_URL_GRAPH, ALLOW);
		dbm->insertUserCreationPermission(ADMIN, BASE_URL_USER, ALLOW);
		dbm->insertUserCreationPermission(ADMIN, BASE_URL_GROUP, ALLOW);

		// default users
		dbm->insertResource(BASE_URL_GROUP, ADMIN, ADMIN);
		dbm->insertResource(BASE_URL_USER, ADMIN, ADMIN);
		dbm->insertUser(ADMIN, hash_pwd, ADMIN);

		return true;
	}

	return false;
}

int main(int argc, char *argv[]) {

	SQLiteManager *dbm = NULL;
	char db_name[BUFFER_SIZE];
	char *pwd = NULL;

	// Check for root privileges
	if(geteuid() != 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Root permissions are required to run %s\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	// Check for arguments
	if(argc < 2) {
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Initialize local database and set the password for the default 'admin' user.");
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Usage: sudo ./db_initializer admin_password\n");
		exit(EXIT_FAILURE);
	}

	pwd = argv[1];
	sprintf(db_name, "../orchestrator/%s", DB_NAME);

	dbm = new SQLiteManager(db_name);

	if(!initDB(dbm, pwd)) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Database already initialized.");
		exit(EXIT_FAILURE);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Database initialized successfully.");
}
