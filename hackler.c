/// @file client.c

/// @brief Contiene l'implementazione del client.



#include "defines.h"



int semID = -1;

char* F7;

action_group* carica_F7(char*);

void carica_PIDS(char*,int,int[]);

void writeActionReverse(char *,action_group*);



int main(int argc, char * argv[]) {

  //creo semaforo, sarà lo stesso del sender
  semID = semget(SKey, SEMNUMBER, IPC_CREAT | S_IRUSR | S_IWUSR);
  //finchè il sender non ha creato le IPC aspetto
  semOp(semID, HACKLERRECEIVER, -1);
  semOp(semID, HACKLERSENDER, -1);

  //acquisisco parametro passato da linea di comando
  F7 = argv[1];

  if(argc < 1){
    exit(1);
  }

  //inizializzo la struttura leggendo i dati dal file
  action_group* action_group = carica_F7(F7);
  int i = 0;


  //due array che contengono il pid dei receiver e i pid dei sender
  int senderPids[3];
  int receiverPids[3];

  //le inizializzo sfruttando le funzioni
  carica_PIDS(F8,SenderPIDHeader,senderPids);
  carica_PIDS(F9,ReceiverPIDHeader,receiverPids);

  //scorro le azioni dell'hackler
  for(i=0; i<action_group->length; i++){

    if(strcmp(action_group->actions[i].action,"ShutDown")==0){
      pid_t actionProcess= fork();
      if(actionProcess == 0){
        sleep(action_group->actions[i].delay + 30);
        kill(senderPids[0],SIGINT);
        kill(senderPids[1],SIGINT);
        kill(senderPids[2],SIGINT);
        kill(receiverPids[0],SIGINT);
        kill(receiverPids[1],SIGINT);
        kill(receiverPids[2],SIGINT);
        exit(0);
      }
    } else if(strcmp(action_group->actions[i].action,"IncreaseDelay")==0){
      pid_t actionProcess= fork();
      if(actionProcess == 0){
        sleep(action_group->actions[i].delay);
        sleep(5);
        exit(0);
      }
    }

  }



  // Eliminazione della struttura dei messaggi di hackler

  for(i = 0; i < action_group->length; i++){
    free(action_group->actions[i].target);
    free(action_group->actions[i].action);
  }

  free(action_group->actions);
  free(action_group);

  return 0;
}

void carica_PIDS(char nomeFile[], int lunghezzaHeader, int pids[]) {
  //apro il file 
  int fp = open(nomeFile, O_RDONLY);
  if (fp == -1)
    ErrExit("Open");

  // utilizzo lseek per calcolarne le dimensioni 
  int fileSize = lseek(fp, (size_t)0, SEEK_END);
  if (fileSize == -1) { ErrExit("Lseek"); }

  // posiziono l'offset alla prima riga delle azioni (salto i titoli) 
  if (lseek(fp, (size_t)lunghezzaHeader * sizeof(char), SEEK_SET) == -1) {
    ErrExit("Lseek");
  }

  //calcolo la dimensione del file da leggere a cui tolgo i "titoli" dei vari campi
  int bufferLength = fileSize / sizeof(char) - lunghezzaHeader;

  //inizializzo il buffer
  char buf[bufferLength];

  //leggo dal file e salvo ciò che ho letto nel buf
  if ((read(fp, buf, bufferLength * sizeof(char)) == -1)) {
    ErrExit("Read");
  }

  //numero di action che inserisco
  int index = 0;

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
    while (segment!=NULL)
    {	
      //memorizzo il segmento ne rispettivo campo della struttura

      switch (campo) {
        case 1:
          pids[index] = atoi(segment);
          break;
        default:
          break;
      }

      //vado al campo successivo
      campo++;
      segment = strtok_r(NULL, ";", &end_segment);
    }

    //vado alla riga successiva
    index++;
    row = strtok_r(NULL, "\n", &end_str);
  }
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

  action* actions = malloc(sizeof(action) * (rowNumber));

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
    while (segment!=NULL)
    {	
      //se il segmento è vuoto, faccio inserire una stringa vuota
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
          segment[strlen(segment)-1]='\0';
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
