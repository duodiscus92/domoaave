#include "domoaave.h"

#define WIRINGPI 	0
#define PIGPIO 		1-WIRINGPI

#if PIGPIO == 1
#include <pigpio.h>
#endif

extern void mqttPublish(const int idx, const char *topic, const char *msg);

//iadresse IP du serveur Domotciz
const char *IpDomoticz;

// tableau des idx des instruments Domoticz
static  int tidx[MAX_CP]={1, 2, 0, 0, 0, 0, 0, 0};
static  int tidxHP[MAX_CP]={3, 7, 0, 0, 0, 0, 0, 0};
static  int tidxHC[MAX_CP]={4, 6, 0, 0, 0, 0, 0, 0};
static  int tidxHPeak[MAX_CP]={5, 8, 0, 0, 0, 0, 0, 0};
static  int tidxCTot[MAX_CP]={9, 10, 0, 0, 0, 0, 0};
static  int idxCpuTemp = 46;

// dans ce tableau on cumule les couts
static float tcouTotal[MAX_CP] = {0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; 

// dans ce tableau on cumule les kWh (pour  Telegram)
static float kwhTotal[MAX_CP] = {-1, -1, -1, -1, -1, -1, -1, -1};

// buffer pour les messages json
static  char payload[500];


// tarifs électricité (à récupérer dans Json ultérieuremznt)
// été/hiver et heures pleines ou creuses
static float priceElectricity[][2] = {
	// ETE
	PRIX_HP_ETE, PRIX_HC_ETE, // HP, HC
	// HIVER
	PRIX_HP_HIVER, PRIX_HC_HIVER // HP, HC
};

//	Global variable to count interrupts
//	Should be declared volatile to make sure the compiler doesn't cache it.
//	Global flag pour le watchdog

typedef struct {
    volatile uint32_t pulses;          // compteur brut ISR
    volatile uint32_t pulses_last;     // dernier état lu
} compteur_t;

static compteur_t compteurs[MAX_CP];
static int nb_compteurs = 0;

#if PIGPIO == 1
unsigned char tgpio[] = {11, 9, 10, 22, 17, 4, 99, 5};
unsigned char tindex[] = {0, 1, 2, 3, 4, 5, 6, 7};
#endif

/*
 *********************************************************************************
 * interrupt handler : il incrémente son compteur
 *********************************************************************************
 */
#if WIRINGPI == 1
static void isr_0(void) { compteurs[0].pulses++; }
static void isr_1(void) { compteurs[1].pulses++; }
static void isr_2(void) { compteurs[2].pulses++; }
static void isr_3(void) { compteurs[3].pulses++; }
static void isr_4(void) { compteurs[4].pulses++; }
static void isr_5(void) { compteurs[5].pulses++; }
static void isr_6(void) { compteurs[6].pulses++; }
static void isr_7(void) { compteurs[7].pulses++; }
#endif

#if PIGPIO == 1
/* Callback pigpio */
void alert_callback(int gpio,
                    int level,
                    unsigned int tick,
                    void *userdata)
{
    unsigned char *i = (unsigned char*) userdata;

    if (level == FALLING_EDGE) {
        if (*i < 0 || *i >= 8){
           fprintf(stderr, "Erreur : IRQ d'origine inconnue\n");
           exit(1);
        }
        compteurs[*i].pulses++;
        //printf("ac : IRQ on GPIO %d -> pulses[%u] = %d\n", gpio, *i, pulses[*i]>
        //fflush(stdout);
    }
}
#endif
/*
 *********************************************************************************
 * fonction d'affichage des paramètres lus dans le fichier json de config
 * utile essentiellement pour le debug
 *********************************************************************************
 */
void printJsonConfig(void)
{
   // les idx
   fprintf(stderr, "printJsonConfig : Mes Idx total: ");
   for (int i = 0; i< MAX_CP; i++)
      fprintf(stderr, "%2d, ", tidx[i]);
   fprintf(stderr, "\n");

   fprintf(stderr, "printJsonConfig : Mes Idx heures pleines: ");
   for (int i = 0; i< MAX_CP; i++)
      fprintf(stderr, "%2d, ", tidxHP[i]);
   fprintf(stderr, "\n");

   fprintf(stderr, "printJsonConfig : Mes Idx heures creuses: ");
   for (int i = 0; i< MAX_CP; i++)
      fprintf(stderr, "%2d, ", tidxHC[i]);
   fprintf(stderr, "\n");

   fprintf(stderr, "printJsonConfig : Mes Idx heures de pointes: ");
   for (int i = 0; i< MAX_CP; i++)
      fprintf(stderr, "%2d, ", tidxHPeak[i]);
   fprintf(stderr, "\n");

   fprintf(stderr, "printJsonConfig : Mes Idx cout: ");
   for (int i = 0; i< MAX_CP; i++)
      fprintf(stderr, "%2d, ", tidxCTot[i]);
   fprintf(stderr, "\n");

   fprintf(stderr, "printJsonConfig : Mon Idx cpuTemp: ");
   fprintf(stderr, "%2d\n", idxCpuTemp);
}



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
      //fprintf(stderr, "isSummertime : Heures d'été\n");
      return 1;
    }
    // du 1er novembre au 31 mars: HIVER
    else {
      //fprintf(stderr, "isSummertime : Heures d'hiver\n");
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
        //fprintf(stderr, "hourSlice : Heures de pointe\n");
	return isSummertime() ?  hp : hpeak;
    }else if (local->tm_hour >= DEB_HC || local->tm_hour < FIN_HC) {
        // Tranche qui passe par minuit (ex: 22:00 → 06:00)
	//fprintf(stderr, "hourSlice : Heures creuses\n");
        return hc;
    }
    else {
        // Tranche pleine
	//fprintf(stderr, "hoursSlice : Heures pleine\n");
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

    // test si heure de pointe 
    if ((local->tm_hour >= DEB_HPEAK1 && local->tm_hour <= FIN_HPEAK1) || \
        (local->tm_hour >= DEB_HPEAK2 && local->tm_hour <= FIN_HPEAK2)) {
        // la tranche heures de pôinte n'existe qu'en hiver, en été c'est heures pleines
        //fprintf(stderr, "priceNow : Heures de pointe\n");
	return isSummertime() ?  PRIX_HP_ETE : PRIX_HPEAK;
    }
    // sinon stest si heure creuses 
    else if (local->tm_hour >= DEB_HC || local->tm_hour < FIN_HC) {
        // Tranche qui passe par minuit (ex: 22:00 → 06:00)
	//fprintf(stderr, "priceNow : Heures creuses\n");
        return isSummertime() ? PRIX_HC_ETE : PRIX_HC_HIVER;;
    }
    // sinon c'est forcément heure pleines
    else {
	//fprintf(stderr, "priceNow : Heures pleines\n");
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
void aLive(int compteur)
{
   int i;

   //for (i=0; i< MAX_CP ; i++){
      if(tidx[compteur] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidx[compteur], 0);
        fprintf(stderr, "aLive : %s", payload);
        //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
        mqttPublish(tidx[compteur], "domoticz/in",payload);
      }
      if(tidxHP[compteur] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHP[compteur], 0);
        fprintf(stderr, "aLive : %s", payload);
        //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
        mqttPublish( tidxHP[compteur], "domoticz/in",payload);
      }
      if(tidxHC[compteur] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHC[compteur], 0);
        fprintf(stderr, "aLive : %s", payload);
        //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
        mqttPublish(tidxHC[compteur], "domoticz/in",payload);
      }
      if(tidxHPeak[compteur] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHPeak[compteur], 0);
        fprintf(stderr, "aLive : %s", payload);
        //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
        mqttPublish(tidxHPeak[compteur],"domoticz/in",payload);
      }
      if(tidxCTot[compteur] != 0){
        sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%8.3f\" }\n",  tidxCTot[compteur], tcouTotal[compteur]*TVA);
        fprintf(stderr, "aLive : %s", payload);
        //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
        mqttPublish(tidxCTot[compteur], "domoticz/in",payload);
      }
   //}
}


