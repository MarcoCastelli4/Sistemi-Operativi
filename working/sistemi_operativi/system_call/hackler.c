/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"

char* F7;
action_group* carica_F7(char*);
void writeActionReverse(char *,action_group*);

int main(int argc, char * argv[]) {

  //acquisisco parametro passato da linea di comando
  F7 = argv[1];

  if(argc < 1){
    exit(1);
  }

  //inizializzo la struttura leggendo i dati dal file
  action_group* action_group = carica_F7(F7);
  
  //scrivo i dati della struttura in ordine inverso
  writeActionReverse(F7out, action_group);

  // Eliminazione della struttura dei messaggi di hackler
  free(action_group);

  //addormento il processo
  sleep(2);
  
  return 0;
}


action_group* carica_F7(char nomeFile[]) {

	//apro il file 
	int fp = open(nomeFile, O_RDONLY);
	if (fp == -1)
		ErrExit("Open");

	// utilizzo lseek per calcolarne le dimensioni 
	int fileSize = lseek(fp, (size_t)0, SEEK_END);
	if (fileSize == -1) { ErrExit("Lseek"); }

	// posiziono l'offset alla prima riga delle azioni (salto i titoli) 
	if (lseek(fp, (size_t)ActionSendingHeader * sizeof(char), SEEK_SET) == -1) {
		ErrExit("Lseek");
	}


	//calcolo la dimensione del file da leggere a cui tolgo i "titoli" dei vari campi
	int bufferLength = fileSize / sizeof(char) - ActionSendingHeader;
	//inizializzo il buffer
	char buf[bufferLength];
	//leggo dal file e salvo ciò che ho letto nel buf
	if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
		ErrExit("Read");
	}

	//inizializzo l'array riga
	char row[bufferLength];

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F7 (corrispondono al numero di messaggi presenti)
	for (int i = 0; i < bufferLength; i++) {
		if (buf[i] == '\n' || buf[i] == '\0' || i == bufferLength - 1) {
			rowNumber++;
		}
	}
	//allochiamo dinamicamente un array di azioni delle dimensioni opportune
	action* actions = malloc(sizeof(action) * (rowNumber + 1));

	//contatore 
	int counter = 0;
	//numero di action che inserisco
	int actionNumber = 0;

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
			//j ci dice a quale colonna siamo da 0..7 (id,delay,...)
			int campo = 0;
			//x è la posizione in cui si trova il carattere che stiamo prendendo in questione
			int x = 0;
			// Per ognuno dei campi
			while (campo < 4) {
				//posizione in cui si trova il primo carattere del campo in questione
				int oldX = x;
				//inizializzo questa variabile per il calcolo della lunghezza del campo (id,..delay,..)
				int segmentLength = 0;
				//calcolo la lunghezza del campo
				while (row[x] != ';' && row[x] != '\n' && row[x] != '\0') {
					segmentLength++;
					x++;
				}

				//ritorno al caraterre iniziale del campo
				x = oldX;
				//alloco una stringa (che contiene il mio campo) della dimensione corretta
				char* segment = malloc(sizeof(char) * segmentLength);

				counter = 0;
				// Ottengo il campo
				while (row[x] != ';' && row[x] != '\n' && row[x] != '\0') {
					segment[counter] = row[x];
					counter++;
					x++;
				}
				segment[counter] = '\0';
				x++;
				// Inizializzo i campi del messaggio hackler
				switch (campo) {
				case 0:
					actions[actionNumber].id = atoi(segment);
					break;
				case 1:
					actions[actionNumber].delay = atoi(segment);
					break;
				case 2:
					actions[actionNumber].target = (char*)malloc(sizeof(segment));
					strcpy(actions[actionNumber].target, segment);
					break;
				case 3:
					actions[actionNumber].action = (char*)malloc(sizeof(segment));
					strcpy(actions[actionNumber].action, segment);
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
			actionNumber++;

			counter = 0;
		}
	}
	//inserisco nella mia struttura l'array di messaggi e quanti messaggi sono stati inseriti
	action_group* actionG = malloc(sizeof(actionG));
	actionG->length = actionNumber;
	actionG->actions = actions;

	return actionG;
}


//Funzione utilizzata per scrivere in un file le informazioni delle azioni
void writeActionReverse(char* pathName, action_group* actionG) {

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

	// Per ogni messaggio
	for (int i = actionG->length - 1;i >= 0; i--) {

    //calcolo la dimensione della riga da scrivere
    bufferLength = (numcifre(actionG->actions[i].id) + (numcifre(actionG->actions[i].delay) +sizeof(actionG->actions[i].target) + sizeof(actionG->actions[i].action) + 8 * sizeof(char)));
    char* string = malloc(bufferLength);

    //mi salvo tutta la stringa
    sprintf(string, "%d;%d;%s;%s\n", actionG->actions[i].id, actionG->actions[i].delay, actionG->actions[i].target, actionG->actions[i].action);

    //scrivo la stringa nel file
    if (write(fp, string, strlen(string) * sizeof(char)) != strlen(string) * sizeof(char)) {
      ErrExit("Write");
    }

    //libero lo spazio allocato per la stringa
    free(string);
	}

	close(fp);
}
