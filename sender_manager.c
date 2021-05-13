#include "defines.h"
#include "shared_memory.h"
#include <signal.h>

char *F0;
int MSQID = -1;
int SHMID = -1;
int semID;
struct request_shared_memory *request_shared_memory;

void writeF8(int, int, int);
message_group *carica_F0(char[]);

void sendMessage(message_group *messageG, char processo[]);

int main(int argc, char *argv[])
{
	signal(SIGINT, sigHandler);
	pid_t pidS1, pidS2, pidS3;
	pid_t waitPID;

	//Inizializzo il semaforo e attendo
	semID = create_sem_set(6);

	// Creo la message queue
	MSQID = msgget(QKey, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (MSQID == -1)
	{
		ErrExit("Message queue failed");
	}

	//Creazione della shared memory
	SHMID = alloc_shared_memory(MKey, sizeof(struct request_shared_memory));

	request_shared_memory = (struct request_shared_memory *)get_shared_memory(SHMID, 0);

	//ho creato puoi usarle
	semOp(semID, CREATION, 1);
	F0 = argv[1];

	if (argc < 1)
	{
		exit(1);
	}
	//la struttura messaggi inizialmente è vuota
	message_group *messages = NULL;

	//genero processo S1
	pidS1 = fork();
	if (pidS1 == 0)
	{
		//inizializzo la struttura con la dimensione di un messaggio

		messages = carica_F0(F0);

		//scrivo intestazione
		printIntestazione(F1);
		//mando tutti i messaggi
		sendMessage(messages, "S1");

		printf("MORTO S1\n");
		exit(0);
		//termino il processo
	}
	else if (pidS1 == -1)
	{
		ErrExit("Fork");
	}
	//genero processo S2
	pidS2 = fork();
	if (pidS2 == 0)
	{
		//scrivo sul file F2
		writeTraffic(F2, NULL);

		sendMessage(messages, "S2");

		printf("MORTO S2\n");
		//termino il processo
		exit(0);
	}
	else if (pidS2 == -1)
	{
		ErrExit("Fork");
	}

	//genero processo S3
	pidS3 = fork();
	if (pidS3 == 0)
	{
		//scrivo sul file F3
		writeTraffic(F3, NULL);
		//addormento per 3 secondo il processo
		sendMessage(messages, "S3");

		printf("MORTO S3\n");
		exit(0);
	}
	else if (pidS3 == -1)
	{
		ErrExit("Fork");
	}

	//genero file8.csv
	writeF8(pidS1, pidS2, pidS3);
	semOp(semID, HACKLERSENDER, 1);

	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0)
		;

	//aspetto che il receiver finisca di usare le IPC
	semOp(semID, ELIMINATION, -1);

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
	if (lseek(fp, (size_t)(MessageSendingHeader + 1) * sizeof(char), SEEK_SET) == -1)
	{
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
	message_sending *messages = malloc(sizeof(message_sending) * (rowNumber));

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
				messages[messageNumber].id = atoi(segment);
				break;
			case 1:

				strcpy(messages[messageNumber].message, segment);
				break;
			case 2:
				strcpy(messages[messageNumber].idSender, segment);
				break;
			case 3:

				strcpy(messages[messageNumber].idReceiver, segment);
				break;
			case 4:
				messages[messageNumber].DelS1 = atoi(segment);
				break;
			case 5:
				messages[messageNumber].DelS2 = atoi(segment);
				break;
			case 6:
				messages[messageNumber].DelS3 = atoi(segment);
				break;
			case 7:
				segment[strlen(segment) - 1] = '\0'; //perchè altrimenti mi rimane un carattere spazzatura in più
				strcpy(messages[messageNumber].Type, segment);
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
	messageG->messages = messages;

	return messageG;
}

void sendMessage(message_group *messageG, char processo[])
{
	time_t now = time(NULL);
	int i;
	int sleepTotale = 0;
	//scorro tutti i messaggi

	//se sono il processo 1 ordino per DELS1
	if (strcmp(processo, "S1") == 0)
		ordinaPerDel(messageG, "S1");
	for (i = 0; i < messageG->length; i++)
	{
		struct message_queue m;
		m.mtype = 1;

		memcpy(&m.message, &messageG->messages[i], sizeof messageG->messages[i]);
		size_t mSize = sizeof(struct message_queue) - sizeof(long);

		//genero tempo attuale
		struct tm timeArrival = *localtime(&now);

		//ritardo il messaggio
		if (strcmp(processo, "S1") == 0)
		{
			//printf("Sto dormendo per %d secondi",messageG->messages[i].DelS1-sleepTotale);
			sleep(messageG->messages[i].DelS1 - sleepTotale); //dormi per quanto ti manca

			//incremento lo sleep totale
			sleepTotale += messageG->messages[i].DelS1 - sleepTotale;
		}

		//stampa su file F1
		printInfoMessage(messageG->messages[i], timeArrival, F1);

		//se sono nel processo sender corretto
		if (strcmp(processo, messageG->messages[i].idSender) == 0)
		{
			//viene inviato tramite message queue
			if (strcmp(messageG->messages[i].Type, "Q") == 0)
			{
				// sending the message in the queue
				if (msgsnd(MSQID, &m, mSize, 0) == -1)
					ErrExit("msgsnd failed");
				semOp(semID, REQUEST, 1);
			}
			//viene inviato tramite shared memory
			else if (strcmp(messageG->messages[i].Type, "SH") == 0)
			{
				semOp(semID, REQUEST, 1);
				memcpy(request_shared_memory, &messageG->messages[i], sizeof(messageG->messages[i]));
				semOp(semID, DATAREADY, -1);
			}
			else if ((strcmp(processo, "S3") == 0) && (strcmp(messageG->messages[i].Type, "FIFO") == 0))
			{
				//invia a R3 tramite FIFO
				//ELIMINARE E' DI PROVA
				if (msgsnd(MSQID, &m, mSize, 0) == -1)
					ErrExit("msgsnd failed");
				semOp(semID, REQUEST, 1);
			}
		}
		//non sono nel processo sender corretto, seguo la catena di invio
		else
		{

			semOp(semID, REQUEST, 1);
			memcpy(request_shared_memory, &messageG->messages[i], sizeof(messageG->messages[i]));
			semOp(semID, DATAREADY, -1);
			/*
			if (msgsnd(MSQID, &m, mSize, 0) == -1)
					ErrExit("msgsnd failed");*/

			//viene inviato tramite PIPE, fino a che non raggiunge il sender corretto con il quale partità con modalità Type
			/** if (strcmp(processo, "S1") == 0)
				* {
				*   //invia a S2 tramite PIPE
				*   //ELIMINARE E' DI PROVA
				*   semOp(semID, REQUEST, 1);
				*   memcpy(request_shared_memory, &messageG->messages[i], sizeof(messageG->messages[i]));
				*   semOp(semID, DATAREADY, -1);
				* }
				* if (strcmp(processo, "S2") == 0)
				* {
				*   //invia a S3 tramite PIPE
				*   //ELIMINARE E' DI PROVA
				*   semOp(semID, REQUEST, 1);
				*   memcpy(request_shared_memory, &messageG->messages[i], sizeof(messageG->messages[i]));
				*   semOp(semID, DATAREADY, -1);
				* }
				* if (strcmp(processo, "S3") == 0)
				* {
				*   //invia a S3 tramite PIPE
				*   //ELIMINARE E' DI PROVA
				*   semOp(semID, REQUEST, 1);
				*   memcpy(request_shared_memory, &messageG->messages[i], sizeof(messageG->messages[i]));
				*   semOp(semID, DATAREADY, -1);
				* } */
		}
	}
}
