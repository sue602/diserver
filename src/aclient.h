
#ifndef _CLIENT_H
#define _CLIENT_H

#include <string.h>
#include <stdio.h>

#include "alog.h"
#include "zmalloc.h"
#include "anet.h"

#define IP_STR_LEN 16
#define CUID_SIZE 32
#define CLIENT_DATA_SIZE 64
#define BUF_SIZE 1024

typedef struct yhclient {
	char cuid[CUID_SIZE];
	char os[8];

	char cip[IP_STR_LEN];
	int cport;

	int cellId[4];
	double lng;
	double lat;
	long long geohash;

	time_t ctime;
	time_t lastinteraction;

	int fd;
	char data[CLIENT_DATA_SIZE];
} aclient;

//typedef struct yhclientpool{
//	yhclient* cs;
//} yhclientPool;

aclient* createClient(int fd){
	aclient* c = (aclient*)zmalloc(sizeof(aclient));
	if(c == NULL) return NULL;
	c->fd = fd;

	anetNonBlock(NULL,fd);
	anetEnableTcpNoDelay(NULL,fd);
	anetKeepAlive(NULL, fd, 1);

	return c;
}

void freeClient(aclient * c){
	zfree(c);
}

void setKeyVal(aclient* c, char* key, char* value){
	printf("key:%s,val:%s\n", key, value);
	if(strcmp(key, "cuid")==0){
		memcpy(c->cuid, value, CUID_SIZE);
	} else if(strcmp(key, "mcc")==0){
		c->cellId[0] = atoi(value);	
	} else if(strcmp(key, "mnc")==0){
		c->cellId[1] = atoi(value);	
	} else if(strcmp(key, "lac")==0){
		c->cellId[2] = atoi(value);
	} else if(strcmp(key, "cid")==0){
		c->cellId[3] = atoi(value);
	}
}

void copyStr(char* dest, char* start, char* end){
	char * tmp = start;
	while(tmp != end){
		*dest++ = *tmp++;
	}
}

void processBuf(char* buff,int length, aclient* c){
	int i=0;
	aDebug("string before proc buff=%x: %d \n",buff,length);
	unsigned short dataLength = buff[0] << 8 | buff[1];
	aDebug("read length =%d  \n",dataLength);
	memcpy(c->data,buff+2,dataLength);
	c->data[dataLength+1] = '\0';
	aDebug("string before proc length=%d: %s \n",dataLength,c->data);
}


#endif

