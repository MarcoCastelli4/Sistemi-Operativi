#include "defines.h"
#include "shared_memory.h"

void writeF9(int, int, int);
void listen(int, int, int, char[]);
int MSQID = -1;
int SHMID = -1;
int semID = -1;
int pipe3[2];
int pipe4[2];
pids_manager *pids = NULL;
struct request_shared_memory *request_shared_memory;
void deliverMessage(message_sending, char []);
// Uccisione ricorsiva
void recursiveKill(pid_t pid){
	for(int i=0; i<pids->length; i++){
		if(pids->pids[i].pid_parent == pid){
			recursiveKill(pids->pids[i].pid);
		}
	}
	kill(pid,SIGTERM);
}
void sigHandlerReceiver(int sig){
	if(sig == SIGINT){
		recursiveKill(getpid());
		exit(0);
	} else if(sig == SIGQUIT){
		print_log("SIGNAL SIGHUP received\n");
		for(int i=0; i<pids->length; i++){
			if(pids->pids[i].pid_parent == getpid()){
				pid_t childTemp = fork();
				if(childTemp == 0){
					kill(pids->pids[i].pid,SIGSTOP);
					sleep(5);
					kill(pids->pids[i].pid,SIGCONT);
					exit(0);
				} else if (childTemp == -1){
					ErrExit("Fork");
				}
			}
		}
	}
}
 

