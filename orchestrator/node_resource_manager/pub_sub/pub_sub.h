#ifndef PUB_SUB_H_
#define PUB_SUB_H_ 1

#include <string>
#include <list>
#include <unistd.h>
#include "../configuration_agent/ConfigurationAgent.h"
#include <czmq.h>
#include <dd.h>

#include "../../utils/logger.h"
#include "pub_sub_constants.h"

using namespace std;

typedef enum {
	FROG_DOMAIN_DESCRIPTION,
    UN_CONFIGURATION
	//[+] add here other topics
}topic_t;

typedef enum {
	ALL,
	REGION,
	CLUSTER,
	NODE,
	NOSCOPE
}scope_t;

struct publish_t{
	topic_t topic;
	const char *message;
};

struct subscribe_t{
	topic_t topic;
	scope_t scope;
};

class DoubleDeckerClient
{
private:
	/**
	*	@brief: this is the client that interacts with the Double
	*		Decker bus
	*/
	static zactor_t *client;

	/**
	*	@brief: this variable is true when the connection with the
	*		Double Decker is established
	*/
	static bool connected;

	static char *clientName; 
	static char *brokerAddress;
	static char *keyPath;

	/**
	*	@brief: list contaning messages to be sent (and the related topic)
	*/
	static list<publish_t> messages;

	/**
	*	@brief: list contaning topics to which subscribe
	*/
	static list<subscribe_t> topics;

	/**
	*	@brief: semaphore to serialize some operations of the Double Decker
	*		client
	*/
	static pthread_mutex_t connected_mutex;

	/**
	*	@brief: this function just print an error message
	*/
	static void sigalarm_handler(int sig);

	/**
	*	@brief: wait for messages coming from the DoubleDecker network
	*/
	static void *loop(void *param);

	/**
	*	@brief: given a topic, returns a string to be used on the
	*		DoubleDecker network
	*/
	static char *topicToString(topic_t topic);

	/**
	*	@brief: given a scope, returns a string to be used on the
	*		DoubleDecker network
	*/
	static char *scopeToString(scope_t scope);

	static ConfigurationAgent configurationAgent;

	DoubleDeckerClient() {}

public:
	/**
	*	@brief: Inizialize the client and connect it to the Double
	*		Decker network
	*
	*	@param	clientName: name of the client in the Double Decker
	*		network
	*	@param	brokerAddress: address:port of the brocker to be
	*		connected to
	*/
	static bool init(char *clientName, char *brokerAddress, char *keyPath);

	/**
	*	@brief: Disconnect the orchestrator from the Double Decker network
	*/
	static void terminate();

	static void publish(topic_t topic, const char *message);

	static void subscribe(topic_t topic/*, scope_t scope*/);
};

#endif //PUB_SUB_H_ 1
