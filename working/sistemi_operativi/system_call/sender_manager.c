#include "defines.h"

char* F0;

void writeF8(int, int, int);
message_group* carica_F0(char[]);

int main(int argc, char * argv[]) {
	pid_t pidS1, pidS2, pidS3;
	pid_t waitPID;
  
  F0 = argv[1];

  if(argc < 1){
    exit(1);
  }
	//la struttura messaggi inizialmente è vuota
	message_group* messages=NULL;

	//genero processo S1
	pidS1 = fork();
	if (pidS1 == 0) {

		//inizializzo la struttura con la dimensione di un messaggio
		messages = carica_F0(F0);
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

	// Eliminazione dei messaggi
	free(messages);


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
	int n = 26 + numcifre(pid1) + numcifre(pid2) + numcifre(pid3);

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
		printf("Open");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { printf("Lseek"); }

	// posiziono l'offset alla prima riga dei messaggi (salto i titoli) 
	if (lseek(fp, (size_t)MessageSendingHeader * sizeof(char), SEEK_SET) == -1) {
		printf("Lseek");
	}


	//calcolo la dimensione del file da leggere a cui tolgo i "titoli" dei vari campi
	int bufferLength = fileSize / sizeof(char) - MessageSendingHeader;
	//inizializzo il buffer
	char buf[bufferLength];
	//leggo dal file e salvo ciò che ho letto nel buf
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		printf("Read");
	}

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F0 (corrispondono al numero di messaggi presenti)
	for (int i = 0; i < bufferLength; i++) {
		if (buf[i] == '\n' || buf[i] == '\0' || i == bufferLength - 1) {
			rowNumber++;
		}
	}
	//allochiamo dinamicamente un array di azioni delle dimensioni opportune
	message_sending* messages = malloc(sizeof(message_sending) * (rowNumber + 1));

	
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
			int campo=0; //(0..7, id, message..)
			//scorro finchè il campo non è finito (la casella)
       			 while (segment!=NULL)
       			 {
				//memorizzo il segmento ne rispettivo campo della struttura
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
				//vado al campo successivo
				campo++;
       			     segment = strtok_r(NULL, ";", &end_segment);

       			 }
			//vado alla riga successiva
			messageNumber++;
       			 row = strtok_r(NULL, "\n", &end_str);
	}
			
	//inserisco nella mia struttura l'array di messaggi e quanti messaggi sono stati inseriti
	message_group* messageG = malloc(sizeof(messageG));
	messageG->length = messageNumber;
	messageG->messages = messages;

	return messageG;
}


