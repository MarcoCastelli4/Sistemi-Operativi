#include "defines.h"

void writeF9(int, int, int);
int MSQID = -1;

int main(int argc, char *argv[])
{
	pid_t pidR1, pidR2, pidR3;
	pid_t waitPID;

	//creo semaforo, sarà lo stesso del sender
	int semID = semget(SKey,2,IPC_CREAT | S_IRUSR | S_IWUSR);
     //finchè il sender non ha creato le IPC aspetto
	semOp(semID, CREATION, -1);
	
	//Accedo alla MessageQueue
	int MSQID = msgget(QKey, S_IRUSR | S_IWUSR);
	if (MSQID == -1){
		ErrExit("Message queue failed");
	}
	
	//genero processo R1
	pidR1 = fork();
	if (pidR1 == 0)
	{
		//stampo intestazione messaggio
		printIntestazione(F6);
	
		//leggo dalla coda
		listenMSQ(MSQID,"R1");
	
		//addormento per 2 secondo il processo
		sleep(1);
		//termino il processo
		exit(0);
	}	 
	
	else if (pidR1 == -1)
	{
		ErrExit("Fork");
	}

	//genero processo S2
	pidR2 = fork();
	if (pidR2 == 0)
	{
		
		//stampo intestazione messaggio
		printIntestazione(F5);

		//leggo dalla coda
		listenMSQ(MSQID,"R2");
		
		//addormento per 2 secondo il processo
		sleep(2);
		//termino il processo
		exit(0);
	}
	else if (pidR2 == -1)
	{
		ErrExit("Fork");
	}

	//genero processo S3
	pidR3 = fork();
	if (pidR3 == 0)
	{
		
		//stampo intestazione messaggio
		printIntestazione(F4);

		//leggo dalla coda
		listenMSQ(MSQID,"R3");

		//addormento per 2 secondo il processo
		sleep(3);
		//termino il processo
		exit(0);
	}
	else if (pidR3 == -1)
	{
		ErrExit("Fork");
	}

	//genero file F9
	writeF9(pidR1, pidR2, pidR3);

	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0);

	//dico al sender che può eliminare le IPC
	semOp(semID, ELIMINATION, 1);
	//termino il processo padre
	exit(0);
	return (0);
}

//Funzione che mi genera il file F9 e scrive ogni riga
void writeF9(int pid1, int pid2, int pid3)
{

	//creo il file se è gia presente lo sovrascrivo
	int fp = open(F9, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer
	int n = 28 + numcifre(pid1) + numcifre(pid2) + numcifre(pid3);

	//inizializzo buffer delle dimenisoni corrette
	ssize_t bufferLength = sizeof(char) * n;
	char *buffer = malloc(bufferLength);

	//converto i dati in stringa
	sprintf(buffer, "Receiver Id;PID\nR1;%d\nR2;%d\nR3;%d\n", pid1, pid2, pid3);

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}

