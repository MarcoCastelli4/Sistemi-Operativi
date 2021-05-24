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
void printInfoMessage(int semID, message_sending message, struct tm timeArrival, char file[])
{

	semOp(semID, INFOMESSAGEFILE, -1);
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
	semOp(semID, INFOMESSAGEFILE, 1);
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

void appendInF10(char * buffer, ssize_t bufferLength)
{
	//creo il file se è gia presente lo sovrascrivo
	int counter = 0;
	for(int i =0; i<strlen(buffer); i++){
		if(buffer[i] == ';'){
			counter++;
			if(counter == 4){
				buffer[i+1]='\n';
				buffer[i+2]='\0';
			}
		}
	}
	int fp = open(F10, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	//scrivo sul file
	write(fp, buffer, bufferLength);
	close(fp);
	free(buffer);
}

void completeInF10(char * searchBuffer) {
	//apro il file 
	int fp = open(F10, O_RDONLY);
	if (fp == -1)
		ErrExit("Open");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { ErrExit("Lseek"); }

	// posiziono l'offset alla prima riga delle azioni (salto i titoli) 
	if (lseek(fp, (size_t)F10Header * sizeof(char), SEEK_SET) == -1) {
		ErrExit("Lseek");
	}

	int bufferLength = fileSize / sizeof(char) - F10Header;
	char buf[bufferLength];
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		ErrExit("Read");
	}

	//numero di action che inserisco
	int index = 0;
	char *end_str;

	// Preparo la stringa del tempo
	time_t now = time(NULL);
	struct tm local = *localtime(&now);
	ssize_t timeLength = (10 * sizeof(char));
	char *appendString = malloc(timeLength);
	sprintf(appendString, "%02d:%02d:%02d\n", local.tm_hour, local.tm_min, local.tm_sec);

	int offsetLength = F10Header;
	int found = 0;

	//prendo la riga che è delimitata dal carattere \n
	char *row = strtok_r(buf, "\n", &end_str);
	//scorro finchè la riga non è finita
	while (row != NULL)
	{

		// Se non ho trovato aggiungo il numero di caratteri per cui spostarmi in avanti
		if(found == 0){
			offsetLength += strlen(row);
		}

		// Se ho trovato il campo ricercato mi sposto avanti e aggiungo alla stringa da incollare
		if(found == 1){
			strcat(appendString,row);
			strcat(appendString,"\n");
		}

		char *end_segment;
		char *segment = strtok_r(row, ";", &end_segment);
		int campo=0;
		while (segment!=NULL)
		{	
			if(found == 0 && campo == 0 && strcmp(segment, searchBuffer)==0){
				found = 1;
			}

			campo++;
			segment = strtok_r(NULL, ";", &end_segment);
		}

		index++;
		row = strtok_r(NULL, "\n", &end_str);
	}
	close(fp);

	fp = open(F10, O_WRONLY, S_IRUSR | S_IWUSR);
	if (fp == -1)
		ErrExit("Open");

	if (lseek(fp, (size_t)(offsetLength) * sizeof(char) + 1, SEEK_SET) == -1)
		ErrExit("Lseek");

	int i;
	for( i = strlen(appendString) - 1; !isdigit(appendString[i]) && appendString[i]!=';'; i--);
	appendString[i+1] = '\n';
	appendString[i+2] = '\0';
	
	if (write(fp, appendString, strlen(appendString)) != strlen(appendString))
	{
		print_log("ERRORE\n");
		ErrExit("Write");
	}
	close(fp);
	free(appendString);
}

char * timestamp(){
	time_t now = time(NULL); 
	char * time = asctime(gmtime(&now));
	time[strlen(time)-1] = '\0';    // Remove \n
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