/*
 *********************************************************************************
 * fonction qui indique s'il est minuit et 10 minutes
 *
 *********************************************************************************
 */
int isMidnightAndTen(void)
{
    static int last_run_day = -1;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    int today = (t->tm_year + 1900) * 10000 +
                (t->tm_mon + 1) * 100 +
                t->tm_mday;

    /* après 00:10 */
    if (t->tm_hour == 0 && t->tm_min >= 10) {
        if (last_run_day != today) {
            last_run_day = today;
	    fprintf(stderr, "isMidnightAndTen : Il est minuit et 10 minutes, Dr Schweitzer.\n");
            return 1;
        }
    }
    return 0;
}

/*
 *********************************************************************************
 * lecture fichier json et initialisation ou mise à jour des parametres
 *********************************************************************************
 */
void initOrUpdateParametres(void)
{
  char *jsonText;
  cJSON *root;

  fprintf(stderr, "initOrUpdateParametres : Initialisation ou mise à jour des paramètres Json\n");

  // lecture du fichier json de paramètres situé sur le serveur domoticz
  jsonText = readJson(JSON_CONFIG_PATH);

  //  on parse la chaine JSON 
  if(!(root = cJSON_Parse(jsonText))){
      fprintf(stderr, "initOrUpdateParametres : Erreur parsing JSON\n");
      free(jsonText);
      exit(1);
  }
  // récupétation des idx de ce point de distribution
  if(getJsonTidx(root, tidx, tidxHP, tidxHC, tidxHPeak, tidxCTot) == NULL) {
      fprintf(stderr, "initOrUpdateParametres : Impossible de récuérer les idx de ce point de distribution\n");
      exit(1);
  }
  // récupération de l'idx de la température cpu de ce point de distribution
  if(getJsonIdxCpuTemp(root, &idxCpuTemp) == NULL) {
      fprintf(stderr, "initOrUpdateParametres : Impossible de récuérer l' idx cpuTemp de ce point de distribution\n");
      exit(1);
  }

  // affichage (pour le debug)
  printJsonConfig();


  // récupération de l'ip Domoticz en fonction du SSID (Buno  ou Montrouge)
  IpDomoticz = getIpDomoticz(root);

  if (!IpDomoticz) {
      fprintf(stderr, "initOrUpdateParametres : Pas d'IP Domoticz\n");
      exit(1);
  }
  //fprintf(stderr, "Detecté  IP Domoticz = %s\n", IpDomoticz);

  cJSON_Delete(root);
  free(jsonText);
}

