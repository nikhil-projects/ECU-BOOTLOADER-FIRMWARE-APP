#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../libs/mqtt/MQTTClient.h"

#define ADDRESS		"tcp://tt.local.com:8130"
#define UserName 	"user"
#define Password 	"passward"
#define CLIENTID    	"ExampleClientSub"
#define TOPIC       	"+/NNNMMM/+/+"
#define PAYLOAD     	"Hello World!"
#define QOS         	2
#define TIMEOUT     	10000L

volatile MQTTClient_deliveryToken deliveredtoken;
int mqtt_init();
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    	int i;
    	char* payloadptr;
    	printf("Message arrived\n");
    	printf("     topic: %s\n", topicName);
    	printf("   message: ");
    	payloadptr = message->payload;
	printf("Debug message %s\n", (char *)message->payload);
	/* File pointer */
	FILE *fp = NULL;
	/* Return status */
	char ret_status = 0;
	do{
		/* Open file to read/write data on file */
		fp = fopen("/home/pi/Data.txt", "a+");
		if (fp == NULL){
			printf("Fail to open file in write mode %s in func %s ,line %d ", "/home/pi/debug.txt", __func__, __LINE__);
			fclose(fp);
			ret_status = -1;
			break;
		}	
		/* Write data to file from  buffer */
		fprintf(fp, "%s\n", (char *)message->payload);
	}while(0);
	/* Close file ptr */
	if (fp != NULL)
		fclose(fp);

    //putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]){
	/*func  return status */
	int ret;
	/* loop to monitor data received on mqtt server */
	do{
		/* func to write mqtt client data in the storage file */ 
		ret = mqtt_init();
		if (ret ==  0){
			printf("Fail to read mqtt server data on server \n");
		}
		sleep(1);
	}while(ret);
	
	return ret;
}

int mqtt_init(){
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
			conn_opts.password = Password;
			conn_opts.username = UserName;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
 
sleep(30);
   //do{
   //     ch = getchar();
    //} while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
