#include "defines.h"


void writeF9(int, int, int);
int MSQID = -1;
int main(int argc, char *argv[])
{
	pid_t pidR1, pidR2, pidR3;
	pid_t waitPID;

	//Inizializzo il semaforo e attendo
	int semID = create_sem_set(2);
	
	// Creo la message queue
	MSQID = msgget(QKey, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (MSQID == -1){
		ErrExit("Message queue failed");
	}
	semOp(semID, RECEIVER_READY, 1);

	//genero processo R1
	pidR1 = fork();
	if (pidR1 == 0)
	{
		struct mymsg messaggio;
		size_t len = sizeof(struct mymsg) - sizeof(long);
		if (msgrcv(MSQID, &messaggio, len, 0, 0) == -1)
		{
			ErrExit("msgrcv failed");
		}
		printf("\nMessaggio ricevuto: %s", messaggio.mtext);
		//scrivo sul file F2
		writeTraffic(F6, NULL);
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
		//scrivo sul file F2
		writeTraffic(F5, NULL);
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
		//scrivo sul file F2
		writeTraffic(F4, NULL);
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

	//Chiusura della msgQueue
	if(msgctl(MSQID,IPC_RMID,NULL)==-1){
		ErrExit("msfctl falied");
	}
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
