#include "pub_sub.h"

static const char LOG_MODULE_NAME[] = "Double-Decker-Client";

dd_t *DoubleDeckerClient::client = NULL;
bool DoubleDeckerClient::connected = false;
list<publish_t> DoubleDeckerClient::messages;
pthread_mutex_t DoubleDeckerClient::connected_mutex;
// Store the configuration in the DoubleDeckerClient object
char *DoubleDeckerClient::clientName;
char *DoubleDeckerClient::brokerAddress;
char *DoubleDeckerClient::keyPath;
bool keep_looping = true;


int pipe_fd[2];

//callback function
void on_reg(void *args) {
	dd_t *dd = (dd_t *)args;
	(void) dd;	
//	printf("\nRegistered with broker %s!\n", dd_get_endpoint(dd));
	dd_un_msg_t msg=reg_dd_un_msg;
	write(pipe_fd[1], &msg, sizeof(dd_un_msg_t));
	fflush(stdout);
}

void on_discon(void *args) {
	dd_t *dd = (dd_t *)args;
	(void) dd;	
//	printf("\nGot disconnected from broker %s!\n", dd_get_endpoint(dd));
	dd_un_msg_t msg=discon_dd_un_msg;
	write(pipe_fd[1], &msg, sizeof(dd_un_msg_t));
	fflush(stdout);
}

void on_pub(char *source, char *topic, unsigned char *data, int length, void *args) {
	dd_t *dd = (dd_t *)args;
	(void) dd;	
//	printf("\nPUB S: %s T: %s L: %d D: '%s'", source, topic, length, data);
	dd_un_msg_t msg=pub_dd_un_msg;
	write(pipe_fd[1], &msg, sizeof(dd_un_msg_t));
	fflush(stdout);
}

void on_data(char *source, unsigned char *data, int length, void *args) {
	dd_t *dd = (dd_t *)args;
	(void) dd;	
//	printf("\nDATA S: %s L: %d D: '%s'", source, length, data);
	dd_un_msg_t msg=pub_dd_un_msg;
	write(pipe_fd[1], &msg, sizeof(dd_un_msg_t));
	fflush(stdout);
}

void on_error(int error_code, char *error_message, void *args) {
	switch (error_code) {
	case DD_ERROR_NODST:
		printf("Error - no destination: %s\n", error_message);
		break;
	case DD_ERROR_REGFAIL:
		printf("Error - registration failed: %s\n", error_message);
		break;
	case DD_ERROR_VERSION:
		printf("Error - version: %s\n", error_message);
		break;
	default:
		printf("Error - unknown error!\n");
		break;
	}
	dd_un_msg_t msg=err_dd_un_msg;
	write(pipe_fd[1], &msg, sizeof(dd_un_msg_t));
	fflush(stdout);
}
bool DoubleDeckerClient::init(char *_clientName, char *_brokerAddress, char *_keyPath)
{
	ULOG_INFO("Inizializing the Double-Decker-Client");
	ULOG_INFO("\t DD client name: '%s'",_clientName);
	ULOG_INFO("\t DD broker address: '%s'",_brokerAddress);
	ULOG_INFO("\t DD key to be used (path): '%s'",_keyPath);

	DoubleDeckerClient::clientName = _clientName;
	brokerAddress = _brokerAddress;
	keyPath = _keyPath;

	if(pipe(pipe_fd)!=0){
		ULOG_DBG_INFO("Failed opening pipe");
		return false;
	}

	pthread_mutex_init(&connected_mutex, NULL);
	//Start a new thread that waits for events
	pthread_t thread[1];
	pthread_create(&thread[0],NULL,loop,NULL);
#ifdef __x86_64__
	//the following function is not available on all platforms
	pthread_setname_np(thread[0],"DoubleDeckerClient");
#endif

	return true;
}

void *DoubleDeckerClient::loop(void *param)
{
	//ULOG_DBG_INFO("DoubleDeckerClient thread started");
	// create a ddactor
	client = dd_new(clientName, brokerAddress, keyPath, on_reg, on_discon, on_data, on_pub, on_error);
	
	dd_un_msg_t event;
	while(keep_looping)
	{
		// wait for event
		read(pipe_fd[0], &event, sizeof(dd_un_msg_t));
		
		//if control reach here, there is an event
		switch(event){
		case reg_dd_un_msg:
			pthread_mutex_lock(&connected_mutex);
			connected = true;
			pthread_mutex_unlock(&connected_mutex);
			ULOG_INFO("Succcessfully registered on the Double Decker network!");

			//Let's send all the messages stored in the list
			for(list<publish_t>::iterator m = messages.begin(); m != messages.end(); m++)
				publish(m->topic,m->message);
			break;
		case discon_dd_un_msg:
			pthread_mutex_lock(&connected_mutex);
			connected = false;
			pthread_mutex_unlock(&connected_mutex);
			ULOG_WARN("Connection with the Double Decker network has been lost!");	
			break;			
		case pub_dd_un_msg:
			ULOG_WARN("Received a 'publication' event. This event is ignored");
			break;
		case data_dd_un_msg:
			ULOG_WARN("Received a 'data' event. This event is ignored");
			break;
		case err_dd_un_msg:
			ULOG_ERR("Error while trying to connect to the Double Decker network.");
			keep_looping = false;
			break;
		default:
			ULOG_ERR("Unknown pipe data.");
			break;
		}
	} // - while (keep_looping) -
	return NULL;
}

void DoubleDeckerClient::terminate()
{
	ULOG_INFO("Stopping the Double Decker client");
	keep_looping = false;
	
	
	ULOG_INFO("Terminating DoubleDecker connection...");
	dd_destroy(&client);
	ULOG_INFO("DoubleDecker connection terminated");
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

	ULOG_INFO("Publishing on topic '%s'",topicToString(topic));
	ULOG_INFO("Publishing message '%s'", message);



	char *msg=strdup(message);
	int len = strlen(msg);
	dd_publish(client, topicToString(topic), msg, len);
	free(msg);
}

char *DoubleDeckerClient::topicToString(topic_t topic)
{
	switch(topic)
	{
		case FROG_DOMAIN_DESCRIPTION:
			return "frog:domain-description";
		default:
			assert(0 && "This is impossible!");
			return "";
	}
}
/*
void DoubleDeckerClient::sigalarm_handler(int sig)
{
	ULOG_ERR("Error while trying to connect to the Double Decker network!");
	ULOG_ERR("This situation is not handled by the code. Please reboot the orchestrator and check if the broker is running!");
	alarm(1);
}
*/
