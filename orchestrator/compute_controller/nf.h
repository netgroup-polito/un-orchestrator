#ifndef NF_H_
#define NF_H_ 1

#pragma once

#include <string>
#include <assert.h>
#include <list>

#include "../utils/constants.h"
#include "description.h"
#include "nfs_manager.h"

using namespace std;
class Description;
class NFsManager;

class NF
{
private:
	/**
	*	@brief: name of the NF. This should be unique
	*/
	string name;


	/**
	*	@brief: available descriptions of the NF
	*/
	list<Description*> descriptions;

	/**
	*	@brief: manager associated with the selected description for this NF
	*/
	NFsManager *selectedDescription;

	/**
	*	@brief: true if the network function is running, false otherwise
	*/
	bool isRunning;

public:
	NF(string name);

	void addDescription(Description *description);
	list<Description*> getAvailableDescriptions();

	void setSelectedDescription(NFsManager *impl);
	NFsManager *getSelectedDescription();

	void setRunning(bool val);
	bool getRunning();

	string getName();
};

#endif //NF_H_ 1