/*
 *********************************************************************************
 * initialisation des compteurs et installation des interrupt handler
 *********************************************************************************
 */
static void initCompteurs(void)
{
#if WIRINGPI == 1
    if (wiringPiSetupGpio() < 0) {
        fprintf(stderr, "initCompteurs : wiringPiSetupGpio failed");
        exit(1);
    }
#endif
#if PIGPIO  == 1
    int pi, /*bcm,*/ i;
    unsigned char bcm;

    /* Connexion au daemon pigpiod */
    pi = gpioInitialise();
    if (pi < 0) {
        fprintf(stderr, "initCompteurs : pigpio_start failed\n");
        exit(1);
    }
    fprintf(stderr, "initcompteurs : pigpio connected (pi=%d)\n", pi);
#endif
    for (int i = 0; i < MAX_CP; i++) {
        compteurs[i].pulses = 0;
        compteurs[i].pulses_last = 0;
#if PIGPIO == 1
        if ((bcm = tgpio[i]) == 99)
           continue;
        gpioSetMode(bcm, PI_INPUT);
        gpioSetPullUpDown(bcm, PI_PUD_OFF);
        gpioGlitchFilter(bcm, 5000);   // 5000 µs = 5 ms
        gpioSetAlertFuncEx(bcm, alert_callback, &tindex[i]);
#endif
    }
#if WIRINGPI == 1
    //wiringPiISR (14, INT_EDGE_FALLING, &isr_0) ; obsolete
    //wiringPiISR (21, INT_EDGE_FALLING, &isr_1) ; obsolete
    wiringPiISR (11, INT_EDGE_FALLING, &isr_0) ;
    wiringPiISR (5,  INT_EDGE_FALLING, &isr_1) ;
    /* pas utilisés pour le moment
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_2) ;
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_3) ;
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_4) ;
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_5) ;
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_6) ;
    wiringPiISR (0,  INT_EDGE_FALLING, &isr_7) ;
    */
#endif
}

