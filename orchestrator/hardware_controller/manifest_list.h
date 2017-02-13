#ifndef MANIFEST_LIST_H
#define MANIFEST_LIST_H 1

#include "drivers/sample_driver.h"
#include "hw_manager.h"

using namespace std;

list<HWManager *> getHardwareManifestList()
{
	list<HWManager *> manifestList;
	/*****************************************************/
	/* Add the initialization of the HW used on the node */
	/*****************************************************/
	manifestList.push_front(new SampleDriver());





	return manifestList;
}

#endif
