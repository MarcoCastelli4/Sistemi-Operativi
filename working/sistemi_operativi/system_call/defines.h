/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "err_exit.h"

//nomi simbolici dei file su cui agisco
#define F8 "OutputFiles/F8.csv"
#define F9 "OutputFiles/F9.csv"
#define F2 "OutputFiles/F2.csv"
#define F3 "OutputFiles/F3.csv"
#define F1 "OutputFiles/F1.csv"
#define F4 "OutputFiles/F4.csv"
#define F5 "OutputFiles/F5.csv"
#define F6 "OutputFiles/F6.csv"
#define F7out "OutputFiles/F7_out.csv"

#define TrafficInfo "Id;Message;Id Sender;Id Receiver;Time arrival;Time dept.\n"
#define HacklerInfo "Id;Delay;Target;Action\n"

//dimensioni classiche mi servono (stringa titoli dei messaggi, stringa titoli delle sole intestazioni dei messaggi, stringa titolo delle sole intestazioni dei messaggi di header)
#define TrafficInfoLength 57
#define MessageSendingHeader 54
#define ActionSendingHeader 23
#define SKey 01110011
#define QKey 01110001
#define MKey 01101101
#define FIFO "OutputFiles/my_fifo.txt"

//struttura messaggio di hackler
typedef struct{
     int id;
     int delay;
     char* target;
     char* action;
} action;

//struttura che contiene l'array dei messaggi di hackler e la rispettiva lunghezza
typedef struct{
     int length;
     action* actions;
} action_group;

//struttura messaggio
typedef struct{
     int id;
     char* message;
     char* idSender;
     char* idReceiver;
     int DelS1;
     int DelS2;
     int DelS3;
     char* Type;
} message_sending;

//struttura che contiene l'array dei messaggi e la rispettiva lunghezza
typedef struct{
     int length;
     message_sending* messages;
} message_group;

struct message_queue{
     long mtype;
     message_sending message;
};

int stringLenght(message_sending message);
char* toString(message_sending message);
int numcifre(int );
void printMessage(message_sending );
void printAction(action );
void writeTraffic(char*, message_group*);
