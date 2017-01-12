#define __USE_GNU 1
#define _GNU_SOURCE 1

#include "utils/constants.h"
#include "utils/logger.h"
#include "node_resource_manager/rest_server/rest_server.h"

#ifdef ENABLE_DOUBLE_DECKER_CONNECTION
	#include "node_resource_manager/pub_sub/pub_sub.h"
#endif

#ifdef ENABLE_RESOURCE_MANAGER
	#include "node_resource_manager/resource_manager/resource_manager.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <openssl/sha.h>
#include "node_resource_manager/database_manager/SQLite/SQLiteManager.h"

#include <INIReader.h>
#include "utils/configuration.h"

#include <signal.h>
#include <execinfo.h>
#include <sys/types.h>
#include <ucontext.h>
#ifdef __x86_64__
	#define USE_REG REG_RIP
//#else
//	#define USE_REG REG_EIP
#endif

static const char LOG_MODULE_NAME[] = "Local-Orchestrator";

/**
*	Global variables (defined in ../utils/constants.h)
*/
ofp_version_t OFP_VERSION;

/**
*	Private variables
*/
struct MHD_Daemon *http_daemon = NULL;

/*
*
* Pointer to database class
*
*/
SQLiteManager *dbm = NULL;

/**
*	Private prototypes
*/
bool parse_command_line(int argc, char *argv[],int *core_mask,char **config_file);
bool usage(void);
void printUniversalNodeInfo();
void terminateRestServer(void);


/**
*	Implementations
*/
void signal_handler(int sig, siginfo_t *info, void *secret)
{
	switch(sig)
	{
		case SIGINT:
			ULOG_INFO( "The '%s' is terminating...",MODULE_NAME);

			MHD_stop_daemon(http_daemon);
			terminateRestServer();

			if(dbm != NULL) {
				//dbm->updateDatabase();
				dbm->cleanTables();
			}
#ifdef ENABLE_DOUBLE_DECKER_CONNECTION
			DoubleDeckerClient::terminate();
#endif
			ULOG_INFO( "Bye :D");
			exit(EXIT_SUCCESS);
		break;
#ifdef __x86_64__
		//We print the stack only if the orchestrator is executed on an x86_64 machine
		case SIGSEGV:
		{
			void *trace[16];
			char **messages = (char **)NULL;
			int i, trace_size = 0;
			ucontext_t *uc = (ucontext_t *)secret;
			char *ret;
			ULOG_ERR( "");
			ULOG_ERR( "Got signal %d, faulty address is %p, from %p", sig, info->si_addr, uc->uc_mcontext.gregs[USE_REG]);

			trace_size = backtrace(trace, 16);
			/* overwrite sigaction with caller's address */
			trace[1] = (void *)uc->uc_mcontext.gregs[USE_REG];

			messages = backtrace_symbols(trace, trace_size);
			/* skip first stack frame (points here) */
			ULOG_ERR( "Backtrace -");
			for (i = 1; i < trace_size; ++i)
			{
				ULOG_ERR( "%s ", messages[i]);
				size_t p = 0;
				while (messages[i][p] != '(' && messages[i][p] != ' ' && messages[i][p] != 0)
					++p;
				char syscom[256];
				sprintf(syscom, "addr2line -f -p %p -e %.*s", trace[i], (int)p, messages[i]);

				char *output;
				FILE *fp;
				char path[1035];

				/* Open the command for reading. */
				fp = popen(syscom, "r");
				if (fp == NULL) {
					printf("Failed to run command %s", syscom);
				}
				ret = fgets(path, sizeof(path), fp);
				if (ret != path) {
					exit(EXIT_FAILURE);
				}
				fclose(fp);

				output = strdup(path);

				if (output != NULL)
				{
					ULOG_ERR( "%s", output);
					free(output);
				}
			}
			exit(EXIT_FAILURE);
		}
		break;
#endif
	}
}

