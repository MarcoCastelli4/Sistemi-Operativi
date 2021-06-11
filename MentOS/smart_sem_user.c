///                MentOS, The Mentoring Operating system project
/// @file smart_sem_user.c
/// @brief Semaphore user-side implementation source code.
/// @copyright (c) 2019 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include "smart_sem_user.h"

#include "syscall.h"
#include "syscall_types.h"
#include "errno.h"
#include "debug.h"

int sem_create()
{
	int retval = 0;
	// carico i valori della system call nei registri (affinchè venga gestito dal kernel con assembly)
	DEFN_SYSCALL0(retval,__NR_sem_creat);
	// verifico che sia andata a buon fine
	if(retval<0){
		errno=-retval;
		retval=-1;
	}

    	dbg_print("sem_create() -> %d\n", retval);

	return retval;
}

int sem_destroy(int id)
{
	int retval = 0;
	// carico i valori della system call nei registri (affinchè venga gestito dal kernel con assembly)
	DEFN_SYSCALL1(retval,__NR_sem_destroy,id);
	if(retval<0){
		errno=-retval;
		retval=-1;
	}
    	dbg_print("sem_destroy(%d)\n", id);
	
	return retval;
}

int sem_init(int id)
{
	int retval = 0;
	
	DEFN_SYSCALL1(retval,__NR_sem_init,id);
	if(retval<0){
		errno=-retval;
		retval=-1;
	}

    	dbg_print("sem_init(%d)\n", id);


	return retval;
}

int sem_acquire(int id)
{
	int retval;
	
    dbg_print("sem_acquire(%d)\n", id);

	do {
	// provo ad acquisire il semaforo e se non ci riesco mi fermo
	DEFN_SYSCALL1(retval,__NR_sem_try_acquire,id);
	if(retval<0){
		errno=-retval;
		retval=-1;
	}
	} while (retval != 1);

	return retval;
}

int sem_release(int id)
{
	int retval = 0;
	
    dbg_print("sem_release(%d)\n", id);

	DEFN_SYSCALL1(retval,__NR_sem_release,id);
	if(retval<0){
		errno=-retval;
		retval=-1;
	}
	
	return retval;
}
