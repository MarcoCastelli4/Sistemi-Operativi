/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#define RECEIVER_READY 0
#define SENDER_READY 1
// Pos0: RECEIVER_READY; POS1: SENDER_READY

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void semOp(int semid, unsigned short sem_num, short sem_op);
int create_sem_set(int nSem);