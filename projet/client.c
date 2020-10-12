// client.c
#include "pse.h"

#define CMD   "client"


char pseudo1[LIGNE_MAX];
int fermeture;

void creer_compte(int canal);
void connexion_ou_creer_compte(int canal);
int verification_pseudo(char ligne[]);
int verification_password(char ligne[]);
void verification_pseudo_password(int canal);
void afficher_timeline(int canal);
void ecrire_timeline(int canal,char pseudo[LIGNE_MAX]);
void menu(int canal,char pseudo[]);





int main(int argc, char *argv[]) {
  int sock, ret;
  struct sockaddr_in *adrServ;

  signal(SIGPIPE, SIG_IGN);

  if (argc != 3)
    erreur("usage: %s machine port\n", argv[0]);

  printf("%s: creating a socket\n", CMD);
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  printf("%s: DNS resolving for %s, port %s\n", CMD, argv[1], argv[2]);
  adrServ = resolv(argv[1], argv[2]);
  if (adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);

  printf("%s: adr %s, port %hu\n", CMD,
	        stringIP(ntohl(adrServ->sin_addr.s_addr)),
	        ntohs(adrServ->sin_port));

  printf("%s: connecting the socket\n", CMD);
  ret = connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in));
  if (ret < 0)
    erreur_IO("connect");

  printf("*********************TWITTEUR***********************************\n\n");
  printf("Bienvenue sur le réseau social crée pour les ismins par Charles CHREIM et Yavuz AKIN\n\n");

  fermeture=0;
  connexion_ou_creer_compte(sock);
  verification_pseudo_password(sock);
  while (!fermeture) {
  menu(sock,pseudo1);
  }

  if (close(sock) == -1)
    erreur_IO("fermeture socket");

  exit(EXIT_SUCCESS);
}




/* Cette fonction permet de vérifier si le pseudo et le password rentré par l'utilisateur est correct*/

void verification_pseudo_password(int canal){
    printf("********CONNEXION*********\n\n");
    char ligne[LIGNE_MAX];
      int lgEcr;
    int checkpoint1=0; // checkpoint pour le pseudo
      int checkpoint2=0; // checkpoint pour le password
      while(!((checkpoint1)&&(checkpoint2))){ // tant que le pseudo et le password ne sont pas corrects
        printf("Ecrire votre pseudo : \n");
        fgets(ligne,LIGNE_MAX,stdin);
	while(strcmp(ligne,"\n")==0){
		printf("le pseudo est incorrect, retapez-le\n");
		fgets(ligne,LIGNE_MAX,stdin);
	}
	strcpy(pseudo1,ligne); // on stocke le pseudo dans une variable globale pour l'utiliser plus tard
        lgEcr = ecrireLigne(canal,ligne);
                if (lgEcr < 0)
                      erreur_IO("ecriture journal");
        int pseudo;
              pseudo=lireLigne(canal,ligne);
	      if (pseudo < 0)
                      erreur_IO("lecture canal");
          checkpoint1=verification_pseudo(ligne); // on vérifie que le pseudo existe
          if(checkpoint1==1){
            printf("le pseudo est correct\nEntrez le password:\n");
            fgets(ligne,LIGNE_MAX,stdin);
	    while(strcmp(ligne,"\n")==0){
		printf("le password est incorrect, retapez-le\n");
		fgets(ligne,LIGNE_MAX,stdin);
	    }
            lgEcr = ecrireLigne(canal,ligne);
                if (lgEcr < 0)
                      erreur_IO("ecriture journal");
            int password;
                  password=lireLigne(canal,ligne); 
		  if (password < 0)
                      erreur_IO("lecture canal");
            checkpoint2=verification_password(ligne);// le serveur nous indique si le password est correct
            if(checkpoint2==1){
                printf("le password est correct\n Bienvenue\n");
                
            }
            else{
                printf("le password est incorrect\n");    
                checkpoint1=0;
            }
          }
          else{
              printf("le pseudo est incorrect\n");
          }    
      }
}


/*Cette fonction permet de vérifier le pseudo avec le serveur*/

int verification_pseudo(char ligne[]){
 int resu;
 if(strcmp(ligne,"pseudo_correct")==0)
    resu=1;
 else
      resu=0;
 return resu;
}

/*Cette fonction permet de vérifier le password avec le serveur*/

