#include "defines.h"
#include "shared_memory.h"

void writeF9(int, int, int);
void listen(int, int, int, char[]);
int MSQID = -1;
int SHMID = -1;
int semID = -1;
struct request_shared_memory *request_shared_memory;

int main(int argc, char *argv[])
{
	signal(SIGINT, sigHandler);
	pid_t pidR1, pidR2, pidR3;
	pid_t waitPID;
	//creo semaforo, sarà lo stesso del sender
	semID = semget(SKey, 6, IPC_CREAT | S_IRUSR | S_IWUSR);
	//finchè il sender non ha creato le IPC aspetto
	semOp(semID, CREATION, -1);
	//Accedo alla
	SHMID = alloc_shared_memory(MKey, sizeof(struct request_shared_memory));

	//Accedo alla MessageQueue
	MSQID = msgget(QKey, S_IRUSR | S_IWUSR);
	if (MSQID == -1)
	{
		ErrExit("Message queue failed");
	}

	request_shared_memory = get_shared_memory(SHMID, 0);

	//genero processo R1
	pidR1 = fork();
	if (pidR1 == 0)
	{
		//stampo intestazione messaggio
		printIntestazione(F6);

		//leggo dalla coda
		listen(MSQID, SHMID, semID, "R1");

		
		printf("MORTO R1\n");
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
		listen(MSQID, SHMID, semID, "R2");

		
		//termino il processo
		printf("MORTO R2\n");
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
		listen(MSQID, SHMID, semID, "R3");

		
		//termino il processo
		printf("MORTO R3\n");
		exit(0);
	}
	else if (pidR3 == -1)
	{
		ErrExit("Fork");
	}

	//genero file F9
	writeF9(pidR1, pidR2, pidR3);
	semOp(semID, HACKLERRECEIVER, 1);

	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0);

	//dico al sender che può eliminare le IPC
	semOp(semID, ELIMINATION, 1);

	//Chiusura della msgQueue
	if (MSQID != -1 && msgctl(MSQID, IPC_RMID, NULL) == -1)
	{
		ErrExit("msgrmv failed");
	}

	//Eliminazione memoria condivisa
	if(SHMID != -1){
		free_shared_memory(request_shared_memory);
		remove_shared_memory(SHMID);
	}

	//Eliminazione semagori
	if (semID != -1 && semctl(semID, 0, IPC_RMID, 0) == -1)
	{
		ErrExit("semrmv failed");
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


void listen(int MSQID, int SHMID, int semID, char processo[])
{
	struct message_queue messaggio;
	time_t now = time(NULL);

	size_t mSize = sizeof(struct message_queue) - sizeof(long);
	while (1)
	{
		//genero tempo attuale
		struct tm timeArrival = *localtime(&now);
		//-------------------------------------------------- BLOCCO MESSAGE QUEUE --------------------------------------------------
		//prelevo il messaggio senza aspettare
   			 msgrcv(MSQID, &messaggio, mSize, 0, IPC_NOWAIT);
		
			//sei nel processo receiver corretto?
			if (strcmp(processo, messaggio.message.idReceiver) == 0)
			{
				printf("\nMsg received: %s", toString(messaggio.message));
				if (strcmp(messaggio.message.idReceiver, "R1") == 0)
				{
					//dormi
					sleep(messaggio.message.DelS1);
					//stampa le info sul tuo file
					printInfoMessage(messaggio.message, timeArrival, F6);
				}

				else if (strcmp(messaggio.message.idReceiver, "R2") == 0)
				{
					sleep(messaggio.message.DelS2);
					printInfoMessage(messaggio.message, timeArrival, F5);
				}

				else if (strcmp(messaggio.message.idReceiver, "R3") == 0)
				{
					sleep(messaggio.message.DelS3);
					printInfoMessage(messaggio.message, timeArrival, F4);
				}
			}
			//ho letto il messaggio che non è per me, quindi lo rimetto in coda
			else if (msgsnd(MSQID, &messaggio, mSize, 0) == -1)
				ErrExit("re-msgsnd failed");
		//semOp(semID, REQUEST, -1);
		//-------------------------------------------------- FINE BLOCCO MESSAGE QUEUE --------------------------------------------------
		//-------------------------------------------------- BLOCCO SHARED MEMORY --------------------------------------------------
		
		
		semOp(semID, REQUEST, -1);
		
		//sei nel receiver corretto?
		
		if (strcmp(processo, request_shared_memory->message.idReceiver) == 0)
			{
				printf("%s - %s",processo,request_shared_memory->message.idReceiver);

				if (strcmp(request_shared_memory->message.idReceiver, "R1") == 0)
				{
					printf("R1 ricevuto: %s\n",toString(request_shared_memory->message));
					//dormi
					sleep(request_shared_memory->message.DelS1);
					//stampa le info sul tuo file
					printInfoMessage(request_shared_memory->message, timeArrival, F6);

				} else if (strcmp(request_shared_memory->message.idReceiver, "R2") == 0)
				{
					printf("R2 ricevuto: %s\n",toString(request_shared_memory->message));
					sleep(request_shared_memory->message.DelS2);
					printInfoMessage(request_shared_memory->message, timeArrival, F5);

				} else if (strcmp(request_shared_memory->message.idReceiver, "R3") == 0)
				{
					printf("R3 ricevuto: %s\n",toString(request_shared_memory->message));
					sleep(request_shared_memory->message.DelS3);
					printInfoMessage(request_shared_memory->message, timeArrival, F4);
				} 
				semOp(semID, DATAREADY, 1);
			}
		
		//non sono nel receiver corretto
		else 
			semOp(semID, REQUEST, 1);

		
		//-------------------------------------------------- FINE BLOCCO SHARED MEMORY --------------------------------------------------
	}
}