int main(int argc, char *argv[])
{
	//Check for root privileges
	if(geteuid() != 0)
	{
		ULOG_ERR( "Root permissions are required to run %s\n",argv[0]);
		ULOG_ERR( "Cannot start the %s",MODULE_NAME);
		exit(EXIT_FAILURE);
	}

#if defined(VSWITCH_IMPLEMENTATION_ERFS) || defined(VSWITCH_IMPLEMENTATION_OVSDB)
	OFP_VERSION = OFP_13;
#else
	OFP_VERSION = OFP_12;
#endif

	int core_mask;
	char *config_file_name = new char[BUFFER_SIZE];

	strcpy(config_file_name, DEFAULT_FILE);

	if(!parse_command_line(argc,argv,&core_mask,&config_file_name))
	{
		ULOG_ERR( "Cannot start the %s",MODULE_NAME);
		exit(EXIT_FAILURE);
	}

	if(!Configuration::instance()->init(string(config_file_name)))
	{
		ULOG_ERR( "Cannot start the %s",MODULE_NAME);
		exit(EXIT_FAILURE);
	}

	if(Configuration::instance()->getUserAuthentication()) {
		std::ifstream ifile(DB_NAME);

		if(ifile)
			dbm = new SQLiteManager(DB_NAME);
		else {
			ULOG_ERR( "Database does not exist!");
			ULOG_ERR( "Run 'db_initializer' at first.");
			ULOG_ERR( "Cannot start the %s",MODULE_NAME);
			exit(EXIT_FAILURE);
		}
	}

#ifdef ENABLE_DOUBLE_DECKER_CONNECTION
	if(!DoubleDeckerClient::init(Configuration::instance()->getDdClientName(), Configuration::instance()->getDdBrokerAddress(), Configuration::instance()->getDdKeyPath()))
	{
		ULOG_ERR( "Cannot start the %s",MODULE_NAME);
		exit(EXIT_FAILURE);
	}
#endif

	if(!RestServer::init(dbm,core_mask))
	{
		ULOG_ERR( "Cannot start the %s",MODULE_NAME);
		exit(EXIT_FAILURE);
	}

#ifdef ENABLE_RESOURCE_MANAGER
	ResourceManager::publishDescriptionFromFile(Configuration::instance()->getDescriptionFileName());
#endif

	http_daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, Configuration::instance()->getRestPort(), NULL, NULL,&RestServer::answer_to_connection,
		NULL, MHD_OPTION_NOTIFY_COMPLETED, &RestServer::request_completed, NULL,MHD_OPTION_END);

	if (NULL == http_daemon)
	{
		ULOG_ERR( "Cannot start the HTTP deamon. The %s cannot be run.",MODULE_NAME);
		ULOG_ERR( "Please, check that the TCP port %d is not used (use the command \"netstat -lnp | grep %d\")",Configuration::instance()->getRestPort(),Configuration::instance()->getRestPort());

		terminateRestServer();

		return EXIT_FAILURE;
	}

	// Ignore all signals but SIGSEGV and SIGINT
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	sigset_t unblock;
	sigaddset(&unblock,SIGINT);
#ifdef __x86_64__
	sigaddset(&unblock,SIGSEGV);
#endif
	sigprocmask(SIG_UNBLOCK,&unblock,&mask);

	/* Install signal handlers */
	struct sigaction sa;

	sa.sa_sigaction = &signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

#ifdef __x86_64__
	sigaction(SIGSEGV, &sa, NULL);
#endif
	sigaction(SIGINT, &sa, NULL);

	printUniversalNodeInfo();
	ULOG_INFO("The '%s' is started!",MODULE_NAME);
	ULOG_INFO("Waiting for commands on TCP port \"%d\"",Configuration::instance()->getRestPort());

	while(true) {
		struct timeval tv;
		tv.tv_sec = 3600;
		tv.tv_usec = 0;
		select(0, NULL, NULL, NULL, &tv);
	}

	return 0;
}

