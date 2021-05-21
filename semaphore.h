/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#define CREATION 0
#define ELIMINATION 1
//Mutex per bloccare processi che vogliono leggere quando non ci sono messaggi nuovi
#define REQUEST 2
#define DATAREADY 3
#define HACKLERRECEIVER 4
#define HACKLERSENDER 5
#define PIPE1WRITER 6
#define PIPE1READER 7
#define PIPE2WRITER 8
#define PIPE2READER 9
#define PIPE3WRITER 10
#define PIPE3READER 11
#define PIPE4WRITER 12
#define PIPE4READER 13
#define F10ACCESS 14

#define SEMNUMBER 15
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
