#include "ivshmem_cmdline_generator.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

static const char LOG_MODULE_NAME[] = "Command-Line-Generator";

using namespace std;

#define MEMPOOL_METADATA_NAME "OVSMEMPOOL"

bool IvshmemCmdLineGenerator::memorypool_generated = false;
pthread_mutex_t IvshmemCmdLineGenerator::memory_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

char IvshmemCmdLineGenerator::mempool_cmd[512];

IvshmemCmdLineGenerator::IvshmemCmdLineGenerator()
{

}

bool IvshmemCmdLineGenerator::get_mempool_cmdline(char * cmdline, int size)
{
	int r;

	ULOG_DBG_INFO("Generating command line for memory pool");

	pthread_mutex_lock(&memory_pool_mutex);
	if(memorypool_generated)
		goto ready;

	//unlink(MEMPOOL_METADATA_NAME);
	//if(mkfifo(MEMPOOL_METADATA_NAME, 0666) == -1)
	//{
	//	ULOG_DBG_INFO("Error creating named pipe: %s", strerror(errno));
	//	goto error;
	//}

	r = system("./compute_controller/plugins/kvm-libvirt/cmdline_generator/build/cmdline_generator -m");
	ULOG_DBG_INFO("Command line executed");
	if(r == -1 || WEXITSTATUS(r) == -1)
	{
		ULOG_DBG_INFO("Error executing command line generator");
		//unlink(MEMPOOL_METADATA_NAME);
		goto error;
	}

	if(!read_from_file(MEMPOOL_METADATA_NAME, mempool_cmd, sizeof(mempool_cmd)))
	{
		//unlink(MEMPOOL_METADATA_NAME);
		goto error;
	}

	ULOG_DBG_INFO("Command line ready");


	unlink(MEMPOOL_METADATA_NAME);

	memorypool_generated = true;

ready:
	strncpy(cmdline, mempool_cmd, size);
	pthread_mutex_unlock(&memory_pool_mutex);
	return true;

error:
	pthread_mutex_unlock(&memory_pool_mutex);;
	return false;
}

bool
IvshmemCmdLineGenerator::get_port_cmdline(const char * port_name, const char * port_alias, char * cmdline, int size)
{
	ULOG_DBG_INFO("Generating command line for port '%s' (alias '%s')", port_name, port_alias ? port_alias : "N/A");

	char c[100];
	int r;

	sprintf(c, "./compute_controller/plugins/kvm-libvirt/cmdline_generator/build/cmdline_generator -p %s %s", port_name, port_alias);

	//if(mkfifo(port_name, 0666) == -1)
	//{
	//	ULOG_DBG_INFO("Error creating named pipe");
	//	goto error;
	//}

	r = system(c);
	if(r == -1 || WEXITSTATUS(r) == -1)
	{
		ULOG_DBG_INFO("Error executing command line generator");
		//unlink(MEMPOOL_METADATA_NAME);
		goto error;
	}

	if(!read_from_file(port_name, cmdline, size))
	{
		//unlink(port_name);
		goto error;
	}

	unlink(port_name);

	return true;
error:
	return false;
}

bool IvshmemCmdLineGenerator::get_single_cmdline(char * cmdline, int size, const std::string& vnf_name, std::vector<std::pair< std::string, std::string> >& port_names)
{
    int r;

    ostringstream oss;
    oss << "./compute_controller/plugins/kvm-libvirt/cmdline_generator/build/cmdline_generator";
    oss << " -n " << vnf_name << " -m";
    for (vector< pair<string, string> >::iterator it = port_names.begin(); it != port_names.end(); ++it) {
	    oss << " -p " << it->first << " " << it->second;
    }
    ULOG_DBG_INFO("Generating IVSHMEM QEMU command line using: %s", oss.str().c_str());

    r = system(oss.str().c_str());
    if(r == -1 || WEXITSTATUS(r) == -1)
    {
        ULOG_DBG_INFO("Error executing command line generator");
        //unlink(MEMPOOL_METADATA_NAME);
        goto error;
    }

    if(!read_from_file(vnf_name.c_str(), cmdline, size))
    {
        //unlink(vnf_name.c_str());
        goto error;
    }

    unlink(vnf_name.c_str());

    return true;
error:
    return false;
}

/*
bool IvshmemCmdLineGenerator::read_from_pipe(const char *name, char *buf, size_t len)
{
	int fd = open(name, O_RDONLY);
	if(fd == -1)
	{
		ULOG_DBG_INFO("Error opening pipe");
		return false;
	}

	int n;
	int total = 0;
	char * p = buf;

	do
	{
		n = read(fd, p, len - total);
		if(n == -1)
		{
			ULOG_DBG_INFO("Error reading from pipe");
			return false;
		}

		total += n;
		p += n;
	} while(n != 0);

	return true;
}
*/
bool IvshmemCmdLineGenerator::read_from_file(const char *name, char *buf, size_t len)
{
	FILE * f = fopen(name, "r");
	if(f == NULL)
	{
		ULOG_DBG_INFO("Error opening file");
		return false;
	}

	if(fgets(buf, len, f) == NULL)
	{
		ULOG_DBG_INFO("Error in reading file");
		return false;
	}
	return true;
}

