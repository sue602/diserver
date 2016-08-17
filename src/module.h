/*
 * module.h
 *
 *  Created on: Aug 17, 2016
 *      Author: ltzd
 */

#ifndef MODULE_H_
#define MODULE_H_

#define MAX_MODULE_TYPE 128

typedef void * (*dl_create)(void);
typedef int (*dl_init)(void * inst,const char * parm);
typedef void (*dl_release)(void * inst);
typedef void (*dl_signal)(void * inst, int signal);

struct module {
	const char * name;
	void * module;
	dl_create create;
	dl_init init;
	dl_release release;
	dl_signal signal;
};

void * open_module(const char * name);

void init_modules();

#endif /* MODULE_H_ */
