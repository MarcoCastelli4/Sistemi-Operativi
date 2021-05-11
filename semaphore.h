/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#define CREATION 0
#define ELIMINATION 1
#define REQUEST 2
#define DATAREADY 3
#define HACKLERRECEIVER 4
#define HACKLERSENDER 5
#define SKey 01110011
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void semOp(int semid, unsigned short sem_num, short sem_op);
int create_sem_set(int nSem);
