#include "defines.h"

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

//mi stampa l'i-esimo messaggio contenuto nella struttura
void printMessage(message_sending message)
{
	printf("%d %s %s %s %d %d %d %s\n", message.id, message.message, message.idSender, message.idReceiver, message.DelS1, message.DelS2, message.DelS3, message.Type);
}
//mi stampa l'i-esimo messaggio di hackler contenuto nella struttura
void printAction(action action)
{
	printf("%d %d %s %s\n", action.id, action.delay, action.target, action.action);
}
//Funzione utilizzata per scrivere in un file le informazioni dei messaggi, se message_group e' null stampa solo intestazione
void writeTraffic(char *pathName, message_group *messageG)
{

	//creo il file se è gia presente lo sovrascrivo
	int fp = open(pathName, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer
	ssize_t bufferLength = sizeof(char) * TrafficInfoLength;
	char *header = TrafficInfo;

	if (write(fp, header, bufferLength) != bufferLength)
	{
		ErrExit("Write");
	}

	//se voglio scrivere il F1
	if (strcmp(F1, pathName) == 0)
	{

		//genero il tempo attuale
		time_t now = time(NULL);
		struct tm local = *localtime(&now);

		// Per ogni messaggio
		for (int i = 0; i < messageG->length; i++)
		{
			//ogni messaggio avrà come tempo di arrivo il tempo attuale più il ritardo
			struct tm then_tm = *localtime(&now);
			then_tm.tm_sec += messageG->messages[i].DelS1;
			mktime(&then_tm);

			//calcolo la dimensione della riga da scrivere
			bufferLength = (numcifre(messageG->messages[i].id) + sizeof(messageG->messages[i].message) + sizeof(messageG->messages[i].idSender) + sizeof(messageG->messages[i].idReceiver) + 20 * sizeof(char));
			char *string = malloc(bufferLength);

			//mi salvo tutta la stringa
			sprintf(string, "%d;%s;%c;%c;%02d:%02d:%02d;%02d:%02d:%02d\n", messageG->messages[i].id, messageG->messages[i].message, messageG->messages[i].idSender[1], messageG->messages[i].idReceiver[1], local.tm_hour, local.tm_min, local.tm_sec, then_tm.tm_hour, then_tm.tm_min, then_tm.tm_sec);

			//scrivo la stringa nel file
			if (write(fp, string, strlen(string) * sizeof(char)) != strlen(string) * sizeof(char))
			{
				ErrExit("Write");
			}

			//libero lo spazio allocato per la stringa
			free(string);
		}
	}
	close(fp);
}

int stringLenght(message_sending message)
{
	return numcifre(message.id) + sizeof(message.message) + sizeof(message.idSender) + sizeof(message.idReceiver) + 20 * sizeof(char);
}
char *toString(message_sending message)
{
	char *string = malloc(stringLenght(message));
	sprintf(string, "%d %s %s %s %d %d %d %s\n", message.id, message.message, message.idSender, message.idReceiver, message.DelS1, message.DelS2, message.DelS3, message.Type);
	return string;
}
ssize_t dimensioneOfMessage(message_sending message)
{
	return 4 * sizeof(int) + sizeof(message.idReceiver) + sizeof(message.idSender) + sizeof(message.Type) + sizeof(message.message);
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
void printInfoMessage(message_sending message, struct tm timeArrival, char file[])
{

	//scrivo in append il messaggio al file già presente cosi non perdo i messaggi precedenti
	int fp = open(file, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//genero il tempo attuale --> TimeDeparture
	time_t now = time(NULL);
	struct tm TimeDeparture = *localtime(&now);

	//calcolo la dimensione della riga da scrivere
	ssize_t bufferLength = (numcifre(message.id) + sizeof(message.message) + sizeof(message.idSender) + sizeof(message.idReceiver) + 20 * sizeof(char));
	char *string = malloc(bufferLength);

	//mi salvo tutta la stringa
	sprintf(string, "%d;%s;%c;%c;%02d:%02d:%02d;%02d:%02d:%02d\n", message.id, message.message, message.idSender[1], message.idReceiver[1], timeArrival.tm_hour, timeArrival.tm_min, timeArrival.tm_sec, TimeDeparture.tm_hour, TimeDeparture.tm_min, TimeDeparture.tm_sec);

	//scrivo la stringa nel file
	if (write(fp, string, strlen(string) * sizeof(char)) != strlen(string) * sizeof(char))
	{
		ErrExit("Write");
	}

	//libero lo spazio allocato per la stringa
	free(string);

	close(fp);
}

void ordinaPerDel(message_group *messageG, char DEL[])
{
	int i;
	int ordinato = 0;
	message_sending tmp;
	int dim = messageG->length;

	while (dim > 1 && !ordinato)
	{
		ordinato = 1;
		for (i = 0; i < dim - 1; i++)
		{
			if (strcmp(DEL, "S1") == 0)
			{
				if (messageG->messages[i].DelS1 > messageG->messages[i + 1].DelS1)
				{
					tmp = messageG->messages[i];
					messageG->messages[i] = messageG->messages[i + 1];
					messageG->messages[i + 1] = tmp;
					ordinato = 0;
				}
			}
		}
		dim -= 1;
	}
}
void listen(int MSQID, int SHMID, int semID, char processo[])
{
	struct message_queue messaggio;
	struct request_shared_memory *request_shared_memory = (struct request_shared_memory *)get_shared_memory(SHMID, 0);
	time_t now = time(NULL);

	size_t mSize = sizeof(struct message_queue) - sizeof(long);
	while (1)
	{
		//genero tempo attuale
		struct tm timeArrival = *localtime(&now);
		//-------------------------------------------------- BLOCCO MESSAGE QUEUE --------------------------------------------------
		//prelevo il messaggio
		if (msgrcv(MSQID, &messaggio, mSize, 0, 0) == -1)
		{
			ErrExit("msgrcv failed");
		}
		else
		{
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
		}
		//-------------------------------------------------- FINE BLOCCO MESSAGE QUEUE --------------------------------------------------
		//-------------------------------------------------- BLOCCO SHARED MEMORY --------------------------------------------------
		
		

		semOp(semID, REQUEST, -1);
		//printf("Shared memory message: %s\n", toString(request_shared_memory->message));
		
		if (strcmp(processo, request_shared_memory->message.idReceiver) == 0)
			{
				
				printf("\nMsg received: %s", toString(request_shared_memory->message));
				if (strcmp(request_shared_memory->message.idReceiver, "R1") == 0)
				{
					//dormi
					sleep(request_shared_memory->message.DelS1);
					//stampa le info sul tuo file
					printInfoMessage(request_shared_memory->message, timeArrival, F6);
				}

				else if (strcmp(request_shared_memory->message.idReceiver, "R2") == 0)
				{
					sleep(request_shared_memory->message.DelS2);
					printInfoMessage(request_shared_memory->message, timeArrival, F5);
				}

				else if (strcmp(request_shared_memory->message.idReceiver, "R3") == 0)
				{
					sleep(request_shared_memory->message.DelS3);
					printInfoMessage(request_shared_memory->message, timeArrival, F4);
				}
			}
		semOp(semID, DATAREADY, 1);
		//-------------------------------------------------- FINE BLOCCO SHARED MEMORY --------------------------------------------------
	}
}