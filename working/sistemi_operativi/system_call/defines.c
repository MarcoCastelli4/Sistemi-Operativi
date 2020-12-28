/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

int numcifre(int n){
   int i=0;
   
   while(n/10!=0){
    i++;
    n/=10;
   }
   
   return i+1;
}

void printMessage(message_sending message){
  printf("%d %s %s %s %d %d %d %s\n", message.id, message.message, message.idSender, message.idReceiver, message.DelS1, message.DelS2, message.DelS3, message.Type);
}