/*
 *********************************************************************************
 * publier par mQTT la temperature cpu
 *********************************************************************************
*/
static char *tcpuJson = "{\"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%4.1f\"}\n";
static void getPublishCpuTemp(void)
{
   FILE *fp;
   int temp0, temp1, temp2, tempD;
   float temp;


   if((fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r")) == NULL)
      fprintf(stderr, "getPublishCpuTemp - Impossible d'ouvrir le fichier : /sys/class/thermal/thermal_zone0/temp - erreur : %d\n", errno);
   else {
      fscanf(fp, "%d", &temp0);
      printf("TCPU = %d\n", temp0);
      temp1 = temp0/1000;
      temp2 = temp0/100;
      tempD = temp2 % temp1;
      temp = temp1 + tempD/10.0;
      sprintf(payload, tcpuJson, idxCpuTemp, temp);
      printf(payload);
      mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
      fclose(fp);
   }
}

/*
 *********************************************************************************
 * publier par MQTT les valeurs liées à un compteur donné :
 * kWh total , kwh HP, HC, HPeak
 *********************************************************************************
 */
static void getPublishEnergy(int compteur)
{
   int slice;

   // publication energie gobale
   sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidx[compteur], 1);
   // cumuler les kwh pour envoi via Telegram
   //if (tidx[compteur] != 0) kwhTotal[compteur]++;
   //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
   mqttPublish(tidx[compteur], "domoticz/in",payload);
   if((slice = hourSlice()) == hp){
      // publication energie heure pleine
      sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHP[compteur], 1);
      mqttPublish(tidxHP[compteur], "domoticz/in",payload);
   }
   else if (slice == hc){
      // publication energie heure creuse
      sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHC[compteur], 1);
      mqttPublish(tidxHC[compteur], "domoticz/in",payload);
   }
   else {
      // publication energie heure pointe
      sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%d\" }\n",  tidxHPeak[compteur], 1);
      mqttPublish(tidxHPeak[compteur], "domoticz/in",payload);
   }
   //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
   //mqttPublish("domoticz/in",payload);

}

/*
 *********************************************************************************
 * mettre à jour les kwh totaux et le persister
 * 
 *********************************************************************************
 */
static void getPersistTotalKwh(int compteur)
{
   FILE *fp;
   int fn;

   // persister kwh total dans un fichier binaire
   if((fn = open(TOTAL_KWH_BIN_PATH, O_RDWR | O_CREAT | O_EXCL, 0644)) == -1){
      if(errno==EEXIST){
         if((fn = open(TOTAL_KWH_BIN_PATH, O_RDWR)) == -1){
            //fprintf(stderr, "getPersistTotalKwh : Impossible d'ouvrir ou creer fichier totalKwh.data\n");
            exit(1);
         }
         else {
            //fprintf(stderr, "getPersistTotalKwh : Le fichier  totalKwh.data existe déja : on le charge en mémoire\n");
            // cout du kWh HT (compte tenu du la saison et de la tranche horaire
            read(fn, kwhTotal, sizeof(kwhTotal));
            //for(int i=0; i<MAX_CP;i++)
               //fprintf(stderr, "getPersistTotalKwh : Kwh(%d) = %8.4f\n", i+1, totalKwh.data[i]);
         }
      }
   }
   else{
      // le fichier n'existait pas il a été crée : il est  initialisé avec valeurs par defaut
      fprintf(stderr, "getPersistTotalKwh : Initialisation du fichier totalKwh.data\n");
      write (fn, kwhTotal, sizeof(kwhTotal));
   }

   if (tidx[compteur] !=0) kwhTotal[compteur]++;

   // on se replace au début du fichier
   lseek(fn, 0, SEEK_SET);
   write(fn, kwhTotal, sizeof(kwhTotal));
   close(fn);

   // enregistrer une version texte pour Telegram et aussi pour une lecture facile (en debug)
   if ((fp = fopen(TOTAL_KWH_TXT_PATH, "w")) == NULL) {
       fprintf(stderr, "getPersistTotalKwh : impossible ouvrir/creer fichier totalKwh.text\n");
       exit(1);
   }

   for(int i=0; i< MAX_CP; i++)
       fprintf(fp, "PC%d=%.3f\n", i+1,  kwhTotal[i]/1000.0);

   fclose(fp);
}

/*
 *********************************************************************************
 * mettre à jour le coût total, le publier et le persister
 * 
 *********************************************************************************
 */
