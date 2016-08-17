/*
 * module.c
 *
 *  Created on: Aug 17, 2016
 *      Author: ltzd
 */


#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "zmalloc.h"
#include "spinlock.h"
#include "module.h"

struct modules {
	int count;
	struct spinlock lock;
	const char * path;
	struct module m[MAX_MODULE_TYPE];
};

static struct modules * M = NULL;

static int _open_sym(struct module *mod)
{
	size_t name_size = strlen(mod->name);
	char tmp[name_size + 9]; // create/init/release/signal , longest name is release (7)
	memcpy(tmp, mod->name, name_size);
	strcpy(tmp+name_size, "_create");
	mod->create = dlsym(mod->module, tmp);
	strcpy(tmp+name_size, "_init");
	mod->init = dlsym(mod->module, tmp);
	strcpy(tmp+name_size, "_release");
	mod->release = dlsym(mod->module, tmp);
	strcpy(tmp+name_size, "_signal");
	mod->signal = dlsym(mod->module, tmp);

	return mod->init == NULL;
}

static struct module * _query(const char * name)
{
	int i;
	for (i=0;i<M->count;i++) {
		if (strcmp(M->m[i].name,name)==0) {
			return &M->m[i];
		}
	}
	return NULL;
}

void * open_module(const char * name)
{
	struct module * result = _query(name);
	if (result)
		return result;

	SPIN_LOCK(M)

	result = _query(name); // double check

	if (result == NULL && M->count < MAX_MODULE_TYPE) {
		int index = M->count;
		char path[80]={0};
		char fullpath[128]={0};
		getcwd(path,80-1);
		sprintf(fullpath,"%s/Debug/%s.so",path,name);
		void * dl = dlopen(fullpath, RTLD_NOW | RTLD_GLOBAL);
		if (dl) {
			M->m[index].name = name;
			M->m[index].module = dl;

			if (_open_sym(&M->m[index]) == 0) {
				//M->m[index].name = skynet_strdup(name);
				M->count ++;
				result = &M->m[index];
			}
		}
		else
		{
			printf("open module=%s failed\n",name);
		}
	}

	SPIN_UNLOCK(M)

	return result;
}

void init_modules()
{
	struct modules *m = zmalloc(sizeof(*m));
	m->count = 0;

	SPIN_INIT(m)

	M = m;
}
