/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>

#pragma once
//creation viene messo alzato da sender quando ha creto le IPC, di conseguenza il receiver aspetta fino a che non si alza
#define CREATION 0
//non ha molto senso...
#define ELIMINATION 1
//mi avvisa quando è arrivato un messaggio per la FIFO
#define ACCESSTOFIFO 2
//viene abbassato dopo la scrittura di un messaggio sulle IPC, e alzato quando il processo giusto legge
#define DATAREADY 3
//servono per fare in modo che hackler parta dopo che il S,R generano i file F8,F9
#define HACKLERRECEIVER 4
#define HACKLERSENDER 5
//per gestire scrittura e lettura dalle PIPE, mangio writer quando devo scrivere e poi alzo reader per fare leggere
#define PIPE1WRITER 6
#define PIPE1READER 7
#define PIPE2WRITER 8
#define PIPE2READER 9
#define PIPE3WRITER 10
#define PIPE3READER 11
#define PIPE4WRITER 12
#define PIPE4READER 13

//...
#define INFOMESSAGEFILE 14
//mi avvisa quando è arrivato un messaggio per la Q
#define ACCESSTOQ 15
//mi avvisa quando è arrivato un messaggio per la SH
#define ACCESSTOSH 16

//numero dei semafori
#define SEMNUMBER 17
//chiave dei semafori
#define SKey 01110011

//struttura
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

//funzioni disponibili
void semOp(int semid, unsigned short sem_num, short sem_op);
int create_sem_set(int nSem);
