# UN Security Module

## Overview

Security Manager is the component that implements the security features available in the Universal Node. Currently, this module exposes the following features:
- User authentication
- User authorization (i.e. permissions)

These are the steps every REST request must pass in order to be processed by the UN.

## Security and permissions: database structure

Security and permissions with respect to the basic CRUD operations are managed through a local database, whose structure is depicted below:

![database-structure](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/new_master/images/Database.JPG)

As shown in the diagram above, the local database is composed by the following tables:
- Users: stores general information about the subscribed users in the node orchestrator
- Login: keeps track of the users that have been authenticated in the system
- Generic resources: lists the available classes of resources
- Current resources permissions: lists the available resources with the related permissions
- User creation permissions: for each user, specify the permission for creating an instance of a generic resource
- Default usage permissions: specifies default permissions to be assigned to the resources belonging to a particular class (i.e. generic resource)

## Authentication
- The user sends an authentication request to the server through the REST API
- If authentication is successful, the server provides the user a token
- For each following request, the user must provide the so-obtained authentication token to prove its identity to the UN

## Authorization

### Scenario 1: Read/Update/Delete request
Once a user requests a Read/Update/Delete operation on a resource, the UN follows the steps depicted below.
![create-permissions](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/new_master/images/RUD.png)

After authenticating the user, the UN Orchestrator reads the CURRENT_RESOURCES_PERMISSIONS table to check whether the requested resource does exist or not. If it does, authorization is verified reading the proper permission flag included the table accessed before. According to the user's role (e.g. owner of the resource, member of the owner's group, system administrator, ...) and the request type (read, update, delete) the proper permission flag is selected and evaluated. In particular:
 - The proper permission string is selected among the following attributes of the CURRENT_RESOURCE_PERMISSIONS table: OWNER_PERMISSION, GROUP_PERMISSION, ALL_PERMISSION and ADMIN_PERMISSION.
 - Since each one of these attributes is a 3-tuple of flags: {read, upate, delete}. Therefore, the user will be authroized or not according to the value of the flag related to the requested operation.

If also this test is passed, the UN creates the new resource and assigns it the default permissions, specified in the DEFAULT_USAGE_PERMISSION table.

### Scenario 2: Creation request
On the other hand, authorization handling is more complex when the user asks to create a new resource. Therefore, it has been represented as a separated workflow, which is depicted below.
![rud-permissions](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/new_master/images/Creation.png)

Since creation permissions are related to not existing resources, they cannot be included in the CURRENT_RESOURCES_PERMISSIONS. For this reason, the authorization phase is slightly different in case of creation requests. In particular, the UN executes a lookup in the USER_CREATION_PERMISSION table, which specifies the authorization to create an instance of a generic resource for each user. In case the previous phases are completed successfully, the new resource is created with default permissions, which are related to the generic reosurce it belongs to (see DEFAULT_RESOURCES_PERMISSIONS table).

## Creating the database and enabling the security in the UN
The database structure described above is created by the [db_inizializer](../db_inizializer) module.
In order to enable the security in the un-orchestrator, you have to edit its configuration file [config/default-config.ini](config/default-config.ini#L16) by setting `user_authentication = true`.
