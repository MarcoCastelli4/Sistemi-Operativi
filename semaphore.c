/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include <stdio.h>
#include <stdlib.h>
#include "semaphore.h"
#include <errno.h>

void semOp (int semid, unsigned short sem_num, short sem_op) {
    int rc;
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
    while ((rc = semop(semid,&sop,1) == -1)) {
        if (errno != EINTR) {
            ErrExit("semop failed");
            break;
        }     
        // Interrupted system call handler ignore it
    }
}

int create_sem_set(int nSem) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(SKey, nSem, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        ErrExit("semget failed");

    // Initialize the semaphore set with semctl
    union semun arg;
    unsigned short values[] = {0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0};
    arg.array = values;

    // inserisci con setALL...
    if (semctl(semid,0,SETALL,arg)==-1)
        ErrExit("semctl SETALL failed");

    return semid;
}
