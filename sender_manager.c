#include "defines.h"
#include "shared_memory.h"
#include <signal.h>


int pipe1[2];
int pipe2[2];
int s1EndReading = 0;
int s2EndReading = 0;
int waitTime = 0;
char *F0;
int MSQID = -1;
int SHMID = -1;
int semID;
pids_manager *myChildrenPid = NULL;
message_group *messages = NULL;
shared_memory_messages *shMessages = 0;

void sigHandlerChild(int);
void sigHandlerSender(int);
void writeF8(int, int, int);
void writeF10Header();
message_group *carica_F0(char[]);

void sendMessage(message_group *messageG, char processo[]);
void messageHandler(message_sending message, char processo[]);

// Uccisione ricorsiva
void recursiveKill(pid_t pid){
	for(int i=0; i<myChildrenPid->length; i++){
		if(myChildrenPid->pids[i].pid_parent == pid){
			recursiveKill(myChildrenPid->pids[i].pid);
		}
	}
	//se il pid è tuo allora termina 
	if(pid == getpid()){
		kill(pid,SIGKILL);
	} else {
		//altrimenti manda un sigterm in modo che uccida tutta la sua catena di figli
		kill(pid,SIGTERM);
	}
}

//funzione usata per gestire dinamicamente il tempo di attesa dei figli che gestiscono i messaggi,
//soprattutto per gli increase delay e sendmsg dell'hackler
void customPause(int startingDelay){
	if (startingDelay > 0){
		//faccio partire un cronometro per il tempo di ritardo da attendere e mi metto in pausa
		alarm(startingDelay); 
		pause();
		//waitTime è il ritardo dato da un segnale dell'hackler
		while(waitTime != 0){
			alarm(waitTime);
			waitTime = 0;
			pause();
		}
	}
}
//Come in receiver, qua ci sono due strati di processi, il primo inoltra i segnali al secondo
void sigHandlerSender(int sig){
	//per i processi S1,S2,S3
	if(sig == SIGTERM){
		//terminazione dei figli(i quali si occuperanno di terminare i loro figli)
		recursiveKill(getpid());
		exit(0);
	} else if(sig == SIGUSR2){
		//per il segnale increase delay, invio a tutti i figli sfruttando l'array di pids
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid, SIGUSR2);
			}
		}
	} else if(sig == SIGUSR1){
		//per il segnale send message
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				kill(myChildrenPid->pids[i].pid,SIGUSR1);
			}
		}
	} else if(sig == SIGINT){
		// per il segnale remove message (il figlio che si occupava del messaggio verrà ucciso)
		for(int i=0; i<myChildrenPid->length; i++){
			if(myChildrenPid->pids[i].pid_parent == getpid()){
				recursiveKill(myChildrenPid->pids[i].pid);
			}
		}
		return;
	}
}

void sigHandlerChild(int sig){
	//handler del secondo livello
	if(sig == SIGTERM){
		//uccisione del secondo livello ed eventuali figli (se non ci sono uccide il pid inserito e basta)
		recursiveKill(getpid());
		exit(0);
	}else if(sig == SIGUSR2){
		//aumento il wait time gestito nella funzione custom pause
		waitTime += 5;
		pause();
	} else if(sig == SIGUSR1){
		//questo farà si che il messaggio termini l'attesa nella quale si trovava e invii immediatamente il messaggio
		waitTime = 0;
	} else if(sig ==  SIGALRM){
		//segnale generico, a disposizione per eventuali modifiche in sede d'esame
	} 
	return;
}