static void getPublishPersistTotalCost(int compteur)
{
   int fn;
   float price;

   // persister cout total
   if((fn = open(TOTAL_COST_PATH, O_RDWR | O_CREAT | O_EXCL, 0644)) == -1){
      if(errno==EEXIST){
         if((fn = open(TOTAL_COST_PATH, O_RDWR)) == -1){
            //fprintf(stderr, "getPublishPersistTotalCost : Impossible d'ouvrir ou creer fichier totalCost.data\n");
            exit(1);
         }
         else {
            //fprintf(stderr, "getPublishPersistTotalCost : Le fichier  totalCost.data existe déja : on le charge en mémoire\n");
            // cout du kWh HT (compte tenu du la saison et de la tranche horaire
            read(fn, tcouTotal, sizeof(tcouTotal));
            //for(int i=0; i<MAX_CP;i++)
               //fprintf(stderr, "getPublishPersistTotalCost : V(%d) = %8.4f\n", i+1, tcouTotal[i]);
         }
      }
   }
   else{
      // le fichier n'existait pas il a été crée : il est  initialisé avec valeurs par defaut
      fprintf(stderr, "getPublishPersistTotalCost : Initialisation du fichier totalCost.data\n");
      write (fn, tcouTotal, sizeof(tcouTotal));
  }
  // on se replace au début du fichier
  lseek(fn, 0, SEEK_SET);
  price = priceNow();
  // cout HT du Wh incluant les charges diverses (Accise, CTA etc)
  tcouTotal[compteur] +=  ((price + CHARGES)/1000);
  //fprintf(stderr, "getPublishPersistTotalCost ; Prix du kwh HT en cts € %8.3f\n", price);
  sprintf(payload, "{ \"idx\" : %d, \"nvalue\" : 0, \"svalue\" : \"%8.3f\" }\n",  tidxCTot[compteur], tcouTotal[compteur]*TVA);
  //fprintf(stderr, " getPublishPersistTotalCost : %s", payload);
  //mosquitto_publish(mosq, NULL, "domoticz/in", strlen(payload), payload, 0, false);
  mqttPublish(tidxCTot[compteur], "domoticz/in",payload);
  // on sauvegarde le coût total actualisé
  //fprintf(stderr,"getPublishPersistTotalCost : Sauvegarde totalCost.data avec valeurs actualisées\n");
  write(fn, tcouTotal, sizeof(tcouTotal));
  close(fn);
}


/*
 *********************************************************************************
 * le noyau d'acquisition est ici
 *********************************************************************************
 */
static void processCompteurs(void)
{
    unsigned int now, delta;

    for (int i = 0; i < MAX_CP; i++) {
        now = compteurs[i].pulses;
        delta = now - compteurs[i].pulses_last;

        if (delta > 0) {
            compteurs[i].pulses_last = now;
            // 👉 ICI : ta logique métier
            getPublishEnergy(i);
            getPublishPersistTotalCost(i);
	    getPersistTotalKwh(i);
        }
    }
}

/*
 *********************************************************************************
 * initialisations mosquitto pour MQTT
 *********************************************************************************
 */
static void initMosquitto(void)
{
  char hostname[32];

 // initialisation de la librairie mosquitto
  if(mosquitto_lib_init() != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "initMosquitto : Impossible d'initialiser la librairie mosquitto\n");
    exit(1);
  }

  //besoin de mon hostname avant d'appeler mosquitto_new car chaque client MQTT doit avoir son propre identifiant ...
  // ... alors, pourquoi pas prendre le hostname ...
  gethostname(hostname, sizeof(hostname));
  fprintf(stderr, "initMosquitto : %s\n", hostname);

  if((mosq = mosquitto_new(hostname, true, NULL)) == NULL) {
    fprintf(stderr, "initMosquitto : Impossible de creer une nouveau client mosquitto pour %x\n", hostname);
    mosquitto_destroy(mosq);
    exit(1);
  }
  else  {
    fprintf(stderr, "initMosquitto : Pointeur mosq = %x \n", (long) mosq);
  }

  // reconnection automatique (recommandé par Chatgpt pour faire face à perte du Wifi)
  mosquitto_reconnect_delay_set(mosq,2,10,true);

  //connection au serveur domoticz
  if(mosquitto_connect(mosq, IpDomoticz, PORT_MQTT_DOMOTICZ, 60) != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "initMosquitto : Impossible de se connecter au serveur Domoticz\n");
    mosquitto_destroy(mosq);
    exit(1);
  }

  // boucle reseau interne (recommandé par Chatgpt pour faire face à perte du Wifi)
  mosquitto_loop_start(mosq);
}

