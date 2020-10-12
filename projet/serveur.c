// serveur.c
#include "pse.h"

#define CMD         "serveur"
#define PSEUDOS "pseudos.log"
#define PASSWORDS "passwords.log"
#define TIMELINE "timeline.log"
#define NB_WORKERS  2 




DataSpec dataSpec[NB_WORKERS];
sem_t semWorkersLibres;

int pseudos;
int passwords;
int timeline;

int fermeture=0;

pthread_mutex_t monmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCanal[NB_WORKERS];




void creerCohorteWorkers(void);
int chercherWorkerLibre(void);
void *threadWorker(void *arg);
void sessionClient(int canal);
void lockMutexCanal(int numWorker);
void unlockMutexCanal(int numWorker);


void creer_compte(int canal);
void connexion_ou_creer_compte(int canal);
int verification_pseudo(char ligne[LIGNE_MAX], int compteur[]);
int verification_password(char ligne[LIGNE_MAX], int compteur[]);
void verification_pseudo_password(int canal);
void afficher_timeline(int canal);
void ecrire_timeline(int canal);
void menu(int canal);
void horaire(void);
int existence_pseudo(char pseudo[]);





int main(int argc, char *argv[]) {
  short port;
  int ecoute, canal, ret;
  struct sockaddr_in adrEcoute, adrClient;
  unsigned int lgAdrClient;
  int numWorkerLibre;

  if (argc != 2)
    erreur("usage: %s port\n", argv[0]);

  port = (short)atoi(argv[1]);

  pseudos = open(PSEUDOS, O_CREAT|O_RDWR , 0644);
  if (pseudos == -1)
    erreur_IO("ouverture identifiants");

  passwords = open(PASSWORDS, O_CREAT|O_RDWR , 0644);
  if (passwords == -1)
    erreur_IO("ouverture identifiants");

  timeline = open(TIMELINE, O_CREAT|O_RDWR , 0644);
  if (passwords == -1)
    erreur_IO("ouverture timeline");

  creerCohorteWorkers();

  ret = sem_init(&semWorkersLibres, 0, NB_WORKERS);
  if (ret == -1)
    erreur_IO("init semaphore workers libres");

  printf("%s: creating a socket\n", CMD);
  ecoute = socket(AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(port);
  printf("%s: binding to INADDR_ANY address on port %d\n", CMD, port);
  ret = bind(ecoute, (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  printf("%s: listening to socket\n", CMD);
  ret = listen(ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");

 




  while (VRAI) {
    printf("%s: accepting a connection\n", CMD);
    lgAdrClient = sizeof(adrClient);
    canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
    if (canal < 0)
      erreur_IO("accept");

    printf("%s: adr %s, port %hu\n", CMD,
        stringIP(ntohl(adrClient.sin_addr.s_addr)), ntohs(adrClient.sin_port));

    ret = sem_wait(&semWorkersLibres);
    if (ret == -1)
      erreur_IO("wait semaphore workers libres");
    numWorkerLibre = chercherWorkerLibre();

    dataSpec[numWorkerLibre].canal = canal;
    ret = sem_post(&dataSpec[numWorkerLibre].sem);
    if (ret == -1)
      erreur_IO("post semaphore worker");
  }

  if (close(ecoute) == -1)
    erreur_IO("fermeture ecoute");

  if (close(pseudos) == -1)
    erreur_IO("fermeture pseudos");

  if (close(passwords) == -1)
    erreur_IO("fermeture passwords");

   if (close(timeline) == -1)
    erreur_IO("fermeture timeline");

  exit(EXIT_SUCCESS);
}







void creerCohorteWorkers(void) {
  int i, ret;

  for (i = 0; i < NB_WORKERS; i++) {
    dataSpec[i].canal = -1;
    dataSpec[i].tid = i;

    ret = sem_init(&dataSpec[i].sem, 0, 0);
    if (ret == -1)
      erreur_IO("init semaphore worker");

    ret = pthread_create(&dataSpec[i].id, NULL, threadWorker, &dataSpec[i]);
    if (ret != 0)
      erreur_IO("creation worker");
  }
}

// retourne le no. du worker ou -1 si pas de worker libre
int chercherWorkerLibre(void) {
  int i, canal;

  for (i = 0; i < NB_WORKERS; i++) {
    lockMutexCanal(i);
    canal = dataSpec[i].canal;
    unlockMutexCanal(i);
    if (canal < 0)
      return i;
  }

  return -1;
}

void *threadWorker(void *arg) {
  DataSpec *dataSpec = (DataSpec *)arg; 
  int ret;

  while (VRAI) {
    ret = sem_wait(&dataSpec->sem);
    if (ret == -1)
      erreur_IO("wait semaphore worker");

    printf("worker %d: reveil\n", dataSpec->tid);

    sessionClient(dataSpec->canal);

    lockMutexCanal(dataSpec->tid);
    dataSpec->canal = -1;
    unlockMutexCanal(dataSpec->tid);

    printf("worker %d: sommeil\n", dataSpec->tid);

    ret = sem_post(&semWorkersLibres);
    if (ret == -1)
      erreur_IO("post semaphore workers libres");
  }

  pthread_exit(NULL);
}


void sessionClient(int canal) {
  fermeture=0;
  connexion_ou_creer_compte(canal);
  verification_pseudo_password(canal);
  while(!fermeture){
  	menu(canal);
  }
  fermeture=0;

  if (close(canal) == -1)
    erreur_IO("fermeture canal");
}

/*Vérifie le pseudo envoyé par le client*/

int verification_pseudo(char ligne[LIGNE_MAX],int compteur[]){
	int resu=0;
	char ligne2[LIGNE_MAX];
	off_t depl=0;
	off_t pos=lseek(pseudos,depl,SEEK_SET); // on se met au début du fichier log
	if (pos==-1)
		erreur_IO("erreur lseek");
	compteur[0]=0;
	while((lireLigne(pseudos,ligne2)!=0)&&(resu!=1)){
		compteur[0]++;
		if(strcmp(ligne2,ligne)==0)
  			resu=1;
  	}
  	return resu;
}


/*Vérifie le password envoyé par le client*/

int verification_password(char ligne[LIGNE_MAX],int compteur[]){
	int resu=0;
	char ligne2[LIGNE_MAX];
	off_t depl=0;
	off_t pos=lseek(passwords,depl,SEEK_SET); // on se met au début du fichier log
	if (pos==-1)
		erreur_IO("erreur lseek");
	compteur[1]=0;
	while((lireLigne(passwords,ligne2)!=0)&&(resu!=1)){
		compteur[1]++;
		if((strcmp(ligne2,ligne)==0)&&(compteur[1]==compteur[0]))
  			resu=1;
	}
	return resu;
}


/*Vérifie le pseudo et password envoyé par le client*/

void verification_pseudo_password(int canal){
	char ligne[LIGNE_MAX];
  	int  lgEcr;
	int checkpoint1=0; // checkpoint pour le pseudo
  	int checkpoint2=0; // checkpoint pour le password
  	int compteur[2];
 	 while(!((checkpoint1)&&(checkpoint2))){ // tant que le pseudo et le password ne sont pas corrects
		int pseudo=0;
    		pseudo=lireLigne(canal,ligne);
		if (pseudo == -1)
      			erreur_IO("lecture canal");	
  		checkpoint1=verification_pseudo(ligne,compteur); // on vérifie que le pseudo existe
  		if(checkpoint1==1){
			lgEcr = ecrireLigne(canal, "pseudo_correct\n");
			printf("pseudo_correct(canal=%d) \n",canal);
        		if (lgEcr < 0)
        	 	 	erreur_IO("ecriture journal");
			int password=0;
  			password=lireLigne(canal,ligne);
			if (password == -1)
      				erreur_IO("lecture canal");
			checkpoint2=verification_password(ligne,compteur); // on vérifie que le password existe
			if(checkpoint2==1){
				lgEcr = ecrireLigne(canal, "password_correct\n");
				printf("password correct (canal= %d) \n",canal);
        			if (lgEcr < 0)
        	 	 		erreur_IO("ecriture journal");
			}
			else{
				lgEcr = ecrireLigne(canal, "password_incorrect\n");
				printf("password incorrect (canal=%d)\n",canal);
        			if (lgEcr < 0)
        	 	 		erreur_IO("ecriture journal");
        	 	 	checkpoint1=0;
			}
			
 		 }
  		else{
  			lgEcr = ecrireLigne(canal, "pseudo_incorrect\n");
  			printf("pseudo incorrect (canal=%d)\n",canal);
        		if (lgEcr < 0)
        	 		erreur_IO("ecriture journal");
  		}	
  	}
}

/*Vérifie l'existence du pseudo*/

int existence_pseudo(char pseudo[]){
	char ligne[LIGNE_MAX];	
	off_t depl=0;
	off_t pos=lseek(pseudos,depl,SEEK_SET);
	if (pos==-1)
		erreur_IO("erreur lseek");
	int resu=0;
	while(lireLigne(pseudos,ligne)>0){
		if (strcmp(pseudo,ligne)==0)
			resu=1;
	}
	return resu;
}


/*Crée le compte demandé par le client*/

void creer_compte(int canal){
	char ligne[LIGNE_MAX];
	int message,lgEcr;
	message=lireLigne(canal,ligne);
	if (message == -1)
      		erreur_IO("lecture canal");
	while(existence_pseudo(ligne)==1){
		lgEcr = ecrireLigne(canal,"EXISTANT\n");
		if (lgEcr < 0)
      			erreur_IO("ecriture canal");
		message=lireLigne(canal,ligne);
		if (message == -1)
      			erreur_IO("lecture canal");
	}
	lgEcr = ecrireLigne(canal,"INEXISTANT\n");
	if (lgEcr < 0)
      		erreur_IO("ecriture canal");

        off_t depl1=0;
	off_t pos1=lseek(pseudos,depl1,SEEK_END);
	if (pos1==-1)
		erreur_IO("erreur lseek");
	lgEcr=ecrireLigne(pseudos,ligne);
	if (lgEcr < 0)
        	 erreur_IO("ecriture pseudos");
	message=lireLigne(canal,ligne);
	off_t depl2=0;
	off_t pos2=lseek(passwords,depl2,SEEK_END);
	if (pos2==-1)
		erreur_IO("erreur lseek");
	lgEcr=ecrireLigne(passwords,ligne);
	if (lgEcr < 0)
        	 erreur_IO("ecriture passwords");
	printf("un compte a été crée (canal=%d)\n",canal);
	
}

/*Prends en compte la décision du client*/

void connexion_ou_creer_compte(int canal){
	char ligne[LIGNE_MAX];
	int message;
	message=lireLigne(canal,ligne);
	if (message == -1)
        	erreur_IO("lecture canal");
	if(strcmp(ligne,"0")==0){
		creer_compte(canal);			
	}
	else{}
}


/*Envoie la timeline au client*/

void afficher_timeline(int canal){
	char ligne[LIGNE_MAX];
	int lgEcr;
	off_t depl3=0;
	off_t pos3=lseek(timeline,depl3,SEEK_SET);
	if (pos3==-1)
		erreur_IO("erreur lseek");
	while(lireLigne(timeline,ligne)>0){
		lgEcr=ecrireLigne(canal,ligne);
		if (lgEcr < 0)
        		erreur_IO("ecriture canal");
	}			
	lgEcr=ecrireLigne(canal,"LECTURE_TERMINEE\n");
		if (lgEcr < 0)
        		erreur_IO("ecriture canal");
}


/*Ecrit le message du client sur la timeline*/

void ecrire_timeline(int canal){
	char ligne[LIGNE_MAX];
	char pseudo[LIGNE_MAX];
	int lgLue,lgEcr;
	lgLue=lireLigne(canal,ligne);
	if (lgLue == -1)
      		erreur_IO("lecture canal");
      	lgLue=lireLigne(canal,pseudo);
      	if (lgLue == -1)
      		erreur_IO("lecture canal");
	pthread_mutex_lock(&monmutex);
      	off_t depl=0;
	off_t pos=lseek(timeline,depl,SEEK_END);
	if (pos==-1)
		erreur_IO("erreur lseek");
	lgEcr = ecrireLigne(timeline,"*********** POST ********************\n");
	if (lgEcr < -1) // il suffit de mettre 1 pour verifier s'il y a un erreur ou pas
      		erreur_IO("ecriture timeline");
	ecrireLigne(timeline," post fait par : \n");
	ecrireLigne(timeline,pseudo);
	ecrireLigne(timeline,"\n");
        horaire();
	ecrireLigne(timeline,"\n");
	ecrireLigne(timeline, "contenu du post : \n");
	ecrireLigne(timeline,ligne);
	ecrireLigne(timeline,"\n");
	ecrireLigne(timeline,"*************************************\n");
	ecrireLigne(timeline,"\n");
        pthread_mutex_unlock(&monmutex);

}


/*Prends en compte la décision du client*/

void menu(int canal){
	int fin = 0;
  	char ligne[LIGNE_MAX];
  	int lgLue;
 	 while (!fin) {
  	  	lgLue = lireLigne(canal, ligne);
   	 	if (lgLue == -1)
   	   		erreur_IO("lecture canal");
   		if (strcmp(ligne,"1")==0){  
    	    		printf("afficher la timeline (canal=%d)\n",canal);
	    		afficher_timeline(canal);
		}
   	 	if (strcmp(ligne,"2") == 0){
			printf("ecriture sur la timeline (canal=%d)\n",canal);
    	 		ecrire_timeline(canal);
   	 	}
		if (strcmp(ligne,"3")==0){
			fermeture=1;
			printf("la fermeture à été lancé (canal=%d)\n",canal);
			fin=1;
		}
		
	}
}	


/*Ecrit le temps sur timeline*/

void horaire(void)
{

	// time_t is arithmetic time type
	time_t now;

	// Obtain current time
	time(&now);

	// Convert to local time format and print
	ecrireLigne(timeline,"Date:\n");
	ecrireLigne(timeline,ctime(&now));

}


void lockMutexCanal(int numWorker)
{
  int ret;

  ret = pthread_mutex_lock(&mutexCanal[numWorker]);
  if (ret != 0)
    erreur_IO("lock mutex canal");
}

void unlockMutexCanal(int numWorker)
{
  int ret;

  ret = pthread_mutex_unlock(&mutexCanal[numWorker]);
  if (ret != 0)
    erreur_IO("unlock mutex canal");
}


