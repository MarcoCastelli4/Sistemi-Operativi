/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include <stdio.h>
#include <stdlib.h>
#include "semaphore.h"
#include <errno.h>

//effettuo l'operazione
void semOp (int semid, unsigned short sem_num, short sem_op) {
    int rc;
    //printf("Semaforo: %d, opreazione: %d\n", sem_num, sem_op);
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
    //ciclo necessario per fare in modo che se arriva un segnale durante che da EINTR esso non si interrompe
    while ((rc = semop(semid,&sop,1) == -1)) {
        if (errno != EINTR) {
            ErrExit("semop failed");
            break;
        }     
    }
}

int create_sem_set(int nSem) {
    //creo un set di nSem semafori
    int semid = semget(SKey, nSem, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        ErrExit("semget failed");

    //inizializzo i semafori 
    union semun arg;
    unsigned short values[] = {0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0};
    arg.array = values;

    //setto l'inizializzazione
    if (semctl(semid,0,SETALL,arg)==-1)
        ErrExit("semctl SETALL failed");

    return semid;
}
