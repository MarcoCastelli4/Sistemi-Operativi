/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once

#define F8 "OutputFiles/F8.csv"
#define F0 "InputFiles/F0.csv"
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
int numcifre(int );
void printMessage(message_sending );