int main(int argc, char *argv[]){
	pid_t pidR1, pidR2, pidR3;
	pid_t waitPID;
	pids = malloc(sizeof(pids_manager));
	pid_manager *pids_list = malloc(sizeof(pid_manager) * (15));
	pids->length = 0;
	pids->pids = pids_list;


	//creo semaforo, sarà lo stesso del sender
	semID = semget(SKey, SEMNUMBER, IPC_CREAT | S_IRUSR | S_IWUSR);
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

	// checking if PIPE successed
	int resPipe3 = pipe(pipe3);
	if (resPipe3 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale --> TimeDeparture
	time_t now = time(NULL);
	struct tm TimeDeparture = *localtime(&now);

	//calcolo la dimensione della riga da scrivere
	ssize_t bufferLength = (sizeof("PIPE3") +numcifre(resPipe3) +  sizeof("SM") + 12 * sizeof(char));
	char *string = malloc(bufferLength);

	//mi salvo tutta la stringa
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;;\n", "PIPE3",resPipe3, "RM", TimeDeparture.tm_hour, TimeDeparture.tm_min, TimeDeparture.tm_sec);
	
	appendInF10(string, bufferLength);

	// checking if PIPE successed
	int resPipe4 = pipe(pipe4);
	if (resPipe4 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale --> TimeDeparture
	now = time(NULL);
	TimeDeparture = *localtime(&now);

	//calcolo la dimensione della riga da scrivere
	bufferLength = (sizeof("PIPE2") +numcifre(resPipe4) +  sizeof("SM") + 12 * sizeof(char));
	string = (char *) malloc(bufferLength);

	//mi salvo tutta la stringa
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;;\n",  "PIPE4",resPipe4, "RM", TimeDeparture.tm_hour, TimeDeparture.tm_min, TimeDeparture.tm_sec);

	appendInF10(string, bufferLength);

	//genero processo R1
	pidR1 = fork();
	if (pidR1 == 0)	{

		signal(SIGINT, sigHandlerReceiver);
		//stampo intestazione messaggio
		printIntestazione(F6);

		pid_t pidPIPER1 = fork();
		if(pidPIPER1 == 0){
			while(1){
				message_sending messageIncoming;
				semOp(semID, PIPE4READER, -1);
				ssize_t nBys = read(pipe4[0],&messageIncoming, sizeof(messageIncoming));
				if(nBys < 1){
					ErrExit("Errore uscito\n");
				}


				pid_t pidPIPER1_1 = fork();
				if(pidPIPER1_1==0){
					//genero tempo attuale
					time_t now = time(NULL);
					struct tm timeArrival = *localtime(&now);

					sleep(messageIncoming.DelS1);
					printInfoMessage(messageIncoming, timeArrival, F6);
					exit(0);
				} else if (pidPIPER1_1 == -1){
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = pidPIPER1_1;		
					pids->length = pids->length +1;
				}

				semOp(semID, PIPE4WRITER, 1);
			}
			exit(0);
		} else if (pidPIPER1 == -1){
			ErrExit("Fork");
		} else {
			pids->pids[pids->length].pid_parent = getpid();		
			pids->pids[pids->length].pid = pidPIPER1;		
			pids->length = pids->length +1;
		}

		//leggo dalla coda
		listen(MSQID, SHMID, semID, "R1");

		exit(0);
	} else if (pidR1 == -1){
		ErrExit("Fork");
	} else {
		pids->pids[pids->length].pid_parent = getpid();		
		pids->pids[pids->length].pid = pidR1;		
		pids->length = pids->length +1;
	}


	//genero processo R2
	pidR2 = fork();
	if (pidR2 == 0){

		signal(SIGINT, sigHandlerReceiver);
		//stampo intestazione messaggio
		printIntestazione(F5);

		pid_t pidPIPER2 = fork();
		if(pidPIPER2 == 0){
			while(1){
				message_sending messageIncoming;
				semOp(semID, PIPE3READER, -1);
				ssize_t nBys = read(pipe3[0],&messageIncoming, sizeof(messageIncoming));
				if(nBys < 1){
					ErrExit("Errore uscito\n");
				}


				pid_t pidPIPER2_1 = fork();
				if(pidPIPER2_1==0){
					//genero tempo attuale
					time_t now = time(NULL);
					struct tm timeArrival = *localtime(&now);

					sleep(messageIncoming.DelS2);
					printInfoMessage(messageIncoming, timeArrival, F5);
					deliverMessage(messageIncoming, "R2");
					exit(0);
				} else if (pidPIPER2_1 == -1){
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = pidPIPER2_1;		
					pids->length = pids->length +1;
				}

				semOp(semID, PIPE3WRITER, 1);
			}
			exit(0);
		} else if (pidPIPER2 == -1){
			ErrExit("Fork");
		} else {
			pids->pids[pids->length].pid_parent = getpid();		
			pids->pids[pids->length].pid = pidPIPER2;		
			pids->length = pids->length +1;
		}

		//leggo dalla coda
		listen(MSQID, SHMID, semID, "R2");

		//termino il processo
		exit(0);
	}
	else if (pidR2 == -1)
	{
		ErrExit("Fork");
	} else {
		pids->pids[pids->length].pid_parent = getpid();		
		pids->pids[pids->length].pid = pidR2;		
		pids->length = pids->length +1;
	}

	//genero processo S3
	pidR3 = fork();
	if (pidR3 == 0)	{

		signal(SIGINT, sigHandlerReceiver);
		//stampo intestazione messaggio
		printIntestazione(F4);

		listen(MSQID, SHMID, semID, "R3");

		//termino il processo
		exit(0);
	}
	else if (pidR3 == -1)
	{
		ErrExit("Fork");
	} else {
		pids->pids[pids->length].pid_parent = getpid();		
		pids->pids[pids->length].pid = pidR3;		
		pids->length = pids->length +1;
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

	unlink(FIFO);

	//Eliminazione semafori
	if (semID != -1 && semctl(semID, 0, IPC_RMID, 0) == -1)
	{
		ErrExit("semrmv failed");
	}

	// Eliminazione della struttura dei messaggi di pids
  free(pids->pids);
  free(pids);

	print_log("STO PER CHIUDERE\n");

	//termino il processo padre
	exit(0);
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

	size_t mSize = sizeof(struct message_queue) - sizeof(long);
	while (1)
	{
		semOp(semID, REQUEST, -1);

		//genero tempo attuale
		time_t now = time(NULL);
		struct tm timeArrival = *localtime(&now);

		//-------------------------------------------------- BLOCCO MESSAGE QUEUE --------------------------------------------------
		//prelevo il messaggio senza aspettare
		msgrcv(MSQID, &messaggio, mSize, 0, IPC_NOWAIT);

		//sei nel processo receiver corretto?
		if(strcmp(processo, messaggio.message.idReceiver) == 0){
			if (strcmp(messaggio.message.idReceiver, "R1") == 0)
			{
				pid_t childS1 = fork();
				if(childS1 == 0){
					//dormi
					sleep(messaggio.message.DelS1);
					//stampa le info sul tuo file
					printInfoMessage(messaggio.message, timeArrival, F6);
					deliverMessage(messaggio.message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}
			}
			else if (strcmp(messaggio.message.idReceiver, "R2") == 0)
			{
				pid_t childS1 = fork();
				if(childS1 == 0){
					//dormi
					sleep(messaggio.message.DelS2);
					//stampa le info sul tuo file
					printInfoMessage(messaggio.message, timeArrival, F5);
					deliverMessage(messaggio.message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}
			}
			else if (strcmp(messaggio.message.idReceiver, "R3") == 0)
			{
				pid_t childS1 = fork();
				if(childS1 == 0){
					//dormi
					sleep(messaggio.message.DelS3);
					//stampa le info sul tuo file
					printInfoMessage(messaggio.message, timeArrival, F4);
					deliverMessage(messaggio.message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}

			}
			semOp(semID, DATAREADY, 1);
			continue;
		}else if(strcmp("Q", messaggio.message.Type)== 0){
			//messaggio per un altro receiver
			if (msgsnd(MSQID, &messaggio, mSize, 0) == -1){
				ErrExit("re-msgsnd failed");
			} else {
				semOp(semID, REQUEST, 1);
				continue;
			}
		}
		//-------------------------------------------------- FINE BLOCCO MESSAGE QUEUE --------------------------------------------------

		//-------------------------------------------------- BLOCCO SHARED MEMORY --------------------------------------------------

		else if (strcmp(processo, request_shared_memory->message.idReceiver) == 0){

			if (strcmp(request_shared_memory->message.idReceiver, "R1") == 0){
				pid_t childS1 = fork();
				if(childS1 == 0){
					//dormi
					sleep(request_shared_memory->message.DelS1);
					//stampa le info sul tuo file
					printInfoMessage(request_shared_memory->message, timeArrival, F6);
					deliverMessage(request_shared_memory->message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}

			} else if (strcmp(request_shared_memory->message.idReceiver, "R2") == 0){
				pid_t childS1 = fork();
				if(childS1 == 0){
					sleep(request_shared_memory->message.DelS2);
					printInfoMessage(request_shared_memory->message, timeArrival, F5);
					deliverMessage(request_shared_memory->message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}

			} else if (strcmp(request_shared_memory->message.idReceiver, "R3") == 0){
				pid_t childS1 = fork();
				if(childS1 == 0){
					sleep(request_shared_memory->message.DelS3);
					printInfoMessage(request_shared_memory->message, timeArrival, F4);
					deliverMessage(request_shared_memory->message, processo);
					exit(0);
				} else if (childS1 == -1) {
					ErrExit("Fork");
				} else {
					pids->pids[pids->length].pid_parent = getpid();		
					pids->pids[pids->length].pid = childS1;		
					pids->length = pids->length +1;
				}
			} 

			semOp(semID, DATAREADY, 1);
			continue;
		} 

		//-------------------------------------------------- FINE BLOCCO SHARED MEMORY --------------------------------------------------

		//-------------------------------------------------- BLOCCO FIFO --------------------------------------------------

		else if (strcmp("R3", processo) == 0){

			int fd = open(FIFO, O_RDONLY);
			message_sending message;
			ssize_t nBys = read(fd,&message, sizeof(message));
			if(nBys < 1){
				ErrExit("Errore uscito\n");
			}

			pid_t childFIFO = fork();
			if(childFIFO == 0){
				sleep(message.DelS3);
				printInfoMessage(message, timeArrival, F4);
				deliverMessage(message, "R3");
				exit(0);
			} else if (childFIFO == -1) {
				ErrExit("Fork");
			} else {
				pids->pids[pids->length].pid_parent = getpid();		
				pids->pids[pids->length].pid = childFIFO;		
				pids->length = pids->length +1;
			}


			semOp(semID, DATAREADY, 1);
			continue;
		}

		//-------------------------------------------------- FINE BLOCCO FIFO --------------------------------------------------

		else {
			//non sono nel receiver corretto
			semOp(semID, REQUEST, 1);
			continue;
		}

	}
}

void deliverMessage(message_sending message, char processo[]){
	if (strcmp(processo, "R3") == 0){
		//invia a R2 tramite PIPE
		semOp(semID, PIPE3WRITER, -1);
		ssize_t nBys = write(pipe3[1], &message, sizeof(message));
		if(nBys != sizeof(message))
			ErrExit("Messaggio inviato male");
		semOp(semID, PIPE3READER, 1);
	} else if (strcmp(processo, "R2") == 0){
		//invia a R1 tramite PIPE
		semOp(semID, PIPE4WRITER, -1);
		ssize_t nBys = write(pipe4[1], &message, sizeof(message));
		if(nBys != sizeof(message))
			ErrExit("Messaggio inviato male");
		semOp(semID, PIPE4READER, 1);
	}
}
