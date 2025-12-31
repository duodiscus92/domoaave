#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <mosquitto.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
//pour le debug
#define  DEBUG1
#define  DEBUG2

// adresse IP et port MQTT du serveur Domoticz
#define IP_DOMOTICZ  "192.168.1.44"
#define PORT_MQTT_DOMOTICZ 1884

// tranches horaires
//heeures creuses
#define DEB_HC 22
#define FIN_HC 6
//heures de pointe 1ere tranche horaire
#define DEB_HPEAK1 9
#define FIN_HPEAK1 11
//heurss de pointe 2ieme tranche horaire
#define DEB_HPEAK2 18
#define FIN_HPEAK2 20
//tout le reste est en heures creuses

// tarifs 2025 HT EDF du PACK PERFORMANCE en cts d'euros
#define PRIX_HP_ETE 6.876
#define PRIX_HC_ETE 4.703
#define PRIX_HP_HIVER 12.581
#define PRIX_HC_HIVER 9.025
#define PRIX_HPEAK 16.423
#define TVA	(1.2)
#define CHARGES (0.076) // Montant par kWh Accise, CTA, abonnement, options etc 

//Nb de secondes entre deux signalement que toujours vivant
#define ALIVE_PERIOD (3600)

// tableau des idx des instruments Domoticz
static  int tidx[8]={13, 18, 0, 0, 0, 0, 0, 0};
static  int tidxHP[8]={15, 20, 0, 0, 0, 0, 0, 0};
static  int tidxHC[8]={14, 19, 0, 0, 0, 0, 0, 0};
static  int tidxHPeak[8]={16, 21, 0, 0, 0, 0, 0, 0};
static  int tidxCTot[8]={17, 22, 0, 0, 0, 0, 0, 0};

// dans ce tableau on cumule les couts
static float tcouTotal[8] = {0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; 

// buffer pour les messages json
static  char payload[500];

// globalCounter and globa flag:
//	Global variable to count interrupts
//	Should be declared volatile to make sure the compiler doesn't cache it.
//	Global flag pour le watchdog
static volatile int globalCounter [8];

static float priceElectricity[][2] = {
	// ETE
	PRIX_HP_ETE, PRIX_HC_ETE, // HP, HC
	// HIVER
	PRIX_HP_HIVER, PRIX_HC_HIVER // HP, HC
};


/*
 * myInterrupt:
 *********************************************************************************
 */

void myInterrupt0 (void) { ++globalCounter [0] ; }
void myInterrupt1 (void) { ++globalCounter [1] ; }
void myInterrupt2 (void) { ++globalCounter [2] ; }
void myInterrupt3 (void) { ++globalCounter [3] ; }
void myInterrupt4 (void) { ++globalCounter [4] ; }
void myInterrupt5 (void) { ++globalCounter [5] ; }
void myInterrupt6 (void) { ++globalCounter [6] ; }
void myInterrupt7 (void) { ++globalCounter [7] ; }


/*
 *********************************************************************************
 * fonction pour  savoir si on est en heures d'été ou hiver (selon EDF)
 *********************************************************************************
 */
enum saison {ete, hiver};

// renvoie 1 si ETE, 0 si HIVER
int isSummertime(void)
{
    time_t now = time(NULL);          // Récupère l'heure actuelle
    struct tm *local = localtime(&now);

    int mois = local->tm_mon + 1;     // tm_mon : 0 = janvier, 11 = décembre

    // du 1er avril au 31 octobre : ETE
    if(mois >= 4 && mois <= 10) {
#ifdef DEBUG2
      fprintf(stderr, "Heures d'été\n");
#endif
      return 1;
    }
    // du 1er novembre au 31 mars: HIVER
    else {
#ifdef DEBUG2
      fprintf(stderr, "Heures d'hiver\n");
#endif
      return 0;
    } 
}


/*
 *********************************************************************************
 * fonction qui indique si on est en HP ou HC ou heure de pointe (selon EDF)
 *********************************************************************************
 */
enum tranche {hp, hc, hpeak};
// renvoie  la tranche horaire en cours
float hourSlice(void)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if ((local->tm_hour >= DEB_HPEAK1 && local->tm_hour <= FIN_HPEAK1) || \
        (local->tm_hour >= DEB_HPEAK2 && local->tm_hour <= FIN_HPEAK2)) {
        // Tranche POINTE
#ifdef DEBUG2
        fprintf(stderr, "Heures de pointe\n");
#endif
	return isSummertime() ?  hp : hpeak;
    }else if (local->tm_hour >= DEB_HC || local->tm_hour < FIN_HC) {
        // Tranche qui passe par minuit (ex: 22:00 → 06:00)
#ifdef DEBUG2
	fprintf(stderr, "Heures creuses\n");
#endif
        return hc;
    }
    else {
        // Tranche pleine
#ifdef DEBUG2
	fprintf(stderr, "Heures pleine\n");
#endif
        return hp;
    }
}

