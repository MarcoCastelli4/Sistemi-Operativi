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
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "err_exit.h"
#include "semaphore.h"
#include <signal.h>
#include <string.h>      
#include <ctype.h>      
#define print_log(f_, ...) printf((f_), ##__VA_ARGS__), printf("PID: %d, PARENT_PID: %d\n%s\n%s:%d\n\n",getpid(), getppid(), timestamp(),__FILE__,__LINE__)

//nomi simbolici dei file su cui agisco
#define F8 "OutputFiles/F8.csv"
#define F9 "OutputFiles/F9.csv"
#define F10 "OutputFiles/F10.csv"
#define F2 "OutputFiles/F2.csv"
#define F3 "OutputFiles/F3.csv"
#define F1 "OutputFiles/F1.csv"
#define F4 "OutputFiles/F4.csv"
#define F5 "OutputFiles/F5.csv"
#define F6 "OutputFiles/F6.csv"
#define F7out "OutputFiles/F7_out.csv"

#define TrafficInfo "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n"
#define HacklerInfo "ID;Delay;Target;Action\n"

//dimensioni classiche mi servono (stringa titoli dei messaggi, stringa titoli delle sole intestazioni dei messaggi, stringa titolo delle sole intestazioni dei messaggi di header)
#define TrafficInfoLength 57
#define F10Header 47
#define MessageSendingHeader 54
#define ActionSendingHeader 23
#define ReceiverPIDHeader 16
#define SenderPIDHeader 13
#define QKey 01110001
#define MKey 01101101
#define FIFO "OutputFiles/my_fifo.txt"

//struttura IPC history
typedef struct
{
  char *ipc;
  char *idKey;
  char *creator;
  char *creationTime;
  char *destructionTime;
} IPC_history;

//struttura ..
typedef struct
{
  int length;
  IPC_history *histories;
} IPC_history_group;

//struttura PIDS
typedef struct
{
  int isFather;
  ssize_t pid;
  ssize_t pid_parent;
} pid_manager;

//struttura che contiene tutti i pid dei figli generati--> nipoti, pronipoti, ecc
typedef struct
{
  int length;
  pid_manager *pids;
} pids_manager;

//struttura messaggio di hackler
typedef struct
{
  int id;
  int delay;
  char *target;
  char *action;
} action;

//struttura che contiene l'array dei messaggi di hackler e la rispettiva lunghezza
typedef struct
{
  int length;
  action *actions;
} action_group;

//struttura messaggio
typedef struct
{
  int id;
  char message[50];
  char idSender[3];
  char idReceiver[3];
  int DelS1;
  int DelS2;
  int DelS3;
  char Type[5];
  pid_t PidManager;
} message_sending;

//struttura che contiene l'array dei messaggi e la rispettiva lunghezza
typedef struct
{
  int length;
  message_sending *messages;
} message_group;

//struttura x messaggio inviato attraverso shared memory
struct request_shared_memory
{
  message_sending message;
  key_t SHMKey;
};

//array di messaggi x shared memory
typedef struct
{
  int cursorEnd;
  int cursorStart;
  message_sending messages[10];
} shared_memory_messages;

//struttura per messaggio inviato attraverso message queue
struct message_queue
{
  long mtype;
  message_sending message;
};




char * timestamp();
char *toString(message_sending message);

int numcifre(int);
void printInfoMessage(int semID,message_sending message, struct tm timeArrival, char file[]);
void printIntestazione(char FILE[]);
int stringLenght(message_sending message);

//funzioni usate per scrivere su F10
void appendInF10(char * buffer, ssize_t bufferLength,int);
void completeInF10(char * searchBuffer); 
void writeF10Header();
void initSignalChild(void (*handler)(int));
void initSignalMedium(void (*handler)(int));
void initSignalFather(void (*handler)(int));