/*
 *********************************************************************************
 * publication par  MQTT
 *********************************************************************************
 */
void mqttPublish(const int idx, const char *topic, const char *msg)
{
    int rc;

    // si idx == 0 c'est pas normal, on ne publie pas
    if(idx == 0) {
        fprintf(stderr,"mqttPublish : erreur idx = %d ==> message non publié\n",idx);
        return;
    }

    rc = mosquitto_publish(mosq, NULL, topic,strlen(msg), msg, 0, false);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr,"mqttPublish : erreur %d\n",rc);
        mosquitto_reconnect(mosq);
    }
} 

/*
 *********************************************************************************
 * interrupt handler du signal (sigint, sigterm)
 *********************************************************************************
 */
volatile sig_atomic_t running = 1;
void  handleSignal(int sig)
{
   running = 0;
}

/*
 *********************************************************************************
 *  miseà jour d'un fichier heartbeat pour le whatchdog detection de freeze
 *********************************************************************************
 */

void update_heartbeat() {

    FILE *f = fopen("/opt/domoaave/domoaave_client_heartbeat", "w");

    if (!f) {
	fprintf(stderr, "update_heartbeat - impossible ouvrir fichier heartbeat\n");
	return;
    }

    fprintf(f, "%ld\n", time(NULL));
    fclose(f);
}

/*
 *********************************************************************************
 * main
 *********************************************************************************
 */
static int last_hour = -1;
int main (void)
{
  int ndot = 0;
  char hostname[32];
  time_t now;
  struct tm *tm;
  struct sigaction sa;

  // installation handler signaux
  sa.sa_handler = handleSignal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGTERM, &sa, NULL);  // systemctl stop
  sigaction(SIGINT,  &sa, NULL);  // Ctrl-C
  sigaction(SIGQUIT, &sa, NULL);


// initialisation des paramètres Json
  // cette fonction devra évolurr car actuellement trop de paramètres sont en dur
  initOrUpdateParametres();

  // initialisation de la librairie mosquitto
  initMosquitto();

  // initialiser la référence de l'heure locale à Paris
  setenv("TZ", "Europe/Paris", 1);
  tzset();

  // installation des interrupt handler et initalisation des compteurs
  initCompteurs();

  fprintf(stderr, "\nmain : Hello World, toutes les initialisations sont OK, je suis bien vivant !! \n");
  for (int i = 0 ; i < MAX_CP ; i++){ 
    compteurs[i].pulses = compteurs[i].pulses_last = 0;
    aLive(i);
  }

  // boucle d'acqusition
  while (running) {
    // toutes les heures on envoie un message au serveur pour signaler qu'on fonctionne toujours
    // cela évite que les devices Domoticz passent en rouge si aucune impulsion n'est émise par les energie-metres
    // signale a Domoticz toutes les heures que toujours en vie
    // on anvoie aussi la temperature cpu
    now = time(NULL);
    tm = localtime(&now);
    if (tm->tm_hour != last_hour) {
       last_hour = tm->tm_hour;
       fprintf(stderr, "\nmain : Hello World, toujours vivant !!\n");
       for (int i = 0; i < MAX_CP; i++)
          aLive(i);
       // temperature cpu
       getPublishCpuTemp();
    }

    // tout se passe dans cette fonction
    processCompteurs();

    // si c'est minuit et 10 minutes on met à jour les paramètres 
    if (isMidnightAndTen())
      initOrUpdateParametres();

    // fréquence  boucle 5 Hz suffit largement
    fprintf(stderr, ".");
    if(++ndot % 100 == 0) {
       fprintf(stderr, "\n");
       fflush(stderr);
    }

    // on signale que l'appli n'est pas geléé
    update_heartbeat();
#if WIRINGPI == 1
    delay(2000);
#endif
#if PIGPIO == 1
    sleep(2);
#endif
  }

  // on arrive  sur sigint, sigterm  ou sigquit
  fprintf(stderr, "\nArrêt demandé, nettoyage en cours...\n");

#if WIRINGPI == 1
  /* GPIO / wiringPi */
  //wiringPiISRStop();   // ou équivalent si utilisé
#endif
#if PIGPIO == 1
    gpioTerminate();
    return 0;
#endif

  //cloturer Mosquitto
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  return 0 ;
}
