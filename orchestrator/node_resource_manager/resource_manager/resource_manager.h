#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_ 1

#include "../pub_sub/pub_sub.h"
#include "../../utils/logger.h"
#include "../../utils/constants.h"

class ResourceManager
{
	/**
	*	@brief: this class exports resources of the Universal Node, by means of the
	**		DoubleDecker client
	*/

public:
	/**
	*	@breif: Publish the domain information written in a specific file
	*
	*	@param	descr_file: file containing the descriptio of the domain to
	*		be exported
	*/
	static void publishDescriptionFromFile(char *descr_file);
};

#endif // RESOURCE_MANAGER_H_
