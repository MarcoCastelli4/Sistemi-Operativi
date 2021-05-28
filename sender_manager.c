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
void initSignalFather();
void initSignalChild();
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
	// Se sei testo ucciditi
	if(pid == getpid()){
		kill(pid,SIGKILL);
	} else {
		// Altrimenti manda un sigterm in modo che uccida tutta la sua catena di figli
		kill(pid,SIGTERM);
	}
	exit(0);
}

void customPause(int startingDelay){
	if (startingDelay > 0){
		alarm(startingDelay); //dormi per quanto ti manca
		pause();
		while(waitTime != 0){
			alarm(waitTime);
			waitTime = 0;
			pause();
		}
	}

}

void sigHandlerSender(int sig){
	// PER PROCESSI PADRI
	if(sig == SIGTERM){
		// SHUTDOWN
		recursiveKill(getpid());
		exit(0);
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
				kill(myChildrenPid->pids[i].pid,SIGCONT);
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
		print_log("SONO INCREASE DELAY\n");
		waitTime += 5;
		pause();
	} else if(sig == SIGCONT){
		print_log("SONO SIG CONT\n");
		// SEND MSG NEL MESSAGGIO
		waitTime = 0;
	} else if(sig ==  SIGALRM){
		// GENERIC DETECTOR
	} 
	return;
}

