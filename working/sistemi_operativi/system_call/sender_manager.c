#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "defines.h"
#include "err_exit.h"

void writeF8(int, int, int);
void writeTraffic(char*, message_group*);
message_group* carica_F0(char[]);

int main(void) {
	pid_t pidS1, pidS2, pidS3;
	pid_t waitPID;

	//genero processo S1
	pidS1 = fork();
	if (pidS1 == 0) {

		//inizializzo la struttura con la dimensione di un messaggio
		message_group* messages = carica_F0(F0);
		//scrivo sul file F1
		writeTraffic(F1, messages);
		//addormento per 1 secondo il processo
		sleep(1);
		exit(0);
		//termino il processo
	}
	else if (pidS1 == -1) {
		ErrExit("Fork");
	}

	//genero processo S2
	pidS2 = fork();
	if (pidS2 == 0) {
		//scrivo sul file F2
		writeTraffic(F2, NULL);
		//addormento per 2 secondo il processo
		sleep(2);
		//termino il processo
		exit(0);
	}
	else if (pidS2 == -1) {
		ErrExit("Fork");
	}


	//genero processo S3
	pidS3 = fork();
	if (pidS3 == 0) {
		//scrivo sul file F3
		writeTraffic(F3, NULL);
		//addormento per 2 secondo il processo
		sleep(3);
		//termino il processo
		exit(0);
	}
	else if (pidS3 == -1) {
		ErrExit("Fork");
	}


	//genero file8.csv 
	writeF8(pidS1, pidS2, pidS3);


	/** attendo la terminazione dei sottoprocessi prima di continuare */
	int stato = 0;
	while ((waitPID = wait(&stato)) > 0);
	//termino il processo padres
	exit(0);
	return(0);
}



//Funzione che mi genera il file F8 e scrive ogni riga
void writeF8(int pid1, int pid2, int pid3) {

	//creo il file se è gia presente lo sovrascrivo
	int fp = open(F8, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer
	int n = 25 + numcifre(pid1) + numcifre(pid2) + numcifre(pid3);

	//inizializzo buffer delle dimenisoni corrette
	ssize_t bufferLength = sizeof(char) * n;
	char* buffer = malloc(bufferLength);

	//converto i dati in stringa
	sprintf(buffer, "Sender Id;PID\nS1;%d\nS2;%d\nS3;%d\n", pid1, pid2, pid3);

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}

message_group* carica_F0(char nomeFile[]) {


	//apro il file 
	int fp = open(nomeFile, O_RDONLY);
	if (fp == -1)
		ErrExit("Open");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { ErrExit("Lseek"); }

	// riposiziono l'offset di lettura a inizio file 
	if (lseek(fp, (size_t)MessageSendingHeader * sizeof(char), SEEK_SET) == -1) {
		ErrExit("Lseek");
	}


	//calcolo la dimensione del file da leggere a cui tolgo i "titoli" dei vari campi
	int bufferLength = fileSize / sizeof(char) - MessageSendingHeader;
	//inizializzo il buffer
	char buf[fileSize / sizeof(char)];
	//leggo dal file e salvo ciò che ho letto nel buf
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		ErrExit("Read");
	}

	//inizializzo l'array riga
	char row[1000];

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F0 (corrispondono al numero di messaggi presenti)
	for (int i = 0; i < bufferLength; i++) {
		if (buf[i] == '\n' || buf[i] == '\0' || i == bufferLength - 1) {
			rowNumber++;
		}
	}
	//allochiamo dinamicamente un array di messaggi delle dimensioni opportune
	message_sending* messages = malloc(sizeof(message_sending) * (rowNumber + 1));

	//contatore 
	int counter = 0;
	//numero di messaggi che inserisco
	int messageNumber = 0;

	//scorriamo "buf" che è ciò che abbiamo ottenuto con la lettura del file
	for (int i = 0; i < bufferLength; i++) {
		// Mi creo la stringa "row" contenente i dati di una singola riga
		if (buf[i] != '\n' && buf[i] != '\0' && i != bufferLength - 1) {
			row[counter] = buf[i];
			//avanzo per il prossimo carattere
			counter++;
		}
		else {
			row[counter] = '\0';
			//j ci dice a quale colonna siamo da 0..7 (id,message,...)
			int campo = 0;
			//x è la posizione in cui si trova il carattere che stiamo prendendo in questione
			int x = 0;
			// Per ognuno dei campi
			while (campo < 8) {
				//posizione in cui si trova il primo carattere del campo in questione
				int oldX = x;
				//inizializzo questa variabile per il calcolo della lunghezza del campo (id,..message,..)
				int segmentLength = 0;
				//calcolo la lunghezza del campo
				while (row[x] != ';' && row[x] != '\n' && row[x] != '\0' && row[x] != NULL) {
					segmentLength++;
					x++;
				}

				//ritorno al caraterre iniziale del campo
				x = oldX;
				//alloco una stringa (che contiene il mio campo) della dimensione corretta
				char* segment = malloc(sizeof(char) * segmentLength);

				counter = 0;
				// Ottengo il campo
				while (row[x] != ';' && row[x] != '\n' && row[x] != '\0' && row[x] != NULL) {
					segment[counter] = row[x];
					counter++;
					x++;
				}
				segment[counter] = '\0';
				x++;
				// Inizializzo i campi del messaggio
				switch (campo) {
				case 0:
					messages[messageNumber].id = atoi(segment);
					break;
				case 1:
					messages[messageNumber].message = (char*)malloc(sizeof(segment));
					strcpy(messages[messageNumber].message, segment);
					break;
				case 2:
					messages[messageNumber].idSender = (char*)malloc(sizeof(segment));
					strcpy(messages[messageNumber].idSender, segment);
					break;
				case 3:
					messages[messageNumber].idReceiver = (char*)malloc(sizeof(segment));
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
					messages[messageNumber].Type = (char*)malloc(sizeof(segment));
					strcpy(messages[messageNumber].Type, segment);
					break;
				default:
					break;
				}
				free(segment);
				campo++;	//passo al campo successivo
			}

			// Resetto la riga
			strcpy(row, "");
			//aumento il contatore dei messaggi
			messageNumber++;

			counter = 0;
		}
	}
	//inserisco nella mia struttura l'array di messaggi e quanti messaggi sono stati inseriti
	message_group* messageG = malloc(sizeof(messageG));
	messageG->length = messageNumber;
	messageG->messages = messages;

	return messageG;
}

