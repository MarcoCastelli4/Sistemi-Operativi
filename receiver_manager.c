#include "defines.h"
#include "shared_memory.h"

int MSQID = -1;
int SHMID = -1;
int semID = -1;
int pipe3[2];
int pipe4[2];
pids_manager *myChildrenPid = NULL;
int waitTime = 0;
shared_memory_messages *shMessages = NULL;

void deliverMessage(message_sending, char []);
void writeF9(int, int, int);
void listen(int, int, int, char[]);

void sigHandlerChild(int);
void sigHandlerReceiver(int);

// Uccisione ricorsiva
void recursiveKill(pid_t pid){
	for(int i=0; i<myChildrenPid->length; i++){
		if(myChildrenPid->pids[i].pid_parent == pid){
			recursiveKill(myChildrenPid->pids[i].pid);
		}
	}
	// Se sei te stesso ucciditi
	if(pid == getpid()){
		kill(pid,SIGKILL);
	} else {
		// Altrimenti manda un sigterm in modo che uccida tutta la sua catena di figli
		kill(pid,SIGTERM);
	}
}

void customPause(int startingDelay){
	if(startingDelay > 0){
		alarm(startingDelay); //dormi per quanto ti manca
		pause();
		while(waitTime != 0){
			alarm(waitTime);
			waitTime = 0;
			pause();
		}
	}
}

void sigHandlerReceiver(int sig){
	// PER PROCESSI PADRI
	if(sig == SIGTERM){
		// SHUTDOWN
		recursiveKill(getpid());
		//exit(0);
	} else if(sig == SIGUSR2){
		// PER INCREASE DELAY
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid, SIGUSR2);
			}
		}
	} else if(sig == SIGUSR1){
		// PER SEND MESSAGE
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid,SIGUSR1);
			}
		}
	} else if(sig == SIGINT){
		// REMOVE MESSAGE
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid, SIGINT);
			}
		}
		return;
	}
}

void sigHandlerMedium(int sig){
	// PER PROCESSI MEDIUM
	if(sig == SIGTERM){
		// SHUTDOWN
		recursiveKill(getpid());
		//exit(0);
	} else if(sig == SIGUSR2){
		// PER INCREASE DELAY
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid, SIGUSR2);
			}
		}
	} else if(sig == SIGUSR1){
		// PER SEND MESSAGE
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid,SIGUSR1);
			}
		}
	} else if(sig == SIGINT){
		// REMOVE MESSAGE
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				recursiveKill(myChildrenPid->pids[i].pid);
			}
		}
		return;
	}
}

void sigHandlerChild(int sig){
	// PER PROCESSI PADRI
	if(sig == SIGTERM){
		// SHUTDOWN
		recursiveKill(getpid());
		exit(0);
	}else if(sig == SIGUSR2){
		// INCREASE DELAY NEL MESSAGGIO
		waitTime += 5;
		pause();
	} else if(sig == SIGUSR1){
		// SEND MSG NEL MESSAGGIO
		waitTime = 0;
	} else if(sig ==  SIGALRM){
		// GENERIC DETECTOR
	} 
	return;
}