int main(int argc, char *argv[])
{
	initSignalFather();

	pid_t pidS1, pidS2, pidS3;
	pid_t waitPID;
	myChildrenPid = malloc(sizeof(pids_manager));
	pid_manager *pids_list = malloc(sizeof(pid_manager) * (15));
	myChildrenPid->length = 0;
	myChildrenPid->pids = pids_list;

	// Creazione header per F10
	writeF10Header();

	//Inizializzo il semaforo e attendo
	semID = create_sem_set(SEMNUMBER);

	//genero il tempo attuale
	time_t now = time(NULL);
	struct tm timeCreation = *localtime(&now);

	//Scrivo info creazione semaforo
	ssize_t bufferLength = (sizeof("S") + numcifre(semID) + sizeof("SM") + 20 * sizeof(char));
	char *string = malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "S", semID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,1);

	// Creo la message queue
	MSQID = msgget(QKey, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (MSQID == -1)
	{
		ErrExit("Message queue failed");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione Q
	bufferLength = (sizeof("Q") + numcifre(MSQID) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "Q", MSQID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,2);

	//Creazione della shared memory
	SHMID = alloc_shared_memory(MKey, sizeof(shared_memory_messages));
	if (SHMID == -1)
	{
		ErrExit("Shared memory failed");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione SH
	bufferLength = (sizeof("SH") + numcifre(SHMID) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "SH", SHMID, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,3);

	shMessages = (shared_memory_messages*)get_shared_memory(SHMID, 0);

	unlink(FIFO);
	int res = mkfifo(FIFO, O_CREAT | O_TRUNC | S_IRUSR | S_IWUSR);
	if (res == -1)
	{
		ErrExit("Creazione fifo errata");
	}

	//genero il tempo attuale
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione FIFO
	bufferLength = (sizeof("FIFO") + numcifre(res) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "FIFO", res, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,4);

	// checking if PIPE successed
	int resPipe1 = pipe(pipe1);
	if (resPipe1 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale 
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione PIPE1
	bufferLength = (sizeof("PIPE1") + numcifre(resPipe1) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "PIPE1", resPipe1, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,5);

	// checking if PIPE successed
	int resPipe2 = pipe(pipe2);
	if (resPipe2 == -1)
		ErrExit("PIPE");

	//genero il tempo attuale 
	now = time(NULL);
	timeCreation = *localtime(&now);

	//Scrivo info creazione PIPE2
	bufferLength = (sizeof("PIPE2") + numcifre(resPipe2) + sizeof("SM") + 20 * sizeof(char));
	string = (char *)malloc(bufferLength);
	sprintf(string, "%s;%d;%s;%02d:%02d:%02d;00:00:00;\n", "PIPE2", resPipe2, "SM", timeCreation.tm_hour, timeCreation.tm_min, timeCreation.tm_sec);
	appendInF10(string, bufferLength,6);

	//ho creato puoi usarle
	semOp(semID, CREATION, 1);
	F0 = argv[1];

	if (argc < 1)
	{
		exit(1);
	}

	//la struttura messaggi inizialmente è vuota
	//genero processo S1
	pidS1 = fork();
	if (pidS1 == 0)
	{
		initSignalChild();
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
	else if (pidS1 == -1)
	{
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidS1;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero processo S2
	pidS2 = fork();
	if (pidS2 == 0)
	{
		initSignalChild();
		//scrivo intestazione
		printIntestazione(F2);

		while (s1EndReading == 0)
		{
			message_sending message;
			semOp(semID, PIPE1READER, -1);
			ssize_t nBys = read(pipe1[0], &message, sizeof(message));
			if (nBys < 1)
			{
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
		initSignalChild();
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
	else if (pidS3 == -1)
	{
		ErrExit("Fork");
	} else {
		myChildrenPid->pids[myChildrenPid->length].pid_parent = getpid();		
		myChildrenPid->pids[myChildrenPid->length].pid = pidS3;		
		myChildrenPid->length = myChildrenPid->length +1;
	}

	//genero file8.csv
	writeF8(pidS1, pidS2, pidS3);
	semOp(semID, HACKLERSENDER, 1);

	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0);

	//aspetto che il receiver finisca di usare le IPC
	semOp(semID, ELIMINATION, -1);

	// Eliminazione della struttura dei messaggi di pids
	free(myChildrenPid->pids);
	free(myChildrenPid);



	//Segna chiuso PIPE1
	completeInF10("PIPE1");

	///Segna chiuso PIPE2
	completeInF10("PIPE2");

	//termino il processo padre
	exit(0);
}

//Funzione che mi genera il file F8 e scrive ogni riga
void writeF8(int pid1, int pid2, int pid3)
{

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

message_group *carica_F0(char nomeFile[])
{

	//apro il file
	int fp = open(nomeFile, O_RDONLY);
	if (fp == -1)
		ErrExit("Open");

	// utilizzo lseek per calcolarne le dimensioni
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1)
	{
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
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1))
	{
		ErrExit("Read");
	}
	buf[bufferLength] = '\0';

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F0 (corrispondono al numero di messaggi presenti)
	for (int i = 0; i < bufferLength; i++)
	{
		if (buf[i] == '\n')
		{
			rowNumber++;
		}
	}

	//allochiamo dinamicamente un array di azioni delle dimensioni opportune
	// TODO MALLOC DIFETTOSA
	message_sending *messageList = malloc(sizeof(message_sending) * (rowNumber));
	//numero di messaggi che inserisco
	int messageNumber = 0;

	char *end_str;
	//prendo la riga che è delimitata dal carattere \n
	char *row = strtok_r(buf, "\n", &end_str);
	//scorro finchè la riga non è finita
	while (row != NULL)
	{

		char *end_segment;
		//prendo il singolo campo/segmento che è delimitato dal ;
		char *segment = strtok_r(row, ";", &end_segment);
		int campo = 0; //(0..7, id, message..)
		//scorro finchè il campo non è finito (la casella)
		while (segment != NULL)
		{

			//memorizzo il segmento ne rispettivo campo della struttura
			switch (campo)
			{
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

void sendMessage(message_group *messageG, char processo[])
{
	time_t now = time(NULL);
	int i;
	for (i = 0; i < messageG->length; i++)
	{
		//genero tempo attuale
		struct tm timeArrival = *localtime(&now);

		//ritardo il messaggio
		if (strcmp(processo, "S1") == 0){

			pid_t childS1 = fork();
			if(childS1 == 0){
				initSignalChild();
				print_log("PRIMA ATTESA\n");
				customPause(messageG->messages[i].DelS1);

				//stampa su file F1
				printInfoMessage(semID,messageG->messages[i], timeArrival, F1);
				messageHandler(messageG->messages[i], "S1");
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
		else if (strcmp(processo, "S2") == 0)
		{
			pid_t childS1 = fork();
			if(childS1 == 0){
				initSignalChild();
				customPause(messageG->messages[i].DelS2);

				//stampa su file F2
				printInfoMessage(semID,messageG->messages[i], timeArrival, F2);
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
				initSignalChild();
				customPause(messageG->messages[i].DelS3);

				//stampa su file F3
				printInfoMessage(semID,messageG->messages[i], timeArrival, F3);
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

void messageHandler(message_sending message, char processo[])
{
	struct message_queue m;
	m.mtype = 1;

	memcpy(&m.message, &message, sizeof(message));
	size_t mSize = sizeof(struct message_queue) - sizeof(long);
	//se sono nel processo sender corretto
	//viene inviato tramite message queue
	if (strcmp(message.Type, "Q") == 0 && strcmp(processo, message.idSender) == 0)
	{
		// sending the message in the queue
		semOp(semID, REQUEST, 1);
		if (msgsnd(MSQID, &m, mSize, 0) == -1)
			ErrExit("msgsnd failed");
		semOp(semID, DATAREADY, -1);
	}
	//viene inviato tramite shared memory
	else if (strcmp(message.Type, "SH") == 0 && strcmp(processo, message.idSender) == 0)
	{

		semOp(semID, REQUEST, 1);
		memcpy(&shMessages->messages[shMessages->cursorEnd], &message, sizeof(message));	
		shMessages->messages[shMessages->cursorEnd] = message;
		if(shMessages->cursorEnd < 15){
			shMessages->cursorEnd++;
		}else{
			shMessages->cursorEnd = 0;
		}

		semOp(semID, DATAREADY, -1);
	}
	else if ((strcmp(processo, "S3") == 0) && (strcmp(message.Type, "FIFO") == 0))
	{
		//invia a R3 tramite FIFO
		semOp(semID, REQUEST, 1);
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
		} else if (strcmp(processo, "S2") == 0)
		{
			//invia a S3 tramite PIPE
			semOp(semID, PIPE2WRITER, -1);
			ssize_t nBys = write(pipe2[1], &message, sizeof(message));
			if(nBys != sizeof(message))
				ErrExit("Messaggio inviato male");
			semOp(semID, PIPE2READER, 1);
		}
	}
}


void initSignalFather ( ){
	sigset_t mySet;
	sigfillset(&mySet);
	sigdelset(&mySet, SIGINT);
	sigdelset(&mySet, SIGUSR1);
	sigdelset(&mySet, SIGUSR2);
	sigdelset(&mySet, SIGTERM);
	sigprocmask(SIG_SETMASK, &mySet, NULL);

	struct sigaction sigact;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigHandlerSender;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
};

void initSignalChild ( ){
	sigset_t mySet;
	sigfillset(&mySet);
	sigdelset(&mySet, SIGALRM);
	sigdelset(&mySet, SIGCONT);
	sigdelset(&mySet, SIGUSR2);
	sigdelset(&mySet, SIGTERM);
	sigprocmask(SIG_SETMASK, &mySet, NULL);

	struct sigaction sigact;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigHandlerChild;
	sigaction(SIGALRM, &sigact, NULL);
	sigaction(SIGCONT, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
};
