# Database Initalizer


## Overview

Database Initializer is the module responsible for creating and populating the database of the Universal Node orchestrator. The tables involved are briefly described below:
- Users: specifies information about users (e.g. password, group, ...)
- Login: includes the users currently authenticated with login information
- User creation permissions: for each user, it defines the permission to create a resource belonging to a particular class
- Current resources permissions: includes the resources provided by the system plus permissions information
- Default usage permissions: default permissions for each class of resoruces

## Compile the Database Initializer

	$ cd [un-orchestrator]

	; Choose among possible compilation options
	$ ccmake .

The previous command allows you to select some configuration parameters for the
db-initializder, such as the logging level. 
**Please be sure that the option `BUILD_DBInitializer ` is `ON`.**
When you're finished, exit from the `ccmake` interface by 
*generating the configuration files* (press 'c' and 'g')
and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executable
	$ make

# How to run the Database Initializer

	$ sudo ./db_initializer <default-admin-password>

This will create the user database with minimal data in it, with a standard user 'admin' associated to the password specified in the command line.
