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
  for(int i = 0; i < action_group->length; i++){
    free(action_group->actions[i].target);
    free(action_group->actions[i].action);
  }
  free(action_group->actions);
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

	//contatore delle righe
	int rowNumber = 0;

	// Contiamo il numero di righe presenti nel F7 (corrispondono al numero di azioni hackler presenti)
	for (int i = 0; i < bufferLength; i++) {
		if (buf[i] == '\n' || buf[i] == '\0' || i == bufferLength - 1) {
			rowNumber++;
		}
	}
	//allochiamo dinamicamente un array di azioni delle dimensioni opportune
	action* actions = malloc(sizeof(action) * (rowNumber + 1));

	
	//numero di action che inserisco
	int actionNumber = 0;

	char *end_str;
	//prendo la riga che è delimitata dal carattere \n
  char *row = strtok_r(buf, "\n", &end_str);
	//scorro finchè la riga non è finita
  while (row != NULL)
    {
			char *end_segment;
      //prendo il singolo campo/segmento che è delimitato dal ;
      char *segment = strtok_r(row, ";", &end_segment);
			int campo=0; //0->id , 1->delay
			//scorro finchè non ho aggiunto i 4 campi
        while (campo<4)
        {	//se il segmento è vuoto, faccio inserire una stringa vuota
          if(segment==NULL)
            segment="";
          //memorizzo il segmento ne rispettivo campo della struttura
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
          //vado al campo successivo
          campo++;
          segment = strtok_r(NULL, ";", &end_segment);

        }
			//vado alla riga successiva
			actionNumber++;
      row = strtok_r(NULL, "\n", &end_str);
	}
			
	//inserisco nella mia struttura l'array di hackler e quanti hackler sono stati inseriti
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
	ssize_t bufferLength = sizeof(char) * ActionSendingHeader;
	char* header = HacklerInfo;

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
