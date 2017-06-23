#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

#include <string.h>

#include "../orchestrator/node_resource_manager/database_manager/SQLite/SQLiteManager.h"
#include <INIReader.h>

#include "../orchestrator/utils/constants.h"
#include "../orchestrator/utils/logger.h"

char* hash_password(char *plain_password){
	unsigned char *hash_token = new unsigned char[HASH_SIZE];
	char *hash_pwd = new char[BUFFER_SIZE];
	char *tmp = new char[HASH_SIZE];
	char *pwd = new char[HASH_SIZE];

	strcpy(pwd, plain_password);
	SHA256((const unsigned char*)pwd, strlen(pwd), hash_token);

	for (int i = 0; i < HASH_SIZE; i++) {
		sprintf(tmp, "%.2x", hash_token[i]);
		strcat(hash_pwd, tmp);
	}
	return hash_pwd;
}

bool initDB(SQLiteManager *dbm, char *pass)
{

	if(dbm->createTables()){

		char *hash_pwd = hash_password(pass);

		// insert generic resources
		dbm->insertResource(BASE_URL_GRAPH);
		dbm->insertResource(BASE_URL_USER);
		dbm->insertResource(BASE_URL_GROUP);
		dbm->insertResource(URL_CONFIGURATION);

		// default permissions for NF-FGs
		dbm->insertDefaultUsagePermissions(BASE_URL_GRAPH,
				DEFAULT_NFFG_OWNER_PERMISSION,
				DEFAULT_NFFG_GROUP_PERMISSION,
				DEFAULT_NFFG_ALL_PERMISSION,
				DEFAULT_NFFG_ADMIN_PERMISSION);

		// default permissions for Configuration Parameters
		dbm->insertDefaultUsagePermissions(URL_CONFIGURATION,
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
		dbm->insertUserCreationPermission(ADMIN, URL_CONFIGURATION, ALLOW);

		// default users
		dbm->insertResource(BASE_URL_GROUP, ADMIN, ADMIN);
		dbm->insertResource(BASE_URL_USER, ADMIN, ADMIN);
		dbm->insertUser(ADMIN, hash_pwd, ADMIN);

		return true;
	}

	return false;
}

void handle_init(SQLiteManager *dbm){

    char password[20];
    printf("Insert a password for the default 'admin' user: ");
    scanf("%s", password);
    getchar();

	if(!initDB(dbm, password)) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Database already initialized.");
		exit(EXIT_FAILURE);
	}

	logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "Database initialized successfully.");
    getchar();
}

void handle_add_user(SQLiteManager *dbm){

	char username[20];
	char password[20];
	char *hash_pwd = NULL;

	printf("Username: ");
	scanf("%s", username);
	getchar();
	printf("Password: ");
	scanf("%s", password);
    getchar();

	hash_pwd = hash_password(password);
    if (dbm->userExists(username, hash_pwd)) {
        logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User already exists.");
    }
    else{
        dbm->insertUser(username, hash_pwd, ADMIN);
        logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User added successfully.");
    }
    getchar();

}

void handle_delete_user(SQLiteManager *dbm){

    char username[20];
    char password[20];
	char *hash_pwd = NULL;

    printf("Username: ");
    scanf("%s", username);
    getchar();
    printf("Password: ");
    scanf("%s", password);
    getchar();

	hash_pwd = hash_password(password);
    if (dbm->userExists(username, hash_pwd)) {
		dbm->deleteUser(username);
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User deleted successfully.");
	}
	else{
		logger(ORCH_INFO, MODULE_NAME, __FILE__, __LINE__, "User not found.");
	}
    getchar();

}

int main(int argc, char *argv[]) {

	SQLiteManager *dbm = NULL;
	char db_name[BUFFER_SIZE];

	// Check for root privileges
	if(geteuid() != 0) {
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Root permissions are required to run %s\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	sprintf(db_name, "../orchestrator/%s", DB_NAME);

	int cmd;
	do{
		cout << "1. Initialize database\n";
		cout << "2. Add user\n";
		cout << "3. Delete user\n";
		cout << "0. Exit\n";
		cout << "Choose: ";
		cin >> cmd;

        if(cmd!=0)
            dbm = new SQLiteManager(db_name);

		switch(cmd){
			case 1:
				handle_init(dbm);
				break;
			case 2:
				handle_add_user(dbm);
				break;
			case 3:
				handle_delete_user(dbm);
				break;
			case 0:
				break;
			default:
				cout << "Invalid command!\n";
		}
	}while(cmd!=0);


}