int verification_password(char ligne[]){
 int resu;
 if(strcmp(ligne,"password_correct")==0)
    resu=1;
 else
      resu=0;
 return resu;
}


/*Cette fonction permet de créer un compte*/

void creer_compte(int canal){
	char ligne[LIGNE_MAX];
	printf("******CREATION DE COMPTE********\n\n");
	printf("Entrez le pseudo que vous voulez créer:\n");
	int verif_pseudo=0;
	while(verif_pseudo==0){
	fgets(ligne,LIGNE_MAX,stdin);
	ecrireLigne(canal,ligne);
	lireLigne(canal,ligne);
		if (strcmp(ligne,"EXISTANT")==0){
			verif_pseudo=0;
			printf("Ce pseudo existe déjà, entrez un nouveau: \n");
		}
		else if (strcmp(ligne,"INEXISTANT")==0)
			verif_pseudo=1;
		else
			printf("Il y a un problème, réessayez\n");
	}
	printf("Entrez votre mot de passe:\n");
	fgets(ligne,LIGNE_MAX,stdin);
	ecrireLigne(canal,ligne);
}

/*permet le choix entre créer un compte ou se connecter*/

void connexion_ou_creer_compte(int canal){
	char ligne[LIGNE_MAX];
	int fin=0;
	while(!fin){
		printf("ENTREE DE L'APPLICATION : \n\n Veuillez choisir une des options : \n");
		printf("0:créer un compte \n1:connexion\n");
		fgets(ligne,LIGNE_MAX,stdin);
		if(strcmp(ligne,"0\n")==0){
			ecrireLigne(canal,"0\n");
			creer_compte(canal);
			fin=1;
		}
		else if(strcmp(ligne,"1\n")==0){
			ecrireLigne(canal,"1\n");
			fin=1;
		}
		else{
			printf("la commande n'a pas été comprise \n\n");
		}
			
	}
}

/*Affiche la timeline renvoyé par le serveur*/
void afficher_timeline(int canal){
	char ligne[LIGNE_MAX];
	int lgLue;
	lgLue=lireLigne(canal,ligne);
	if (lgLue == -1)
      		erreur_IO("lecture canal");
	printf("\n\n ******** Bienvenue dans la Timeline *******\n\n");
	while(strcmp("LECTURE_TERMINEE",ligne)!=0){
		printf("%s\n",ligne);
		lgLue=lireLigne(canal,ligne);
		if (lgLue == -1)
      			erreur_IO("lecture canal");	
	}
}

/*envoie ce qu'il faut écrire dans la timeline*/

void ecrire_timeline(int canal,char pseudo[LIGNE_MAX]){
	printf("\n\n*******ECRITURE TIMELINE*********\n\n");
	int carac_max=LIGNE_MAX;
	char ligne2[LIGNE_MAX];
	int lgEcr;
	printf("Entrez un message d'au maximum %d caractères:\n",carac_max);
	fgets(ligne2, LIGNE_MAX, stdin);
	lgEcr=ecrireLigne(canal,ligne2);
	if (lgEcr < 0)
      		erreur_IO("ecriture canal");
	lgEcr=ecrireLigne(canal,pseudo);
		if (lgEcr < 0)
        		erreur_IO("ecriture canal");
}

/*imprime l'interface menu*/

void menu(int canal,char pseudo[]){
  int quitter = 0;
  char ligne[LIGNE_MAX];
 
  while (!quitter)
  {
    printf("\n");
    printf("****************MENU****************\n\n");
    printf("1: afficher la timeline\n");
    printf("2: ecrire dans la timeline\n");
    printf("3: deconnexion\n");
    printf("\n\n");
    printf("choix : ");
    fgets(ligne,LIGNE_MAX,stdin);
    if (strcmp(ligne,"1\n") == 0) {
    	ecrireLigne(canal,ligne);
	quitter = 1;
	afficher_timeline(canal);
    }    
    else if (strcmp(ligne,"2\n") == 0) {
    	ecrireLigne(canal,ligne);
	quitter = 1; 
	ecrire_timeline(canal,pseudo);   
    }
    else if (strcmp(ligne,"3\n") == 0) {
	ecrireLigne(canal,ligne);      	
	fermeture=1;
	quitter=1;
    }
    else {
	printf("la commande est incorrect veuilliez recommencer \n\n");
	}
   }
      
}









