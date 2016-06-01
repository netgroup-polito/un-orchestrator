#include "resource_manager.h"

void ResourceManager::publishDescriptionFromFile(char *descr_file)
{
	assert(descr_file != NULL);

	char c, *mesg = "";

	FILE *fp = fopen(descr_file, "r");
	if(fp == NULL)
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Something wrong while opening file '%s'.",descr_file);

	int i = 0, n = 0;
	while(fscanf(fp, "%c", &c) != EOF)
		i++;

	n = i;

	mesg = (char *)calloc(n, sizeof(char));

	fclose(fp);

	//FIXME: instead of closing the file and opening it again immediately, I think that there exist
	//some functions that allows to return back to the top of the file
	//Open the file again
	fp = fopen(descr_file, "r");
	if(fp == NULL)
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Something wrong while opening file '%s'.",descr_file);

	for(i=0;i<n;i++)
	{
		fscanf(fp, "%c", &c);
		mesg[i] = c;
	}

	mesg[i-1] = '\0';

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Publishing node configuration.");

	fclose(fp);

	//publish the domain description
	DoubleDeckerClient::publish(FROG_DOMAIN_DESCRIPTION, mesg);
}

//TODO currently not used. It must be ported to use the new DD client
#if 0
//Export Domain Information
bool ResourceManager::publishUpdating()
{
	char c, *mesg = "";

	FILE *fp = fopen(FILE_NAME, "r");
	if(fp == NULL)
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "ERROR reading file.");

	int i = 0, n = 0;
	while(fscanf(fp, "%c", &c) != EOF){
		i++;
	}

	n = i;

	mesg = (char *)calloc(n, sizeof(char));

	fclose(fp);

	fp = fopen(FILE_NAME, "r");
	if(fp == NULL)
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "ERROR reading file.");

	for(i=0;i<n;i++){
		fscanf(fp, "%c", &c);
		mesg[i] = c;
	}

	mesg[i-1] = '\0';

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Publishing node configuration.");

	fclose(fp);

	//publish NF-FG
	client->publish("NF-FG", mesg, strlen(mesg), client);

	return true;
}
#endif
