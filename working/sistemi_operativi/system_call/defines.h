/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once

#define F8 "OutputFiles/F8.csv"
#define F2 "OutputFiles/F2.csv"
#define F3 "OutputFiles/F3.csv"
#define F1 "OutputFiles/F1.csv"
#define F0 "InputFiles/F0.csv"
#define TrafficInfo "Id;Message;Id Sender;Id Receiver;Time arrival;Time dept.\n"
#define TrafficInfoLength 57
#define MessageSendingHeader 54
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
