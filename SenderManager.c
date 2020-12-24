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

#define F8 "F8.csv"
#define F0 "F0.csv"

void writeF8(char[],int,int,int);
int numcifre(int );
struct message_sending* carica_F0(char[]);

int main(void) {
pid_t pidS1,pidS2,pidS3;
pid_t waitPID;

//genero struttura del messaggio
 struct{
   int id;
   char* message;
   char* idSender;
   char* idReceiver;
   int DelS1;
   int DelS2;
   int DelS3;
   char* Type;
 }message_sending;

//genero processo S1
pidS1=fork();


if(pidS1==0){
  
  //inizializzo la struttura con la dimensione di un messaggio
  struct message_sending *messagges=carica_F0(F0);
  
  exit(0);
}

//genero processo S2
pidS2=fork();
if(pidS2==0){
 // printf("Processo %d\n",getpid());
  exit(0);
}

//genero processo S3
pidS3=fork();
if(pidS3==0){
 // printf("Processo %d\n",getpid());
  exit(0);
}

//genero file8.csv 
writeF8(F8,pidS1,pidS2,pidS3);



/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato=0;
	while((waitPID = wait(&stato)) > 0);
  exit(0);
  }


int numcifre(int n)
{
   int i=0;
   
   while(n/10!=0){
    i++;
    n/=10;
   }
   
   return i+1;
}
//Funzione che mi genera il file F8 e scrive ogni riga
void writeF8(char nomeFile[],int pid1,int pid2,int pid3){

  //creo il file se è gia presente lo sovrascrivo
  int fp = open(nomeFile,O_CREAT | O_TRUNC| O_WRONLY, S_IRUSR | S_IWUSR ); 
  if (fp == -1)
        printf("Errore apertura file");
  
    //calcolo il numero totale di caratteri da scrivere nel buffer
    int n=25+numcifre(pid1)+numcifre(pid2)+numcifre(pid3);
    
    //inizializzo buffer delle dimenisoni corrette
    char* buffer=malloc(sizeof(char)*n);
    
    //converto i dati in stringa
    sprintf(buffer,"Sender Id;PID\nS1;%d\nS2;%d\nS3;%d\n",pid1,pid2,pid3);

    //scrivo sul file
     write(fp,buffer,sizeof(char)*n);
      close(fp);
}

struct message_sending* carica_F0(char nomeFile[]){
  
 // struct message_sending* tmp=malloc(sizeof(struct message_sending));

  /** apro il file */
	int fp = open(nomeFile, O_RDONLY);
	if (fp == -1) { perror("fd"); exit(1); }

	/** utilizzo lseek per calcolarne le dimensioni */
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { perror("lseek"); exit(1); }
	
	/** riposiziono l'offset di lettura a inizio file */
	lseek(fp, (size_t)0, SEEK_SET);	

	/** eseguo la lettura dal file al buffer */

  char buf[1000];
	if ((read(fp, buf, fileSize) == -1)) { perror("read"); exit(1); }
	
  printf("%s",buf);
 // return tmp;
  return 0;
}



