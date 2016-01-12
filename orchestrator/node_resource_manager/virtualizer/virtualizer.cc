#include "virtualizer.h"

map<string,string> Virtualizer::nameVirtualization;
unsigned int Virtualizer::currentID = 1;

bool Virtualizer::init()
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Initializing the virtualizer...");

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
    {
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_INIT);
		if (pythonFunction && PyCallable_Check(pythonFunction))
        {
	    	PyObject *pythonRetVal;


	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, NULL);
            long int retVal = PyInt_AsLong(pythonRetVal);

            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);

	    	if(retVal)
	    	{
	    		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The virtualizer has been properly inizialized");
	    		return true;
	    	}
	    	else
	    	{
	    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred during the inizialization of the virtualizer!");
	    		return false;
	    	}
	    }
	    else
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_INIT);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }
}

void Virtualizer::terminate()
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Terminating the virtualizer...");

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
    {
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_TERMINATE);
		if (pythonFunction && PyCallable_Check(pythonFunction))
        {
	    	//Call the python function
	    	PyObject_CallObject(pythonFunction, NULL);

            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);
	    }
	    else
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_INIT);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
    }

    logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The virtualizer has been properly terminated");
}

bool Virtualizer::addResources(int cpu, int memory, char *memory_unit, int storage, char *storage_unit)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Adding resources to the virtualizer:");
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t* CPU: %d",cpu);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t* memory: %d %s",memory,memory_unit);
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\t* memory: %d %s",storage,storage_unit);

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
    {
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADDRESOURCES);
		if (pythonFunction && PyCallable_Check(pythonFunction))
        {
	    	PyObject *pythonArgs = NULL, *pythonRetVal, *pythonValue;

	    	//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			pythonArgs = PyTuple_New(5);
	    	pythonValue = PyInt_FromLong(cpu);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyInt_FromLong(memory);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);
            pythonValue = PyString_FromString(memory_unit);
            PyTuple_SetItem(pythonArgs, 2, pythonValue);
            pythonValue = PyInt_FromLong(storage);
            PyTuple_SetItem(pythonArgs, 3, pythonValue);
            pythonValue = PyString_FromString(storage_unit);
            PyTuple_SetItem(pythonArgs, 4, pythonValue);

	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);

			long int retVal = PyInt_AsLong(pythonRetVal);
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);

	    	if(retVal)
	    	{
	    		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Resources added to the virtualizer");
	    		return true;
	    	}
	    	else
	    	{
	    		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "An error occurred while adding resources to the virtualizer");
	    		return false;
	    	}
	    }
	    else
        {
            if (PyErr_Occurred())
                PyErr_Print();

		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
			return false;
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }
}

bool Virtualizer::addPort(char *physicalName, char *name, char *type)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Adding port to the virtualizer...");

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
    {
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADDNODEPORT);
		if (pythonFunction && PyCallable_Check(pythonFunction))
        {
	    	PyObject *pythonArgs = NULL, *pythonRetVal, *pythonValue;

	    	//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			pythonArgs = PyTuple_New(2);
	    	pythonValue = PyString_FromString(name);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyString_FromString(type);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);

	    	//Call the python function
	    	pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);

			long int retVal = PyInt_AsLong(pythonRetVal);
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

			Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);

	    	if(retVal)
	    	{
	    		string pn(physicalName);
	    		string n(name);
	    		nameVirtualization[pn] = n;
	    		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Port added to the virtualizer");
	    		return true;
	    	}
	    	else
	    	{
	    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occured while adding a port to the virtualizer");
	    		return false;
	    	}
	    }
	    else
        {
            if (PyErr_Occurred())
                PyErr_Print();

		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
		   	Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
			return false;
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		return false;
    }
}

bool Virtualizer::EditPortID(string physicalPortName, unsigned int ID)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Setting ID %d to port %s...",ID,physicalPortName.c_str());

	if(nameVirtualization.count(physicalPortName) == 0)
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Virtualization unknown for physical port '%s'",physicalPortName.c_str());
		return false;
	}
	string portName = nameVirtualization[physicalPortName];

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
	{
    	PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_EDIT_PORT_ID);
    	if (pythonFunction && PyCallable_Check(pythonFunction))
    	{
    		//Set the arguments to the python function
	    	//TODO: check that pythonValue is not NULL
			PyObject *pythonArgs = PyTuple_New(2);
	    	PyObject *pythonValue = PyString_FromString(portName.c_str());
	    	PyTuple_SetItem(pythonArgs, 0, pythonValue);
	    	pythonValue = PyInt_FromLong(ID);
	    	PyTuple_SetItem(pythonArgs, 1, pythonValue);

	    	//Call the python function
	    	PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
            Py_DECREF(pythonArgs);

            long int retVal = PyInt_AsLong(pythonRetVal);
            logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

            Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);

	    	if(retVal)
	    	{
	    		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "The port has been edited correctly");
	    		return true;
	    	}
	    	else
	    	{
	    		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Something went wrong while eding the port!");
	    		return false;
	    	}
    	}
    	else
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_EDIT_PORT_ID);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;
		}
	}
	else
    {
	   	PyErr_Print();
	   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
	   	return false;
	}
}

