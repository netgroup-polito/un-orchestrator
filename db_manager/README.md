# Database Manager


## Overview

Database Initializer is the module responsible for creating and populating the database of the Universal Node orchestrator. The tables involved are briefly described below:
- Users: specifies information about users (e.g. password, group, ...)
- Login: includes the users currently authenticated with login information
- User creation permissions: for each user, it defines the permission to create a resource belonging to a particular class
- Current resources permissions: includes the resources provided by the system plus permissions information
- Default usage permissions: default permissions for each class of resources

It allows to do:
    -  Initialize the database:
        - this will create the user database with minimal data in it, with a standard user 'admin'
    -  Add a new user into the database
    -  Delete an existent user from the database


## Compile the Database Manager

	$ cd [un-orchestrator]

	; Choose among possible compilation options
	$ ccmake .

The previous command allows you to select some configuration parameters for the
db_manager, such as the logging level.
**Please be sure that the option `BUILD_DBManager ` is `ON`.**
When you're finished, exit from the `ccmake` interface by 
*generating the configuration files* (press 'c' and 'g')
and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executable
	$ make

# How to run the Database Manager

	$ sudo ./db_manager

