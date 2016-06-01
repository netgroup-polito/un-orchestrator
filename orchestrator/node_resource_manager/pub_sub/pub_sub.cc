#include "pub_sub.h"

zactor_t *DoubleDeckerClient::client = NULL;
bool DoubleDeckerClient::connected = false;
list<publish_t> DoubleDeckerClient::messages;
pthread_mutex_t DoubleDeckerClient::connected_mutex;

bool DoubleDeckerClient::init(char *clientName, char *brokerAddress, char *keyPath)
{
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Inizializing the '%s'",DD_CLIENT_MODULE_NAME);
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "\t DD client name: '%s'",clientName);
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "\t DD broker address: '%s'",brokerAddress);
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "\t DD key to be used (path): '%s'",keyPath);

	pthread_mutex_init(&connected_mutex, NULL);

	//Start and register a DD client on the brocker
        client = ddactor_new(clientName, brokerAddress, keyPath);

	//Start a new thread that waits for events
	pthread_t thread[1];
	pthread_create(&thread[0],NULL,loop,NULL);

	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Module inizialized!");

	return true;
}

void *DoubleDeckerClient::loop(void *param)
{
	while(true)
	{
		//receive a message from the DD
		zmsg_t *msg = zmsg_recv (client);
		zmsg_print(msg);
		//retrieve the event
		char *event = zmsg_popstr(msg);

		if(streq("reg",event))
		{
			//When the registration is successful
			pthread_mutex_lock(&connected_mutex);
			connected = true;
			pthread_mutex_unlock(&connected_mutex);
			logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Succcessfully registered on the Double Decker network!");
			free(event);

			//Let's send all the messages stored in the list
			for(list<publish_t>::iterator m = messages.begin(); m != messages.end(); m++)
				publish(m->topic,m->message);
		}
		else if (streq("discon",event))
		{
			logger(ORCH_WARNING, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Connection with the Double Decker network has been lost!");
			free(event);
			//TODO: what to do in this case?
		}
		else if (streq("pub",event))
		{
			logger(ORCH_WARNING, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Received a 'publication' event. This event is ignored");
			free(event);
			//TODO: add here a callback that handle the proper event
		}
		else if (streq("data",event))
		{
			logger(ORCH_WARNING, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Received a 'data' event. This event is ignored");
			free(event);
		}
		else if (streq("$TERM",event))
		{
			char * error = zmsg_popstr(msg);
			logger(ORCH_ERROR, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Error while trying to connect to the Double Decker network: '%s'",error);
			zactor_destroy(&client);
			free(event);
			logger(ORCH_ERROR, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "This situation is not handled by the code. Please reboot the orchestrator and check if the broker is running!");
			signal(SIGALRM,sigalarm_handler);
			alarm(1);
			pthread_exit(NULL);
	    }
	}

	return NULL;
}

void DoubleDeckerClient::terminate()
{
	zsock_send(client, "s", "$TERM");
	zactor_destroy (&client);
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "The connection with the Double Decker is terminated");
}

void DoubleDeckerClient::publish(topic_t topic, const char *message)
{
	assert(client != NULL);

	pthread_mutex_lock(&connected_mutex);
	if(!connected)
	{
		//The client is not connected yet with the Double Decker network, then
		//add the message to a list
		publish_t publish;
		publish.topic = topic;
		publish.message = message;
		messages.push_back(publish);
		pthread_mutex_unlock(&connected_mutex);
		return;
	}
	pthread_mutex_unlock(&connected_mutex);

	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Publishing on topic '%s'",topicToString(topic));
	logger(ORCH_INFO, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Publishing message '%s'",message);

	int len = strlen(message);
	zsock_send(client,"sssb", "publish", topicToString(topic), message,&len, sizeof(len));
}

char *DoubleDeckerClient::topicToString(topic_t topic)
{
	switch(topic)
	{
		case FROG_DOMAIN_DESCRIPTION:
			return "frog:domain-description";
		case UNIFY_MMP:
			return "unify:mmp";
		default:
			assert(0 && "This is impossible!");
			return "";
	}
}

void DoubleDeckerClient::sigalarm_handler(int sig)
{
	logger(ORCH_ERROR, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "Error while trying to connect to the Double Decker network!");
	logger(ORCH_ERROR, DD_CLIENT_MODULE_NAME, __FILE__, __LINE__, "This situation is not handled by the code. Please reboot the orchestrator and check if the broker is running!");
	alarm(1);
}