int main(int argc, char *argv[])
{
	pid_t pidS1, pidS2, pidS3;
	pid_t waitPID;
	myChildrenPid = malloc(sizeof(pids_manager));
	pid_manager *pids_list = malloc(sizeof(pid_manager) * (30));
	myChildrenPid->length = 0;
	myChildrenPid->pids = pids_list;

	//creazione header per F10
	writeF10Header();

	//inizializzo il semaforo e attendo
	semID = create_sem_set(SEMNUMBER);

	//genero il tempo attuale
	time_t now = time(NULL);
	struct tm timeCreation = *localtime(&now);

	//scrivo info creazione semaforo
	ssize_t bufferLength = (sizeof("S") + numcifre(semID) + sizeof("SM") + 20 * sizeof(char));
	char *string = malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;--:--:--;\n", "S", semID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,1);

	//creo la message queue
	MSQID = msgget(QKey, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (MSQID == -1){
		ErrExit("Message queue failed");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//scrivo info creazione Q
	bufferLength = (sizeof("Q") + numcifre(MSQID) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;--:--:--;\n", "Q", MSQID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,2);

	//creazione della shared memory
	SHMID = alloc_shared_memory(MKey, sizeof(shared_memory_messages));
	if (SHMID == -1){
		ErrExit("Shared memory failed");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//scrivo info creazione SH
	bufferLength = (sizeof("SH") + numcifre(SHMID) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;--:--:--;\n", "SH", SHMID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,3);

	shMessages = (shared_memory_messages*)get_shared_memory(SHMID, 0);

	unlink(FIFO);
	int res = mkfifo(FIFO, O_CREAT | O_TRUNC | S_IRUSR | S_IWUSR);
	if (res == -1){
		ErrExit("Creazione fifo errata");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//scrivo info creazione FIFO
	char *addressFIFO=malloc(sizeof(&FIFO));
   	sprintf(addressFIFO,"%p",&FIFO);
	bufferLength = (sizeof("FIFO") + (strlen(addressFIFO)) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%p;%s;%02d:%02d:%02d;--:--:--;\n", "FIFO", FIFO, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,4);

	free(addressFIFO);
	//checking if PIPE successed
	int resPipe1 = pipe(pipe1);
	if (resPipe1 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale 
	now = time(NULL);
	timeCreation = *localtime(&now);

	//scrivo info creazione PIPE1
	char *addressPipe=malloc(sizeof(&pipe1[0]));
   	sprintf(addressPipe,"%p",&pipe1[0]);
 	bufferLength = (sizeof("PIPE1") + (strlen(addressPipe) *2) +sizeof("SM") + 21 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%p/%p;%s;%02d:%02d:%02d;--:--:--;\n", "PIPE1", &pipe1[0], &pipe1[1], "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,5);

	//creo la pipe2
	int resPipe2 = pipe(pipe2);
	if (resPipe2 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale 
	now = time(NULL);
	timeCreation = *localtime(&now);

	//scrivo info creazione PIPE2
	bufferLength = (sizeof("PIPE1") + (strlen(addressPipe) *2) +sizeof("SM") + 21 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%p/%p;%s;%02d:%02d:%02d;--:--:--;\n", "PIPE2", &pipe2[0], &pipe2[1], "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,6);

	free(addressPipe);

	//ora che le risorse sono state create il receiver può partire
	semOp(semID, CREATION, 1);
	
	//controllo e assegno 
	F0 = argv[1];
	if (argc < 1){
		exit(1);
	}

	//la struttura messaggi inizialmente è vuota
	//genero processo S1
	pidS1 = fork();
	if (pidS1 == 0){
		//imposto handler e maschera
		initSignalFather(sigHandlerSender);
		//inizializzo la struttura con la dimensione di un messaggio

		messages = carica_F0(F0);

		//scrivo intestazione
		printIntestazione(F1);
		//mando tutti i messaggi
		sendMessage(messages, "S1");
		s1EndReading = 1;

		while(1)
			sleep(1);

		exit(0);
		//termino il processo
	}
	else if (pidS1 == -1){
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidS1;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero processo S2
	pidS2 = fork();
	if (pidS2 == 0){
		initSignalFather(sigHandlerSender);
		//scrivo intestazione
		printIntestazione(F2);

		while (s1EndReading == 0){
			message_sending message;
			semOp(semID, PIPE1READER, -1);
			ssize_t nBys = read(pipe1[0], &message, sizeof(message));
			if (nBys < 1){
				ErrExit("Errore uscito\n");
			}
			message_group *messageGroupS2 = malloc(sizeof(messageGroupS2));
			messageGroupS2->length = 1;
			messageGroupS2->messages = &message;

			sendMessage(messageGroupS2, "S2");

			free(messageGroupS2);
			semOp(semID, PIPE1WRITER, 1);
		}
		s2EndReading = 1;

		//termino il processo
		while(1)
			sleep(1);
		exit(0);
	}
	else if (pidS2 == -1)
	{
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidS2;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero processo S3
	pidS3 = fork();
	if (pidS3 == 0)
	{
		initSignalFather(sigHandlerSender);
		//scrivo intestazione
		printIntestazione(F3);

		while (s2EndReading == 0)
		{
			message_sending message;
			semOp(semID, PIPE2READER, -1);
			ssize_t nBys = read(pipe2[0], &message, sizeof(message));
			if (nBys < 1)
			{
				ErrExit("Errore uscito\n");
			}
			message_group *messageGroupS3 = malloc(sizeof(messageGroupS3));
			messageGroupS3->length = 1;
			messageGroupS3->messages = &message;

			sendMessage(messageGroupS3, "S3");

			free(messageGroupS3);
			semOp(semID, PIPE2WRITER, 1);
		}

		while(1)
			sleep(1);
		exit(0);
	}
	else if (pidS3 == -1){
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidS3;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero file8.csv
	writeF8(pidS1, pidS2, pidS3);
	//ora che f8 è stato generato l'hackler può partire
	semOp(semID, HACKLERSENDER, 1);

	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0);

	//aspetto che il receiver finisca di usare le IPC
	semOp(semID, ELIMINATION, -1);

	//eliminazione della struttura dei messaggi di pids
	free(myChildrenPid->pids);
	free(myChildrenPid);

	//Chiusura della msgQueue
	if (MSQID != -1 && msgctl(MSQID, IPC_RMID, NULL) == -1){
		ErrExit("msgrmv failed");
	}

	//Segna chiuso Q in F10
	completeInF10("Q");

	//Eliminazione memoria condivisa
	if(SHMID != -1){
		free_shared_memory(shMessages);
		remove_shared_memory(SHMID);
	}

	//Segna chiuso SH in F10
	completeInF10("SH");

	unlink(FIFO);

	// Segna chiuso S in F10
	completeInF10("FIFO");

	//Eliminazione semafori
	if (semID != -1 && semctl(semID, 0, IPC_RMID, 0) == -1){
		ErrExit("semrmv failed");
	}

	//Segna chiuso S in F10
	completeInF10("S");

	close(pipe1[0]);
	close(pipe1[1]);
	//segna chiuso PIPE1
	completeInF10("PIPE1");

	close(pipe2[0]);
	close(pipe2[1]);
	//segna chiuso PIPE2
	completeInF10("PIPE2");
	
	//termino il processo padre
	exit(0);
}

//Funzione che mi genera il file F8 e scrive ogni riga
void writeF8(int pid1, int pid2, int pid3){

	//creo il file se è gia presente lo sovrascrivo
	int fp = open(F8, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer
	int n = 26 + numcifre(pid1) + numcifre(pid2) + numcifre(pid3);

	//inizializzo buffer delle dimenisoni corrette
	ssize_t bufferLength = sizeof(char) * n;
	char *buffer = malloc(bufferLength);

	//converto i dati in stringa
	sprintf(buffer, "Sender Id;PID\nS1;%d\nS2;%d\nS3;%d\n", pid1, pid2, pid3);

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}

message_group *carica_F0(char nomeFile[]){

	//apro il file
	int fp = open(nomeFile, O_RDONLY);
	if (fp == -1)
		ErrExit("Open");

	// utilizzo lseek per calcolarne le dimensioni
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1){
		ErrExit("Lseek");
	}

	// posiziono l'offset alla prima riga dei messaggi (salto i titoli)
	if (lseek(fp, (size_t)(MessageSendingHeader + 1) * sizeof(char), SEEK_SET) == -1){
		ErrExit("Lseek");
	}

	//calcolo la dimensione del file da leggere a cui tolgo i "titoli" dei vari campi
	int bufferLength = fileSize / sizeof(char) - MessageSendingHeader - 1;
	//inizializzo il buffer
	char buf[bufferLength];
	//leggo dal file e salvo ciò che ho letto nel buf
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)){
		ErrExit("Read");
	}
	buf[bufferLength] = '\0';

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F0 (corrispondono al numero di messaggi presenti)
	for (int i = 0; i < bufferLength; i++){
		if (buf[i] == '\n'){
			rowNumber++;
		}
	}

	//allochiamo dinamicamente un array di azioni delle dimensioni opportune
	message_sending *messageList = malloc(sizeof(message_sending) * (rowNumber));
	//numero di messaggi che inserisco
	int messageNumber = 0;

	char *end_str;
	//prendo la riga che è delimitata dal carattere \n
	char *row = strtok_r(buf, "\n", &end_str);
	//scorro finchè la riga non è finita
	while (row != NULL)	{

		char *end_segment;
		//prendo il singolo campo/segmento che è delimitato dal ;
		char *segment = strtok_r(row, ";", &end_segment);
		int campo = 0; //(0..7, id, message..)
		//scorro finchè il campo non è finito (la casella)
		while (segment != NULL){
			//memorizzo il segmento ne rispettivo campo della struttura
			switch (campo){
				case 0:
					messageList[messageNumber].id = atoi(segment);
					break;
				case 1:
					strcpy(messageList[messageNumber].message, segment);
					break;
				case 2:
					strcpy(messageList[messageNumber].idSender, segment);
					break;
				case 3:
					strcpy(messageList[messageNumber].idReceiver, segment);
					break;
				case 4:
					messageList[messageNumber].DelS1 = atoi(segment);
					break;
				case 5:
					messageList[messageNumber].DelS2 = atoi(segment);
					break;
				case 6:
					messageList[messageNumber].DelS3 = atoi(segment);
					break;
				case 7:
					segment[strlen(segment) - 1] = '\0'; //perchè altrimenti mi rimane un carattere spazzatura in più
					strcpy(messageList[messageNumber].Type, segment);
					break;
				default:
					break;
			}
			//vado al campo successivo
			campo++;
			segment = strtok_r(NULL, ";", &end_segment);
		}
		//vado alla riga successiva
		messageNumber++;
		row = strtok_r(NULL, "\n", &end_str);
	}

	//inserisco nella mia struttura l'array di messaggi e quanti messaggi sono stati inseriti
	message_group *messageG = malloc(sizeof(messageG));
	messageG->length = messageNumber;
	messageG->messages = messageList;

	return messageG;
}
//funzione che genera i figli per la gestione dell'invio dei messaggi e dei ritardi
void sendMessage(message_group *messageG, char processo[]){
	int i;
	for (i = 0; i < messageG->length; i++)
	{

		//ritardo il messaggio
		if (strcmp(processo, "S1") == 0){

			pid_t childS1 = fork();
			if(childS1 == 0){
				//assegno l'handler e genero la maschera per i segnali
				initSignalChild(sigHandlerChild);
				//stampo in F1 il messaggio
				printInfoMessage(semID,messageG->messages[i], F1);
				customPause(messageG->messages[i].DelS1);

				//stampa su file F1 la time departure
				completeInfoMessage(semID,messageG->messages[i], F1);
				//Invio effettivo del messaggio
				messageHandler(messageG->messages[i], "S1");
				exit(0);
			}
			else if (childS1 == -1){
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
				myChildrenPid->length = myChildrenPid->length +1;
			}
		}
		else if (strcmp(processo, "S2") == 0)
		{
			pid_t childS1 = fork();
			if(childS1 == 0){
				initSignalChild(sigHandlerChild);
				printInfoMessage(semID,messageG->messages[i], F2);
				customPause(messageG->messages[i].DelS2);

				completeInfoMessage(semID,messageG->messages[i], F2);
				messageHandler(messageG->messages[i], "S2");
				exit(0);
			}
			else if (childS1 == -1)
			{
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
				myChildrenPid->length = myChildrenPid->length +1;
			}
		}
		else if (strcmp(processo, "S3") == 0)
		{
			pid_t childS1 = fork();
			if(childS1 == 0){
				initSignalChild(sigHandlerChild);
				printInfoMessage(semID,messageG->messages[i], F3);
				customPause(messageG->messages[i].DelS3);

				completeInfoMessage(semID,messageG->messages[i], F3);
				messageHandler(messageG->messages[i], "S3");
				exit(0);
			}
			else if (childS1 == -1)
			{
				ErrExit("Fork");
			} else {
				myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
				myChildrenPid->pids[myChildrenPid->length].pid = childS1;		
				myChildrenPid->length = myChildrenPid->length +1;
			}
		}
	}
}
//funzione che si occupa dell'invio del messaggio
void messageHandler(message_sending message, char processo[]){
	struct message_queue m;
	m.mtype = 1;

	memcpy(&m.message, &message, sizeof(message));
	size_t mSize = sizeof(struct message_queue) - sizeof(long);
	//se sono nel processo sender corretto
	//viene inviato tramite message queue
	if (strcmp(message.Type, "Q") == 0 && strcmp(processo, message.idSender) == 0){
		//il receiver potrà accedere alla message queue
		semOp(semID, ACCESSTOQ, 1);
		if (msgsnd(MSQID, &m, mSize, 0) == -1)
			ErrExit("msgsnd failed");
		//un messaggio è stato scritto, il prossimo sender aspetterà qua
		semOp(semID, DATAREADY, -1);
	}
	//viene inviato tramite shared memory
	else if (strcmp(message.Type, "SH") == 0 && strcmp(processo, message.idSender) == 0){
		//il receiver potrà accedere alla shared memory
		semOp(semID, ACCESSTOSH, 1);
		memcpy(&shMessages->messages[shMessages->cursorEnd], &message, sizeof(message));	
		shMessages->messages[shMessages->cursorEnd] = message;
		if(shMessages->cursorEnd < 9){
			shMessages->cursorEnd++;
		}else{
			shMessages->cursorEnd = 0;
		}

		semOp(semID, DATAREADY, -1);
	}
	else if ((strcmp(processo, "S3") == 0) && (strcmp(message.Type, "FIFO") == 0))
	{
		//invia a R3 tramite FIFO
		semOp(semID, ACCESSTOFIFO, 1);
		int fd = open(FIFO, O_WRONLY);
		write(fd, &message, sizeof(message));
		close(fd);
		semOp(semID, DATAREADY, -1);
	}
	//non sono nel processo sender corretto, seguo la catena di invio
	else if(strcmp(processo, message.idSender) != 0 || ((strcmp(processo, "S3") != 0) && (strcmp(message.Type, "FIFO") == 0)))
	{
		//viene inviato tramite PIPE, fino a che non raggiunge il sender corretto con il quale partità con modalità Type
		if (strcmp(processo, "S1") == 0){
			//invia a S2 tramite PIPE
			semOp(semID, PIPE1WRITER, -1);
			ssize_t nBys = write(pipe1[1], &message, sizeof(message));
			if(nBys != sizeof(message))
				ErrExit("Messaggio inviato male");
			semOp(semID, PIPE1READER, 1);
		} else if (strcmp(processo, "S2") == 0){
			//invia a S3 tramite PIPE
			semOp(semID, PIPE2WRITER, -1);
			ssize_t nBys = write(pipe2[1], &message, sizeof(message));
			if(nBys != sizeof(message))
				ErrExit("Messaggio inviato male");
			semOp(semID, PIPE2READER, 1);
		}
	}
}