bool parse_command_line(int argc, char *argv[],int *core_mask,char **config_file_name)
{
	int opt;
	char **argvopt;
	int option_index;

static struct option lgopts[] = {
		{"c", 1, 0, 0},
		{"d", 1, 0, 0},
		{"h", 0, 0, 0},
		{NULL, 0, 0, 0}
	};

	argvopt = argv;
	uint32_t arg_c = 0;

	*core_mask = CORE_MASK;

	while ((opt = getopt_long(argc, argvopt, "", lgopts, &option_index)) != EOF)
	{
		switch (opt)
		{
			/* long options */
			case 0:
				if (!strcmp(lgopts[option_index].name, "c"))/* core mask for network functions */
				{
					if(arg_c > 0)
					{
						ULOG_ERR( "Argument \"--c\" can appear only once in the command line");
						return usage();
					}
					char *port = (char*)malloc(sizeof(char)*(strlen(optarg)+1));
					strcpy(port,optarg);

					sscanf(port,"%x",&(*core_mask));

					arg_c++;
				}
				else if (!strcmp(lgopts[option_index].name, "d"))/* inserting configuration file */
				{
					if(arg_c > 0)
					{
						ULOG_ERR( "Argument \"--d\" can appear only once in the command line");
						return usage();
					}

					strcpy(*config_file_name,optarg);

					arg_c++;
				}
				else if (!strcmp(lgopts[option_index].name, "h"))/* help */
				{
					return usage();
				}
				else
				{
					ULOG_ERR( "Invalid command line parameter '%s'\n",lgopts[option_index].name);
					return usage();
				}
				break;
			default:
				return usage();
		}
	}

	return true;
}

bool usage(void)
{
	stringstream message;

	message << "Usage:                                                                        \n" \
	"  sudo ./name-orchestrator --d configuration_file [options]     						  \n" \
	"                                                                                         \n" \
	"Parameters:                                                                              \n" \
	"  --d configuration_file                                                                 \n" \
	"        File that specifies some parameters such as rest port, physical port file,       \n" \
	"        NF-FG to deploy at the boot, and if client authentication is required            \n" \
	"                                                                                         \n" \
	"Options:                                                                                 \n" \
	"  --c core_mask                                                                           \n" \
	"        Mask that specifies which cores must be used for DPDK network functions. These   \n" \
	"        cores will be allocated to the DPDK network functions in a round robin fashion   \n" \
	"        (default is 0x2)                                                                 \n" \
	"  --h                                                                                    \n" \
	"        Print this help.                                                                 \n" \
	"                                                                                         \n" \
	"Example:                                                                                 \n" \
	"  sudo ./node-orchestrator --d config/default-config.ini	  							  \n";

	ULOG_INFO( "\n\n%s",message.str().c_str());

	return false;
}

/**
*	Prints information about the vSwitch configured and the execution environments supported
*/
void printUniversalNodeInfo()
{

ULOG_INFO( "************************************");

#ifdef __x86_64__
	ULOG_INFO( "The %s is executed on an x86_64 machine",MODULE_NAME);
#endif

#ifdef VSWITCH_IMPLEMENTATION_XDPD
	string vswitch = "xDPd";
#endif
#ifdef VSWITCH_IMPLEMENTATION_OVSDB
	stringstream ssvswitch;
	ssvswitch << "OvS with OVSDB protocol";
#ifdef ENABLE_OVSDB_DPDK
	ssvswitch << " (DPDK support enabled)";
#endif
	string vswitch = ssvswitch.str();
#endif
#ifdef VSWITCH_IMPLEMENTATION_ERFS
	string vswitch = "ERFS";
#endif
	ULOG_INFO( "* Virtual switch used: '%s'", vswitch.c_str());

	list<string> executionenvironment;
#ifdef ENABLE_KVM
	executionenvironment.push_back("virtual machines");
#endif
#ifdef ENABLE_DOCKER
	executionenvironment.push_back("Docker containers");
#endif
#ifdef ENABLE_DPDK_PROCESSES
	executionenvironment.push_back("DPDK processes");
#endif
#ifdef ENABLE_NATIVE
	executionenvironment.push_back("native functions");
#endif
	ULOG_INFO( "* Execution environments supported:");
	for(list<string>::iterator ee = executionenvironment.begin(); ee != executionenvironment.end(); ee++)
		ULOG_INFO( "* \t'%s'",ee->c_str());

#ifdef ENABLE_DOUBLE_DECKER_CONNECTION
	ULOG_INFO( "* Double Decker connection is enabled");
#endif

ULOG_INFO( "************************************");
}

void terminateRestServer() {
	try {
		RestServer::terminate();
	} catch(...) {
		//Do nothing, since the program is terminating
	}
}
