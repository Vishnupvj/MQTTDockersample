#include <sqlite3.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "MQTTAsync.h"
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientSub"
#define CLIENTID1    "ExampleClientSub1"
#define TOPIC       "log/#"
#define QOS         1
#define TIMEOUT     10000L

char* tok1;
char* tok2;
char* tok3;
int VT1;
int VT2;
MQTTAsync client;
MQTTAsync client1;

void stringparser(char* str)
{
    tok1 = strtok(str, ";");
    tok2 = strtok(NULL, ";");
    tok3 = strtok(NULL, ";");
}

void timeparser(char* str)
{
    char* t1;
    char* t2;
    t1 = strtok(str, ";");
    t2 = strtok(NULL, ";");
    
    VT1 = atoi(t1);
    VT2 = atoi(t2);
}


void onSend()
{
	printf("Publish Successful\n");
}

void onSendFailure()
{
	printf("Publish Failed");
}

int SQLInsert() {
    
    sqlite3 *db;
    char *err_msg = 0;
    char Msg[500];
    int rc = sqlite3_open("test.db", &db);
    time_t t;
	t = time(NULL);
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    sprintf(Msg,"CREATE TABLE IF NOT EXISTS Time(SensorName TEXT, SensorTime TEXT, Message TEXT, UtcTime LONG); INSERT INTO Time VALUES('%s','%s','%s',%ld)",tok1,tok2,tok3,t);
    rc = sqlite3_exec(db, Msg, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return 1;
    } 
    
    return 0;
}

int callback(void *, int, char **, char **);


int SQLRetrieve() {
    
    sqlite3 *db;
    char *err_msg = 0;
	char SQLquery[500];
    
    int rc = sqlite3_open("test.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    sprintf(SQLquery,"SELECT * FROM Time WHERE UtcTime >= %d AND UtcTime <= %d",VT1,VT2);  
    rc = sqlite3_exec(db, SQLquery, callback, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);
        
        return 1;
    } 
    
    return 0;
}

int callback(void *NotUsed, int argc, char **argv, 
                    char **azColName) {
    
    NotUsed = 0;
    char SentData[500];
    for (int i = 0; i < argc; i++) {

        sprintf(SentData,"%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;
        opts.onSuccess = onSend;
		opts.onFailure= onSendFailure;
        pubmsg.payload = SentData;
        pubmsg.payloadlen = strlen(pubmsg.payload);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        if ((rc = MQTTAsync_sendMessage(client1, "aloha", &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start sendMessage, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
    }
	
    
    printf("\n");
    
    return 0;
}

volatile MQTTAsync_token deliveredtoken;
int disc_finished = 0;
int subscribed = 0;
int finished = 0;
void connlost(void *context, char *cause)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        int rc;
        printf("\nConnection lost\n");
        printf("     cause: %s\n", cause);
        printf("Reconnecting\n");
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
            finished = 1;
        }
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
	if(strcmp(topicName,"log/put")==0)
	{
		stringparser(payloadptr);
		SQLInsert();
	}
    
	if(strcmp(topicName,"log/get")==0)
	{
		timeparser(payloadptr);
		SQLRetrieve();
	}
	
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}
void onDisconnect(void* context, MQTTAsync_successData* response)
{
        printf("Successful disconnection\n");
        disc_finished = 1;
}
void onSubscribe(void* context, MQTTAsync_successData* response)
{
        printf("Subscribe succeeded\n");
        subscribed = 1;
}
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Subscribe failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Connect failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
void onConnect(void* context, MQTTAsync_successData* response)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;
        printf("Successful connection\n");
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.context = client;
        deliveredtoken = 0;
        if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
}

void onConnect1(void* context, MQTTAsync_successData* response)
{
        printf("Connection Successful");
}


int main(int argc, char* argv[])
{
		setbuf(stdout,NULL);
		printf("C-Program started");
		sleep(5);
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
		MQTTAsync_connectOptions conn_opts1 = MQTTAsync_connectOptions_initializer;
        MQTTAsync_disconnectOptions disc_opts1 = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_message pubmsg1 = MQTTAsync_message_initializer;
        MQTTAsync_token token;
        int rc;
        int ch;
        MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
		
		MQTTAsync_create(&client1, ADDRESS, CLIENTID1, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(client1, NULL, connlost, msgarrvd, NULL);
        conn_opts1.keepAliveInterval = 20;
        conn_opts1.cleansession = 1;
        conn_opts1.onSuccess = onConnect1;
        conn_opts1.onFailure = onConnectFailure;
        conn_opts1.context = client1;
        if ((rc = MQTTAsync_connect(client1, &conn_opts1)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
                exit(EXIT_FAILURE);
		}
        while   (!subscribed)
                #if defined(WIN32) || defined(WIN64)
                        Sleep(100);
                #else
                        usleep(10000L);
                #endif
        if (finished)
                goto exit;
        do
        {
                ch = getchar();
        } while (ch!='Q' && ch != 'q');
        disc_opts.onSuccess = onDisconnect;
        if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start disconnect, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
        while   (!disc_finished)
                #if defined(WIN32) || defined(WIN64)
                        Sleep(100);
                #else
                        usleep(10000L);
                #endif
				
				
		exit:
        MQTTAsync_destroy(&client);
        return rc;
}
