#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

// pour WiringPi
#include <wiringPi.h>
// pour MQTT
#include <mosquitto.h>
// pour cjson
#include <cjson/cJSON.h>
#define JSON_CONFIG_PATH "client.config.json"

#undef  STANDALONE 

//pour le debug
#undef  DEBUG1
#undef  DEBUG2

// adresse IP et port MQTT du serveur Domoticz
#define IP_DOMOTICZ_AAVEIOT  "192.168.7.156"	// à Buno
#define IP_DOMOTICZ_SFR62E8  "192.168.1.44"	// à Montrouge Livebox
#define IP_DOMOTICZ_PIXEL6A  "10.83.34.60"	// sur partage réseau Pixel 6a
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

// nombre max de compteurs d'énergie par points de distribution
#define  MAX_CP	8

//fichiers contenant les couts totaux pour repartir correctement en cas de coupure secteur
#define TOTAL_COST_PATH "totalCost.data"

//fichier contenant les kWh  totaux pour repartir correctement en cas de coupure secteur (binaire et texte pour Telegram)
#define TOTAL_KWH_BIN_PATH 	"/opt/domoaave/totalKwh.data"
#define TOTAL_KWH_TXT_PATH 	"/opt/domoaave/totalKwh.text"

// lecture d'un fichier json de paramètres
extern char *readJson(const char *filename);
// lecture de l'adresse ip du serveur domoticz (ça ne sert pas beaucoup)
extern char *getJsonIpDomoticz(cJSON *root);
// lire le n° de port Mqtt  utilisé par Domoticz
extern int getJsonPortMQTT(cJSON *root);
// lire les idx Domoticz ratttachés au point de distribution dont le nom est hostnamme
extern cJSON *getJsonTidx(cJSON *root, int tidx[], int tidxhp[], int tidxhc[], int tidxhpeak[], int tidxcout[]);
// lire l'idx du device Domoticz cpuTemp ratttachés au point de distribution dont le nom est hostnamme
extern cJSON *getJsonIdxCpuTemp (cJSON *root, int *idxCpuTemp); 
// obtenir l'ip domoticz en fonction du  SSID Buno ou Montrouge
//extern const char *get_domoticz_host(void);
extern const char *getIpDomoticz(cJSON *root);
