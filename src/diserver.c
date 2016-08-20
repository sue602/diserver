/*
 ============================================================================
 Name        : diserver.c
 Author      : vincent
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "common.h"
#include "module.h"
#include "aclient.h"
#include "anet.h"
#include "ae.h"
#include "alog.h"

#define NOTUSED(V) ((void) V)
#define MAX_CLIENTS 10000
#define BINDADDR_MAX 16
#define BUF_SIZE 1024
#define PORT 8001
#define CONFIG_DEFAULT_TCP_BACKLOG       511     /* TCP listen backlog */
char buffer[BUF_SIZE];

//清除资源
void freeResource(aeEventLoop *el, int fd, void* c){
	aDebug("free resource...\n");
	aeDeleteFileEvent(el, fd, AE_READABLE);
	aeDeleteFileEvent(el, fd, AE_WRITABLE);
	close(fd);
	if(c != NULL) freeClient(c);
}

//写函数句柄
void writeTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask){
	aDebug("into writeTcpHandler\n");
	int writen;
	NOTUSED(mask);
	aclient* c = (aclient*) privdata;

	writen = write(fd, c->data, strlen(c->data));

	if(writen == -1){
		if(errno == EAGAIN){
			writen = 0;
		} else {
			aDebug("write err: %s\n", strerror(errno));
		}
	} else {
		aDebug("sent : %d bytes\n", writen);
	}

	//freeResource(el, fd, NULL);
	aeDeleteFileEvent(el, fd, AE_WRITABLE);
}

//读函数句柄
void readTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask){
	aDebug("into readTcpHandler\n");
	int readlen, res;
	aclient* c = (aclient*)privdata;
	NOTUSED(mask);

	readlen = read(fd, buffer, BUF_SIZE);
	if(readlen == -1){
		if(errno == EAGAIN){
			readlen = 0;
		} else {
			aDebug("close conn: %s\n", strerror(errno));
			freeResource(el, fd, c);
		}
	} else if(readlen == 0){
		aDebug("Client closed connection\n");
		freeResource(el, fd, c);
	}

	processBuf(buffer, c);

	res = aeCreateFileEvent(el, fd, AE_WRITABLE, writeTcpHandler, c);
	if(res != -1){
		aDebug("add writable file event to fd %d\n", fd);
	}
}

//accept函数句柄
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask){
	int cport, cfd, res;
	char cip[IP_STR_LEN];
	NOTUSED(el);
	NOTUSED(mask);
	NOTUSED(privdata);

	cfd = anetTcpAccept(buffer, fd, cip, sizeof(cip), &cport);
	if (cfd == -1){
		aDebug("accept err\n");
		freeResource(el, cfd, NULL);
		return ;
	}
	aDebug("accepted %d\n", cfd);

	aclient* yhc = createClient(cfd);
	if(yhc == NULL){
		aDebug("create aclient err\n");
		free(cfd);
		return ;
	}
	yhc->cport = cport;
	memcpy(yhc->cip, cip, sizeof(cip));

	res = aeCreateFileEvent(el, cfd, AE_READABLE, readTcpHandler, yhc);
	if(res == -1){
		freeResource(el, cfd, yhc);
	} else {
		aDebug("add read file event to fd: %d\n", cfd);
	}

	return ;
}

//例行任务
int cronJob(aeEventLoop *el, long long id, void *clientData){
	//do cron jobs here
	void * data = (void*) easy_malloc(10);
	memset(data,0,10);
	memcpy(data,"hello",5);
	printf("\ndata =%p , string =%s\n",data,data);
//	printf("malloc_usable_size = %d \n",malloc_size(data-sizeof(size_t)));
	void * newdata = (void*) easy_realloc(data,20);
	printf("new data =%p oldptr=%p, string =%s\n",newdata,data,newdata);
	data = newdata;
	easy_free(data);
	printf("one loop finish============\n");
	//if(clientData) aDebug("%d event: %s\n",id, clientData);
//	common_fini();
	return 5000;	//5000ms后继续
//	return -1;	//no more
}

int main(){
	aeEventLoop *el;
	int listenfd, port=PORT, res, connfd;
	char * cronTips = "yhserver is running ...";
	char neterr[128] = {0};

	el = aeCreateEventLoop(MAX_CLIENTS);
	aDebug("event loop created...\n");

	listenfd = anetTcpServer(neterr,port,NULL,CONFIG_DEFAULT_TCP_BACKLOG);

	res = aeCreateFileEvent(el, listenfd, AE_READABLE, acceptTcpHandler,NULL);
	if(res == AE_ERR){
		freeResource(el, listenfd, NULL);
		aDebug("AE_ERR: create file event error\n");
		exit(1);
	} else {
		aDebug("file event added:%d\n", listenfd);
	}
	common_init();
	if(aeCreateTimeEvent(el, 1000, cronJob, cronTips, NULL) == AE_ERR) {
		aDebug("create time event error:\n");
		exit(2);
	}
	char path[128]={0};
	getcwd(path,120);
	printf("current path=%s \n",path);
	init_modules();
	open_module("game");

	aeMain(el);
	aeDeleteEventLoop(el);

	return EXIT_SUCCESS;
}
