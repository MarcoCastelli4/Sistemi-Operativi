/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"

action_group* carica_F7(char*);

int main(int argc, char * argv[]) {

  //inizializzo la struttura con la dimensione di un messaggio
  action_group* action_group = carica_F7(F7);

  for(int i=0; i<action_group.length; i++)
    printAction(action_group.actions[i]);

  //sleep(2);
  
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
				// Inizializzo i campi del messaggio
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

  printAction(actions[0]);

	return actionG;
}
