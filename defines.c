#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"

//funzione che mi calcola il numero di cifre di un numero intero (serve per sapere quanti caratteri servono per il numero)
int numcifre(int n)
{
	int i = 0;

	while (n / 10 != 0)
	{
		i++;
		n /= 10;
	}

	return i + 1;
}


char *toString(message_sending message)
{
	char *string = malloc(stringLenght(message));
	sprintf(string, "%d %s %s %s %d %d %d %s\n", message.id, message.message, message.idSender, message.idReceiver, message.DelS1, message.DelS2, message.DelS3, message.Type);
	return string;
}
//usata solo per il toString
int stringLenght(message_sending message)
{
	return numcifre(message.id) + sizeof(message.message) + sizeof(message.idSender) + sizeof(message.idReceiver) + 20 * sizeof(char);
}

//va chiamata all'inizio quando genero i file
//stampa l'intestazione del F1..F6
void printIntestazione(char file[])
{
	//creo il file se è gia presente lo sovrascrivo
	int fp = open(file, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//scrivo intestazione#
	//calcolo il numero totale di caratteri da scrivere nel buffer
	ssize_t bufferLength = sizeof(char) * TrafficInfoLength;
	char *header = TrafficInfo;

	if (write(fp, header, bufferLength) != bufferLength)
	{
		ErrExit("Write");
	}

	close(fp);
}
//Stampa all'interno del file specificato il messaggio che è passato in quel R/S
void printInfoMessage(int semID, message_sending message, char file[])
{

	semOp(semID, INFOMESSAGEFILE, -1);
	//scrivo in append il messaggio al file già presente cosi non perdo i messaggi precedenti
	int fp = open(file, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//genero il tempo attuale --> TimeDeparture
	time_t now = time(NULL);
	struct tm TimeArrival = *localtime(&now);

	//calcolo la dimensione della riga da scrivere
	ssize_t bufferLength = (numcifre(message.id) + sizeof(message.message) + sizeof(message.idSender) + sizeof(message.idReceiver) + 20 * sizeof(char));
	char *string = malloc(bufferLength);

	//mi salvo tutta la stringa
	sprintf(string, "%d;%s;%c;%c;%02d:%02d:%02d;00:00:00;\n", message.id, message.message, message.idSender[1], message.idReceiver[1], TimeArrival.tm_hour, TimeArrival.tm_min, TimeArrival.tm_sec);

	//scrivo la stringa nel file
	if (write(fp, string, strlen(string) * sizeof(char)) != strlen(string) * sizeof(char))
	{
		ErrExit("Write");
	}

	//libero lo spazio allocato per la stringa
	free(string);

	close(fp);
	semOp(semID, INFOMESSAGEFILE, 1);
}

//Stampa all'interno del file specificato il messaggio che è passato in quel R/S
void completeInfoMessage(int semID, message_sending message, char file[])
{

	semOp(semID, INFOMESSAGEFILE, -1);

	//apro il file 
	int fp = open(file, O_RDONLY);
	if (fp == -1)
		ErrExit("Open failed");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { ErrExit("Lseek failed"); }

	// posiziono l'offset alla prima riga (salto i titoli) 
	if (lseek(fp, (size_t)TrafficInfoLength * sizeof(char), SEEK_SET) == -1) {
		ErrExit("Lseek failed");
	}

	int bufferLength = fileSize / sizeof(char) - TrafficInfoLength;
	char buf[bufferLength];
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		ErrExit("Read failed");
	}
	
	int index = 0;
	char *end_str;

	// Preparo la stringa del tempo
	time_t now = time(NULL);
	struct tm local = *localtime(&now);
	ssize_t timeLength = (9 * sizeof(char));
	char *appendString = malloc(timeLength);
	sprintf(appendString, "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);

	//dimensione campo testo
	int offsetLength = TrafficInfoLength;
	int found = 0;

	//prendo la riga che è delimitata dal carattere \n
	char *row = strtok_r(buf, "\n", &end_str);
	char *rigaCambiare;

	//scorro finchè la riga non è finita
	while (row != NULL && found==0)
	{
		rigaCambiare=(char*)malloc(strlen(row));
		strcpy(rigaCambiare,row);
		char *end_segment;
		char *segment = strtok_r(row, ";", &end_segment);
		int campo=0;

		offsetLength+=strlen(rigaCambiare)+1;

		while (segment!=NULL && found==0)
		{	
			if(campo==0 && atoi(segment) == message.id){
				found = 1;
				break;
			}
			campo++;
			segment = strtok_r(NULL, ";", &end_segment);
		}

		index++;
		row = strtok_r(NULL, "\n", &end_str);
		free(rigaCambiare);
		if(found ==1)break;
	}
	close(fp);

	if(found == 1){
		offsetLength -= timeLength +1;
		fp = open(file, O_WRONLY, S_IRUSR | S_IWUSR);
		if (fp == -1)
			ErrExit("Open failed");
		if (lseek(fp, (size_t)(offsetLength) * sizeof(char) , SEEK_SET) == -1)
			ErrExit("Lseek failed");

		if (write(fp, appendString, strlen(appendString)) != strlen(appendString))
		{
			ErrExit("Write failed");
		}
		close(fp);
	}
	free(appendString);
	semOp(semID, INFOMESSAGEFILE, 1);
}


//Funzione che mi genera l' header di F10
void writeF10Header()
{
	//creo il file se è gia presente lo sovrascrivo
	int fp = open(F10, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer

	//inizializzo buffer delle dimenisoni corrette
	ssize_t bufferLength = sizeof(char) * F10Header;
	char *buffer = malloc(bufferLength);

	//converto i dati in stringa
	sprintf(buffer, "IPC;IDKey;Creator;CreationTime;DestructionTime\n");

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}
//Scrive una nuova riga nel file F10 --> tempi creazione IPC
void appendInF10(char * buffer, ssize_t bufferLength, int iteration)
{
	//se sono il primo a chiamare tale funzione, genero anche l'header
	if(iteration==1){
		writeF10Header();
	}
	int fp = open(F10, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}

//Serve per aggiornare l'ora in cui la IPC è stata distrutta
void completeInF10(char * searchBuffer) {

	//apro il file 
	int fp = open(F10, O_RDONLY);
	if (fp == -1)
		ErrExit("Open failed");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { ErrExit("Lseek failed"); }

	// posiziono l'offset alla prima riga (salto i titoli) 
	if (lseek(fp, (size_t)F10Header * sizeof(char), SEEK_SET) == -1) {
		ErrExit("Lseek failed");
	}

	int bufferLength = fileSize / sizeof(char) - F10Header;
	char buf[bufferLength];
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		ErrExit("Read failed");
	}

	// Preparo la stringa del tempo
	time_t now = time(NULL);
	struct tm local = *localtime(&now);
	ssize_t timeLength = (9 * sizeof(char));
	char *appendString = malloc(timeLength);
	sprintf(appendString, "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);

	//dimensione campo testo
	int offsetLength = F10Header;
	int found = 0;

	//prendo la riga che è delimitata dal carattere \n
	char *end_str;
	char *row = strtok_r(buf, "\n", &end_str);
	char *rigaCambiare;

	//scorro finchè la riga non è finita
	while (row != NULL && found==0)
	{
		rigaCambiare=(char*)malloc(strlen(row));
		strcpy(rigaCambiare,row);
		char *end_segment;
		char *segment = strtok_r(row, ";", &end_segment);
		int campo=0;

		offsetLength+=strlen(rigaCambiare)+1;

		while (segment!=NULL && found==0)
		{	
			if(campo==0 && strcmp(segment, searchBuffer)==0){
				found = 1;
				offsetLength-=10; //tolgo lo spiazzamento per scrivere la nuova ora

			}

			campo++;
			segment = strtok_r(NULL, ";", &end_segment);

		}

		if(found == 1)break;
		row = strtok_r(NULL, "\n", &end_str);
		free(rigaCambiare);
	}
	close(fp);



	fp = open(F10, O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open failed");
	if (lseek(fp, (size_t)(offsetLength) * sizeof(char) , SEEK_SET) == -1)
		ErrExit("Lseek failed");

	if (write(fp, appendString, strlen(appendString)) != strlen(appendString))
	{
		ErrExit("Write failed");
	}
	close(fp);
	free(appendString);
}

//Boh
char * timestamp(){
	time_t now = time(NULL); 
	char * time = asctime(gmtime(&now));
	time[strlen(time)-1] = '\0';
	int spaceCounter = 0;
	for(int i = 0; i<strlen(time); i++){
		if(time[i] == ' '){
			spaceCounter ++;
		}	
		if(spaceCounter == 3){
			for(int j = 0; j<8; j++){
				i++;
				time[j]=time[i];
			}
			for(int x = 8; x<strlen(time); x++)
				time[x] = '\0';
			break;
		}
	}
	return time;
}

void initSignalFather(void (*handler)(int)){
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
	sigact.sa_handler = handler;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
};

void initSignalMedium(void (*handler)(int)){
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
	sigact.sa_handler = handler;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
};

void initSignalChild(void (*handler)(int)){
	sigset_t mySet;
	sigfillset(&mySet);
	sigdelset(&mySet, SIGALRM);
	sigdelset(&mySet, SIGUSR1);
	sigdelset(&mySet, SIGUSR2);
	sigdelset(&mySet, SIGTERM);
	sigprocmask(SIG_SETMASK, &mySet, NULL);

	struct sigaction sigact;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = handler;
	sigaction(SIGALRM, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
};
