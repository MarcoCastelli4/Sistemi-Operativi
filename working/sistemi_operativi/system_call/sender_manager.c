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

#include "defines.h"
#include "err_exit.h"

void writeF8(char[],int,int,int);
message_sending* carica_F0(char[]);

int main(void) {
  pid_t pidS1,pidS2,pidS3;
  pid_t waitPID;

  //genero processo S1
  pidS1=fork();
  if(pidS1==0){
    
    //inizializzo la struttura con la dimensione di un messaggio
    message_sending *messagges=carica_F0(F0);
    exit(0);
  } else if(pidS1 == -1){
    ErrExit("Fork");
  }

  //genero processo S2
  pidS2=fork();
  if(pidS2==0){
    //printf("Processo %d\n",getpid());
    exit(0);
  } else if(pidS2 == -1){
    ErrExit("Fork");
  }


  //genero processo S3
  pidS3=fork();
  if(pidS3==0){
    //printf("Processo %d\n",getpid());
    exit(0);
  } else if(pidS3 == -1){
    ErrExit("Fork");
  }


  //genero file8.csv 
  writeF8(F8,pidS1,pidS2,pidS3);



  /** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato=0;
	while((waitPID = wait(&stato)) > 0);
  exit(0);
}



//Funzione che mi genera il file F8 e scrive ogni riga
void writeF8(char nomeFile[],int pid1,int pid2,int pid3){

  //creo il file se è gia presente lo sovrascrivo
  int fp = open(nomeFile,O_CREAT | O_TRUNC| O_WRONLY, S_IRUSR | S_IWUSR ); 
  if (fp == -1)
    ErrExit("Open");
  
  //calcolo il numero totale di caratteri da scrivere nel buffer
  int n=25+numcifre(pid1)+numcifre(pid2)+numcifre(pid3);
  
  //inizializzo buffer delle dimenisoni corrette
  ssize_t bufferLength = sizeof(char)*n; 
  char* buffer=malloc(bufferLength);
  
  //converto i dati in stringa
  sprintf(buffer,"Sender Id;PID\nS1;%d\nS2;%d\nS3;%d\n",pid1,pid2,pid3);

  //scrivo sul file
  write(fp,buffer,bufferLength);
  close(fp);
  free(buffer);
}

message_sending* carica_F0(char nomeFile[]){

  // struct message_sending* tmp=malloc(sizeof(struct message_sending));

  /** apro il file */
  int fp = open(nomeFile, O_RDONLY);
  if (fp == -1)
    ErrExit("Open");

  /** utilizzo lseek per calcolarne le dimensioni */
  int fileSize = lseek(fp, (size_t)0, SEEK_END);
  if (fileSize == -1) { ErrExit("Lseek"); }

  /** riposiziono l'offset di lettura a inizio file */
  if(lseek(fp, (size_t)MessageSendingHeader*sizeof(char), SEEK_SET)	== -1){
    ErrExit("Lseek");
  }

  /** eseguo la lettura dal file al buffer */

  int bufferLength = fileSize/sizeof(char) - MessageSendingHeader;
  char buf[fileSize/sizeof(char)];
  if ((read(fp, buf, bufferLength*sizeof(char)) == -1)) { 
    ErrExit("Read");
  }

  char row[1000];
  int counterRow = 0;
  int rowNumber = 0;

  // Prendiamo il numero di righe per malloc
  for(int i=0; i<bufferLength; i++){
    if(buf[i] == '\n' || buf[i] == '\0' || i == bufferLength - 1){
      rowNumber++;
    } 
  }
  message_sending *messagges =  malloc(sizeof(message_sending)*(rowNumber+1));

  counterRow = 0;
  int messaggeNumber = 0;
  for(int i=0; i<bufferLength; i++){
    // Prendiamo le righe
    if(buf[i] != '\n' && buf[i] != '\0' && i != bufferLength - 1){
      row[counterRow] = buf[i];
      counterRow ++;
    } else {
      // Dalla riga otteniamo i segmenti
      row[counterRow] = '\0';

      int j = 0;
      int x = 0;
      // Per ognuno dei campi
      while(j<8){
        int tempOldValueX = x;
        int segmentLength = 0; 
        while(row[x] != ';' && row[x] != '\n' && row[x] != '\0' && row[x] != NULL){
          x++;
          segmentLength++;
        }
        x = tempOldValueX;
        char *segment = malloc(sizeof(char)*segmentLength);
        int c = 0;
        // Ottengo il campo
        while(row[x] != ';' && row[x] != '\n' && row[x] != '\0' && row[x] != NULL){
          segment[c] = row[x];
          c++;
          x++;
        }
        segment[c] = '\0';
        x++;
        // Inizializzo il messagge
        switch (j){
          case 0:
            messagges[messaggeNumber].id = atoi(segment);
            break;
          case 1:
            messagges[messaggeNumber].message = (char *)malloc(sizeof(segment));
            strcpy(messagges[messaggeNumber].message, segment);
            break;
          case 2:
            messagges[messaggeNumber].idSender = (char *)malloc(sizeof(segment));
            strcpy(messagges[messaggeNumber].idSender, segment);
            break;
          case 3:
            messagges[messaggeNumber].idReceiver = (char *)malloc(sizeof(segment));
            strcpy(messagges[messaggeNumber].idReceiver, segment);
            break;
          case 4:
            messagges[messaggeNumber].DelS1 = atoi(segment);
            break;
          case 5:
            messagges[messaggeNumber].DelS2 = atoi(segment);
            break;
          case 6:
            messagges[messaggeNumber].DelS3 = atoi(segment);
            break;
          case 7:
            messagges[messaggeNumber].Type = (char *)malloc(sizeof(segment));
            strcpy(messagges[messaggeNumber].Type, segment);
            break;
          default:
            break;
        }
        free(segment);
        j++;
      }
      printf("%d\t",messaggeNumber);
      printMessage(messagges[messaggeNumber]);
      // Resetto la riga
      strcpy(row, "");
      messaggeNumber ++;
      counterRow = 0;
    }
  }
  
  return messagges;
}



