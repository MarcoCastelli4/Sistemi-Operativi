/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

#define _SHARED_MEMORY_HH

#include <stdlib.h>

//crea un segmento di shared memory se non esiste;
//ritorna il shmid se va a buon fine, altrimenti termina il processo
int alloc_shared_memory(key_t shmKey, size_t size);

//attacca il segmento di shared memory all'indirizzo logico del processo chiamante
//ritorna un puntatore al segmento di memoria altrimenti termina il processo
void *get_shared_memory(int shmid, int shmflg);

//esegue il detach del segmento di shared memory dall'indirizzo logico del processo chiamante
//se fallisce termina il processo
void free_shared_memory(void *ptr_sh);

//rimuove il segmento di shared memory
//se fallisce termina il processo
void remove_shared_memory(int shmid);