/*
 *********************************************************************************
 * fonction renvoie le prix HT de la tranche horaire en cours
 *********************************************************************************
 */

float priceNow(void)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    /*int maintenant = local->tm_hour * 60 + local->tm_min;
    int debut      = h_deb * 60 + m_deb;
    int fin        = h_fin * 60 + m_fin;*/


    if ((local->tm_hour >= DEB_HPEAK1 && local->tm_hour <= FIN_HPEAK1) || \
        (local->tm_hour >= DEB_HPEAK2 && local->tm_hour <= FIN_HPEAK2)) {
        // Tranche POINTE
        //fprintf(stderr, "Heures de pointe\n");
	return isSummertime() ?  PRIX_HP_ETE : PRIX_HPEAK;
    } if (local->tm_hour >= DEB_HC || local->tm_hour < FIN_HC) {
        // Tranche qui passe par minuit (ex: 22:00 → 06:00)
	//fprintf(stderr, "Heures creuses\n");
        return isSummertime() ? PRIX_HC_ETE : PRIX_HC_HIVER;;
    }
    else {
        // Tranche pleine
	//fprintf(stderr, "Heures pleine\n");
        return isSummertime() ? PRIX_HP_ETE : PRIX_HP_HIVER;
    }
}


/*
 *********************************************************************************
 * fonction qui signale à Domolticz que l'appli est toujours en vie
 * elle envoie des incréments nuls aux compteurs incrémentaux
 *********************************************************************************
 */
static struct mosquitto *mosq;  // mosq est initialisé dans main()
void aLive(int pin)
{
   int i;

   fprintf(stderr, "\nHello World,toujours vivant !! \n");
   for (i=0; i< 8 ; i++){
      if(tidx[pin] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidx[pin], 0);
        mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      }
      if(tidxHP[pin] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHP[pin], 0);
        mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      }
      if(tidxHC[pin] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHC[pin], 0);
        mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      }
      if(tidxHPeak[pin] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHPeak[pin], 0);
        mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      }
      if(tidxCTot[pin] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%8.3f\" }\n",  tidxCTot[pin], tcouTotal[pin]*TVA);
        mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      }
   }
}