int main(int argc, char *argv[]){
	pid_t pidR1, pidR2, pidR3;
	pid_t waitPID;
	myChildrenPid = malloc(sizeof(pids_manager));
	pid_manager *pids_list = malloc(sizeof(pid_manager) * (30));
	myChildrenPid->length = 0;
	myChildrenPid->pids = pids_list;


	//creo semaforo, sarà lo stesso del sender
	semID = semget(SKey, SEMNUMBER, IPC_CREAT | S_IRUSR | S_IWUSR);
	//finchè il sender non ha creato le IPC aspetto
	semOp(semID, CREATION, -1);
	//Accedo alla
	SHMID = alloc_shared_memory(MKey, sizeof(shared_memory_messages));

	//Accedo alla MessageQueue
	MSQID = msgget(QKey, S_IRUSR | S_IWUSR);
	if (MSQID == -1)
	{
		ErrExit("Message queue failed");
	}

	shMessages = get_shared_memory(SHMID, 0);

	// checking if PIPE successed
	int resPipe3 = pipe(pipe3);
	if (resPipe3 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale
	time_t now = time(NULL);
	struct tm timeCreation = *localtime(&now);

	//Scrivo info creazione PIPE3
	ssize_t bufferLength = (sizeof("PIPE3") + sizeof(&pipe3[0]) + sizeof(&pipe3[1]) + sizeof("RM") + 21 * sizeof(char));
	char *string = malloc(bufferLength);
	sprintf(string, "%s;%p/%p;%s;%02d:%02d:%02d;--:--:--;\n", "PIPE3", &pipe3[0], &pipe3[1], "RM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,7);

	// checking if PIPE successed
	int resPipe4 = pipe(pipe4);
	if (resPipe4 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale 
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione PIPE4
	bufferLength = (sizeof("PIPE4") + sizeof(&pipe4[0]) + sizeof(&pipe4[1])+sizeof("RM") + 21 * sizeof(char));
	string = (char *) malloc(bufferLength);
	sprintf(string, "%s;%p/%p;%s;%02d:%02d:%02d;--:--:--;\n", "PIPE4", &pipe4[0], &pipe4[1], "RM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,8);

	//genero processo R1
	pidR1 = fork();
	if (pidR1 == 0)	{
		initSignalFather(sigHandlerReceiver);

		//stampo intestazione messaggio
		printIntestazione(F6);
		//leggo dalla coda
		listen(MSQID, SHMID, semID, "R1");

		while(1)
			sleep(1);
		exit(0);
	} else if (pidR1 == -1){
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidR1;		
		myChildrenPid->length = myChildrenPid->length +1;
	}


	//genero processo R2
	pidR2 = fork();
	if (pidR2 == 0){
		initSignalFather(sigHandlerReceiver);

		//stampo intestazione messaggio
		printIntestazione(F5);
		//leggo dalla coda
		listen(MSQID, SHMID, semID, "R2");

		while(1)
			sleep(1);
		//termino il processo
		exit(0);
	}
	else if (pidR2 == -1)
	{
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidR2;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero processo S3
	pidR3 = fork();
	if (pidR3 == 0)	{
		initSignalFather(sigHandlerReceiver);

		//stampo intestazione messaggio
		printIntestazione(F4);
		listen(MSQID, SHMID, semID, "R3");

		while(1)
			sleep(1);
		//termino il processo
		exit(0);
	}
	else if (pidR3 == -1)
	{
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidR3;		
		myChildrenPid->length = myChildrenPid->length +1;
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

	//Segna chiuso PIPE3
	completeInF10("PIPE3");

	//Segna chiuso PIPE4
	completeInF10("PIPE4");

	//Segna chiuso Q
	completeInF10("Q");

	//Eliminazione memoria condivisa
	if(SHMID != -1){
		free_shared_memory(shMessages);
		remove_shared_memory(SHMID);
	}

	//Segna chiuso SH
	completeInF10("SH");

	unlink(FIFO);

	// Segna chiuso S
	completeInF10("FIFO");

	//Eliminazione semafori
	if (semID != -1 && semctl(semID, 0, IPC_RMID, 0) == -1)
	{
		ErrExit("semrmv failed");
	}

	//Segna chiuso S
	completeInF10("S");

	// Eliminazione della struttura dei messaggi di pids
	free(myChildrenPid->pids);
	free(myChildrenPid);

	//printf("Sono padre receiver e sto morendo\n");
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


void listen(int MSQID, int SHMID, int semID, char processo[]){
	struct message_queue messaggio;

	size_t mSize = sizeof(struct message_queue) - sizeof(long);

	//-------------------------------------------------- BLOCCO MESSAGE QUEUE --------------------------------------------------
	pid_t childMQ = fork();
	if(childMQ == 0){
		initSignalMedium(sigHandlerMedium);
		while(1){
			semOp(semID, ACCESSTOQ, -1);
			//prelevo il messaggio senza aspettare
			msgrcv(MSQID, &messaggio, mSize, 0, IPC_NOWAIT);

			//sei nel processo receiver corretto? ed è arrivato Q
			if(strcmp(processo, messaggio.message.idReceiver) == 0){
				if (strcmp(messaggio.message.idReceiver, "R1") == 0)
				{
					pid_t childS1 = fork();
					if(childS1 == 0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,messaggio.message, F6);
						customPause(messaggio.message.DelS1);
						completeInfoMessage(semID,messaggio.message, F6);
						exit(0);
					} else if (childS1 == -1) {
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}
				}
				else if (strcmp(messaggio.message.idReceiver, "R2") == 0)
				{
					pid_t childS1 = fork();
					if(childS1 == 0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,messaggio.message, F5);
						customPause(messaggio.message.DelS2);
						completeInfoMessage(semID,messaggio.message, F5);
						deliverMessage(messaggio.message, processo);
						exit(0);
					} else if (childS1 == -1) {
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}
				}
				else if (strcmp(messaggio.message.idReceiver, "R3") == 0)
				{
					pid_t childS1 = fork();
					if(childS1 == 0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,messaggio.message, F4);
						customPause(messaggio.message.DelS3);
						completeInfoMessage(semID,messaggio.message, F4);
						deliverMessage(messaggio.message, processo);
						exit(0);
					} else if (childS1 == -1) {
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}
				}
				// Pulisco la variabile message
				memset(&messaggio, 0, sizeof(messaggio));
				semOp(semID, DATAREADY, 1);
				continue;
			} 
			else if((strcmp("Q", messaggio.message.Type) == 0)){
				if (msgsnd(MSQID, &messaggio, mSize, 0) == -1){
					ErrExit("re-msgsnd failed");
				} else {
					// Pulisco la variabile message
					memset(&messaggio, 0, sizeof(messaggio));
					semOp(semID, ACCESSTOQ, 1);
					continue;
				}
			}
			//-------------------------------------------------- FINE BLOCCO MESSAGE QUEUE --------------------------------------------------
		}
	} else if (childMQ == -1) {
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = childMQ;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//-------------------------------------------------- BLOCCO SHARED MEMORY --------------------------------------------------
	pid_t childSM = fork();
	if(childSM == 0){
		initSignalMedium(sigHandlerMedium);
		while(1){
			semOp(semID, ACCESSTOSH, -1);
			if (strcmp(processo, shMessages->messages[shMessages->cursorStart].idReceiver) == 0){
				int i = shMessages->cursorStart;
				for(; i < shMessages->cursorEnd  || (i > shMessages->cursorEnd && i <= 9) ; i++){
					if (strcmp(shMessages->messages[i].idReceiver, "R1") == 0){
						pid_t childS1 = fork();
						if(childS1 == 0){
							initSignalChild(sigHandlerChild);
							printInfoMessage(semID,shMessages->messages[i], F6);
							customPause(shMessages->messages[i].DelS1);
							completeInfoMessage(semID,shMessages->messages[i], F6);
							exit(0);
						} else if (childS1 == -1) {
							ErrExit("Fork");
						} else {
							myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
							myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
							myChildrenPid->length = myChildrenPid->length +1;
						}
					} else if (strcmp(shMessages->messages[i].idReceiver, "R2") == 0){
						pid_t childS1 = fork();
						if(childS1 == 0){
							initSignalChild(sigHandlerChild);
							printInfoMessage(semID,shMessages->messages[i], F5);
							customPause(shMessages->messages[i].DelS2);
							completeInfoMessage(semID,shMessages->messages[i], F5);
							deliverMessage(shMessages->messages[i], processo);
							exit(0);
						} else if (childS1 == -1) {
							ErrExit("Fork");
						} else {
							myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
							myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
							myChildrenPid->length = myChildrenPid->length +1;
						}
					} else if (strcmp(shMessages->messages[i].idReceiver, "R3") == 0){
						pid_t childS1 = fork();
						if(childS1 == 0){
							initSignalChild(sigHandlerChild);
							printInfoMessage(semID,shMessages->messages[i], F4);
							customPause(shMessages->messages[i].DelS3);
							completeInfoMessage(semID,shMessages->messages[i], F4);
							deliverMessage(shMessages->messages[i], processo);
							exit(0);
						} else if (childS1 == -1) {
							ErrExit("Fork");
						} else {
							myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
							myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
							myChildrenPid->length = myChildrenPid->length +1;
						}
					}

				}
				// Se il cursore di scrittura è stato rimesso a 0, resetto anche il cursore di lettura (se è arrivato all'ultimo messaggio)
				//if(shMessages->cursorEnd > shMessages->cursorStart && shMessages->cursorStart < 5){
				if(i <= 9){
					shMessages->cursorStart = i;
				} else{
					shMessages->cursorStart = 0;
				}
				semOp(semID, DATAREADY, 1);
				continue;
			} else if(strcmp("SH", shMessages->messages[shMessages->cursorStart].Type) == 0 ){
				semOp(semID, ACCESSTOSH, 1);
				continue;
			}
			}
		} else if (childSM == -1) {
			ErrExit("Fork");
		} else {
			myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
			myChildrenPid->pids[myChildrenPid->length].pid = childSM;		
			myChildrenPid->length = myChildrenPid->length +1;
		}

		//-------------------------------------------------- BLOCCO FIFO --------------------------------------------------
		if (strcmp("R3",  processo) == 0){
			pid_t childFIFO = fork();
			if(childFIFO == 0){
				initSignalMedium(sigHandlerMedium);
				while(1){
					semOp(semID, ACCESSTOFIFO, -1);
					int fd = open(FIFO, O_RDONLY);
					message_sending message;
					ssize_t nBys = read(fd,&message, sizeof(message));
					close(fd);

					if(nBys < 1){
						ErrExit("Errore uscito\n");
					}

					pid_t childS1 = fork();
					if(childS1 == 0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,message, F4);
						customPause(message.DelS3);
						completeInfoMessage(semID,message, F4);
						deliverMessage(message, "R3");
						exit(0);
					} else if (childS1 == -1) {
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}

					semOp(semID, DATAREADY, 1);
				}
			} else if (childFIFO == -1) {
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = childFIFO;		
				myChildrenPid->length = myChildrenPid->length +1;
			}
		}

		//-------------------------------------------------- BLOCCO PIPE R1 --------------------------------------------------
		if (strcmp("R1",  processo) == 0){
			//leggo dalla pipe R1
			pid_t pidPIPER1 = fork();
			if(pidPIPER1 == 0){
				initSignalMedium(sigHandlerMedium);
				while(1){
					message_sending messageIncoming;
					semOp(semID, PIPE4READER, -1);
					ssize_t nBys = read(pipe4[0],&messageIncoming, sizeof(messageIncoming));
					if(nBys < 1){
						ErrExit("Errore uscito\n");
					}


					pid_t pidPIPER1_1 = fork();
					if(pidPIPER1_1==0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,messageIncoming, F6);
						customPause(messageIncoming.DelS1);
						completeInfoMessage(semID,messageIncoming, F6);
						exit(0);
					} else if (pidPIPER1_1 == -1){
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = pidPIPER1_1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}

					semOp(semID, PIPE4WRITER, 1);
				}
			} else if (pidPIPER1 == -1){
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = pidPIPER1;		
				myChildrenPid->length = myChildrenPid->length +1;
			}
		}

		//-------------------------------------------------- BLOCCO PIPE R2 --------------------------------------------------
		if (strcmp("R2",  processo) == 0){
			//leggo dalla pipe R2
			pid_t pidPIPER2 = fork();
			if(pidPIPER2 == 0){
				initSignalMedium(sigHandlerMedium);
				while(1){
					message_sending messageIncoming;
					semOp(semID, PIPE3READER, -1);
					ssize_t nBys = read(pipe3[0],&messageIncoming, sizeof(messageIncoming));
					if(nBys < 1){
						ErrExit("Errore uscito\n");
					}


					pid_t pidPIPER2_1 = fork();
					if(pidPIPER2_1==0){
						initSignalChild(sigHandlerChild);
						printInfoMessage(semID,messageIncoming, F5);
						customPause(messageIncoming.DelS2);
						completeInfoMessage(semID,messageIncoming, F5);
						deliverMessage(messageIncoming, "R2");
						exit(0);
					} else if (pidPIPER2_1 == -1){
						ErrExit("Fork");
					} else {
						myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
						myChildrenPid->pids[myChildrenPid->length].pid = pidPIPER2_1;		
						myChildrenPid->length = myChildrenPid->length +1;
					}

					semOp(semID, PIPE3WRITER, 1);
				}
			} else if (pidPIPER2 == -1){
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = pidPIPER2;		
				myChildrenPid->length = myChildrenPid->length +1;
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
