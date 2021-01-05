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

#define F8 "OutputFiles/F8.csv"
#define F9 "OutputFiles/F9.csv"
#define F2 "OutputFiles/F2.csv"
#define F3 "OutputFiles/F3.csv"
#define F1 "OutputFiles/F1.csv"
#define F4 "OutputFiles/F4.csv"
#define F5 "OutputFiles/F5.csv"
#define F6 "OutputFiles/F6.csv"
#define F7 "InputFiles/F7.csv"
#define F7out "OutputFiles/F7_out.csv"
#define F0 "InputFiles/F0.csv"
#define TrafficInfo "Id;Message;Id Sender;Id Receiver;Time arrival;Time dept.\n"
#define TrafficInfoLength 57
#define MessageSendingHeader 54
#define ActionSendingHeader 23

typedef struct{
     int id;
     int delay;
     char* target;
     char* action;
} action;

typedef struct{
     int length;
     action* actions;
} action_group;

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

typedef struct{
     int length;
     message_sending* messages;
} message_group;

int numcifre(int );
void printMessage(message_sending );
void printAction(action );
void writeTraffic(char*, message_group*);