/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int main (void)
{
  int gotOne, pin, slice, fn;
  int myCounter [8] ;
  float price;
  time_t now;

  // initialisation de la librairie mosquitto
  if(mosquitto_lib_init() != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "Impossible d'initialiser la librairie mosquitto\n");
    exit(1);
  }
  if((mosq = mosquitto_new("guillaumet", true, NULL)) == NULL) {
    fprintf(stderr, "Impossible de creer une nouveau client mosquitto\n");
    mosquitto_destroy(mosq);
    exit(1);
  }
  if(mosquitto_connect(mosq, IP_DOMOTICZ, PORT_MQTT_DOMOTICZ, 60) != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "Impossible de se connecter au serveur Domoticz\n");
    mosquitto_destroy(mosq);
    exit(1);
  }

  // initialiser la référence de l'heure locale à Partis
  setenv("TZ", "Europe/Paris", 1);
  tzset();

  for (pin = 0 ; pin < 8 ; ++pin){ 
    globalCounter [pin] = myCounter [pin] = 0 ;
    aLive(pin);
  }

  // intialisation de la lib wiringPi
  wiringPiSetup () ;
  // installtion des gestionnaires d'interruption 
  wiringPiISR (14, INT_EDGE_FALLING, &myInterrupt0) ;
  wiringPiISR (21, INT_EDGE_FALLING, &myInterrupt1) ;
  wiringPiISR (2, INT_EDGE_FALLING, &myInterrupt2) ;
  wiringPiISR (3, INT_EDGE_FALLING, &myInterrupt3) ;
  wiringPiISR (4, INT_EDGE_FALLING, &myInterrupt4) ;
  wiringPiISR (5, INT_EDGE_FALLING, &myInterrupt5) ;
  wiringPiISR (6, INT_EDGE_FALLING, &myInterrupt6) ;
  wiringPiISR (7, INT_EDGE_FALLING, &myInterrupt7) ;

  for (;;)
  {
    gotOne = 0 ;
    //printf ("Waiting ... ") ; fflush (stdout) ;

    for (;;)
    {
      // signale a Domoticz toutes les heures que toujours en vie
      now = time(NULL);
      if ((now % ALIVE_PERIOD) == 0){
        for (pin = 0 ; pin < 8 ; pin++)
	  aLive(pin);
        delay(1000);
      }

      for (pin = 0 ; pin < 8 ; ++pin)
      {
	if (globalCounter [pin] != myCounter [pin])
	{
	  // publication energie gobale
	  sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidx[pin], 1);
#ifdef DEBUG2
	  fprintf(stderr, payload);
#else
	  fprintf(stderr,"."); fflush(stderr);
#endif
	  mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
	  //delay(1000); // pas certain que ce soit necessaire
	  if((slice = hourSlice()) == hp)
             // publication energie heure pleine
	     sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHP[pin], 1);
	  else if (slice == hc)
	     // publication energie heure creuse
	     sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHC[pin], 1);
	  else
	     // publication energie heure pointe
	     sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHPeak[pin], 1);
#ifdef DEBUG2
	  fprintf(stderr, payload);
#endif
	  mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
	  // publication du coût total
/**/
          // persister cout total
          if((fn = open("totalCost.data", O_RDWR | O_CREAT | O_EXCL, 0644)) == -1){
	     if(errno==EEXIST){
		if((fn = open("totalCost.data", O_RDWR)) == -1){
                  fprintf(stderr, "Impossible d'ouvrir ou creer fichier totalCost.data\n");
                  exit(1);
		}
		else {
#ifdef DEBUG1
		  fprintf(stderr, "Le fichier  totalCost.data existe déja : le charge en mémoire\n");
#endif
	          // cout du kWh HT (compte tenu du la saison et de la tranche horaire
                  read(fn, tcouTotal, sizeof(tcouTotal));
#ifdef DEBUG1
		  for(int i=0; i<8;i++)
		    fprintf(stderr, "V(%d) = %8.4f\n", i+1, tcouTotal[i]);
#endif
	        }
             }
	  }
          else{
	       // le fichier n'existait pas il a été crée : il est  initialisé avec valeurs par defaut 
#ifdef DEBUG1
	       fprintf(stderr, "Initialisation du fichier totalCost.data\n");
#endif
	       write (fn, tcouTotal, sizeof(tcouTotal));
	  }
	  // on se replace au début du fichier
	  lseek(fn, 0, SEEK_SET);
	  price = priceNow();
	  // cout HT du Wh incluant les charges diverses (Accise, CTA etc)
	  tcouTotal[pin] +=  ((price + CHARGES)/1000);
#ifdef DEBUG2
	  fprintf(stderr, "Prix du kwh HT en cts € %8.3f\n", price);
#endif
	  sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%8.3f\" }\n",  tidxCTot[pin], tcouTotal[pin]*TVA);
#ifdef DEBUG2
	  fprintf(stderr, payload);
#endif
	  mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
	  // on sauvegarde le coût total actualisé
#ifdef DEBUG1
	  fprintf(stderr,"Sauvegarde totalCost.data avec valeurs actualisées\n");
#endif
          write(fn, tcouTotal, sizeof(tcouTotal));
	  close(fn);
/**/
	  myCounter [pin] = globalCounter [pin] ;
	  ++gotOne ;
	}
      }
      if (gotOne != 0)
	break ;
    }
  }
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  return 0 ;
}