bool Virtualizer::addSupportedVNFs(set<NF*> nfs)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Adding supported VNFs to the virtualizer...");

	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
	{
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_ADD_SUPPORTED_VNFs);
		if (pythonFunction && PyCallable_Check(pythonFunction))
		{
			//Iterate on all the NFs
			for(set<NF*>::iterator nfIt = nfs.begin(); nfIt != nfs.end(); nfIt++)
			{
				//Iterate on all the available implementations for a NF
				NF *nf = *nfIt;


				ostringstream sID;
				sID << currentID;

				string id("NF"+sID.str());
				currentID++;

				//ID, name, vnftype, numports

				//Set the arguments to the python function
				//TODO: check that pythonValue is not NULL
				PyObject *pythonArgs = PyTuple_New(4);
				PyObject *pythonValue = PyString_FromString(id.c_str());
				PyTuple_SetItem(pythonArgs, 0, pythonValue);

				//XXX: I use the network function description as name of the VNF
				pythonValue = PyString_FromString((nf->getTextDescription()).c_str());
				PyTuple_SetItem(pythonArgs, 1, pythonValue);

				//XXX: I use the network function name as type of the VNF
				pythonValue = PyString_FromString((nf->getName()).c_str());
				PyTuple_SetItem(pythonArgs, 2, pythonValue);

				pythonValue = PyInt_FromLong(nf->getNumPorts());
				PyTuple_SetItem(pythonArgs, 3, pythonValue);

				//Call the python function
				PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
			    Py_DECREF(pythonArgs);

			    long int retVal = PyInt_AsLong(pythonRetVal);
			    logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %d\n", retVal);

			    Py_DECREF(pythonRetVal);

				if(!retVal)
				{
					logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Something went wrong while adding VNF %s",(nf->getName()).c_str());
					Py_XDECREF(pythonFunction);
					Py_DECREF(pythonFile);
					return false;
				}

			}
			Py_XDECREF(pythonFunction);
			Py_DECREF(pythonFile);
		}
    	else
        {
            if (PyErr_Occurred())
                PyErr_Print();
		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_ADD_SUPPORTED_VNFs);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);
	        return false;
		}

	}
	else
    {
	   	PyErr_Print();
	   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
	   	return false;
	}

	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "VNFs properly added to the virtualizer!");

	return true;
}

handleRequest_status_t Virtualizer::handleRestRequest(char *message, char **answer, const char *url, const char *method)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Handling a REST request in the virtualizer...");

	//In this case, the request in handled by the Python code
	PyObject *pythonFileName = PyString_FromString(PYTHON_MAIN_FILE);
	PyObject *pythonFile = PyImport_Import(pythonFileName);
	Py_DECREF(pythonFileName);
	if (pythonFile != NULL)
    {
		PyObject *pythonFunction = PyObject_GetAttrString(pythonFile, PYTHON_HANDLE_REQ);
		if (pythonFunction && PyCallable_Check(pythonFunction))
        {
	    	PyObject *pythonArgs = NULL, *pythonValue;


			//Set the arguments to the python function
 			int numArgs = (message != NULL)? 3 : 2;
			pythonArgs = PyTuple_New(numArgs);
	    	pythonValue = PyString_FromString(method);
            PyTuple_SetItem(pythonArgs, 0, pythonValue);
            pythonValue = PyString_FromString(url);
            PyTuple_SetItem(pythonArgs, 1, pythonValue);
			if(message != NULL)
			{
				pythonValue = PyString_FromString(message);
				PyTuple_SetItem(pythonArgs, 2, pythonValue);
			}

	    	//Call the python function
	    	PyObject *pythonRetVal = PyObject_CallObject(pythonFunction, pythonArgs);
	    	Py_DECREF(pythonArgs);

			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Result of call: %s\n", PyString_AsString(pythonRetVal));

            string retVal = PyString_AsString(pythonRetVal);

	    	Py_DECREF(pythonRetVal);
            Py_XDECREF(pythonFunction);
	    	Py_DECREF(pythonFile);

	    	//TODO: use return values to distinguish the type of ERROR?
	    	if ( (0 == strcmp (method, GET)) ||  ((0 == strcmp (method, POST) ) && ( retVal != "ERROR")) )
	    	{
	    		//Everything was fine in handling the request
				*answer = NULL;
		 		*answer = (char*)malloc(sizeof(char) * (retVal.size()+1));
		 		memcpy((*answer), retVal.c_str(),retVal.size()+1);
		 		(*answer)[retVal.size()] = '\0';

				logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "REST request properly handled in the virtualizer...");

			    if(retVal == "config updated")
			    	return HR_EDIT_CONFIG;
			    else
			    	return HR_OK;
			}
			else
			{
				logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "An error occurred while handling a REST request in the virtualizer!");
				return HR_INTERNAL_ERROR;
			}
	    }
	    else
        {
            if (PyErr_Occurred())
                PyErr_Print();

		   	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python method \"%s\"",PYTHON_HANDLE_REQ);
			Py_XDECREF(pythonFunction);
	        Py_DECREF(pythonFile);

	        return HR_INTERNAL_ERROR;
		}
    }
    else
    {
       	PyErr_Print();
       	logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot load python file \"%s\"",PYTHON_MAIN_FILE);
		Py_DECREF(pythonFile);
		return HR_INTERNAL_ERROR;
    }
}

string Virtualizer::getRealName(string port)
{
	logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "Translating virtual port \"%s\"",port.c_str());

	for(map<string,string>::iterator it = nameVirtualization.begin(); it != nameVirtualization.end(); it++)
	{
		logger(ORCH_DEBUG_INFO, MODULE_NAME, __FILE__, __LINE__, "\tcurrent: \"%s\"",it->second.c_str());
		if(it->second == port)
		{
			logger(ORCH_DEBUG, MODULE_NAME, __FILE__, __LINE__, "Virtual port \"%s\" is actually the port \"%s\" of the node",port.c_str(),(it->first).c_str());
			return it->first;
		}
	}

	assert(0 && "There is a BUG!");
	throw VirtualizerException();
}