//Funzione che mi genera il file F8 e scrive ogni riga
void writeTraffic(char* pathName, message_group* messageG) {

	//creo il file se è gia presente lo sovrascrivo
	int fp = open(pathName, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//calcolo il numero totale di caratteri da scrivere nel buffer
	ssize_t bufferLength = sizeof(char) * TrafficInfoLength;
	char* header = TrafficInfo;

	if (write(fp, header, bufferLength) != bufferLength) {
		ErrExit("Write");
	}

	//se voglio scrivere il F1
	if (strcmp(F1, pathName) == 0) {

		//genero il tempo attuale
		time_t now = time(NULL);
		struct tm local = *localtime(&now);

		// Per ogni messaggio
		for (int i = 0;i < messageG->length; i++) {
			//ogni messaggio avrà come tempo di arrivo il tempo attuale più il ritardo
			struct tm then_tm = *localtime(&now);
			then_tm.tm_sec += messageG->messages[i].DelS1;
			mktime(&then_tm);

			//calcolo la dimensione della riga da scrivere
			bufferLength = (numcifre(messageG->messages[i].id) + sizeof(messageG->messages[i].message) + sizeof(messageG->messages[i].idSender) + sizeof(messageG->messages[i].idReceiver) + 20 * sizeof(char));
			char* string = malloc(bufferLength);

			//mi salvo tutta la stringa
			sprintf(string, "%d;%s;%c;%c;%02d:%02d:%02d;%02d:%02d:%02d\n\0", messageG->messages[i].id, messageG->messages[i].message, messageG->messages[i].idSender[1], messageG->messages[i].idReceiver[1], local.tm_hour, local.tm_min, local.tm_sec, then_tm.tm_hour, then_tm.tm_min, then_tm.tm_sec);

			//scrivo la stringa nel file
			if (write(fp, string, strlen(string) * sizeof(char)) != strlen(string) * sizeof(char)) {
				ErrExit("Write");
			}

			//libero lo spazio allocato per la stringa
			free(string);
		}

		// Eliminazione dei messaggi
		free(messageG);
	}
	close(fp);
}




