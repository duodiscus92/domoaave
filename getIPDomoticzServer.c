#include "domoaave.h"

int getCurrentSSID(char *ssid, size_t len)
{
    FILE *fp;
    char line[256];

    fp = popen("nmcli -t -f ACTIVE,SSID dev wifi | grep '^yes:'", "r");
    if (!fp)
        return -1;

    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return -1;
    }

    pclose(fp);

    // ligne du type : yes:Livebox-XXXX
    char *p = strchr(line, ':');
    if (!p)
        return -1;

    p++; // après ':'
    strncpy(ssid, p, len - 1);
    ssid[len - 1] = '\0';

    // enlever le \n
    ssid[strcspn(ssid, "\n")] = 0;
    fprintf(stderr, "Détecté reseau wifi SSID = %s\n", ssid); 
    return 0;
}


bool is_ethernet_up(void)
{
    FILE *fp;
    char line[256];

    fp = popen("nmcli -t -f DEVICE,TYPE,STATE dev | grep ethernet", "r");
    if (!fp)
        return false;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, ":connected")) {
            pclose(fp);
            return true;
        }
    }

    pclose(fp);
    return false;
}

/* Récupération IP Domoticz en fonction du SSID */
const char *getIpDomoticz(cJSON *root)
{
   static char ip[32];
   char ssid[64];

   cJSON *network, *servers, *entry;
   const char *default_ip;

   if (getCurrentSSID(ssid, sizeof(ssid)) != 0) {
      fprintf(stderr, "Impossible de récupérer le SSID\n");
      return NULL;
   }

   fprintf(stderr, "SSID courant : %s\n", ssid);

   /* descente dans l'arborescence */
   network = cJSON_GetObjectItem(root, "network");
   if (!network) {
      fprintf(stderr, "Bloc network introuvable\n");
      return NULL;
   }

   default_ip = cJSON_GetObjectItem(network, "default_ip")->valuestring;
   servers = cJSON_GetObjectItem(network, "servers");

   if (!cJSON_IsArray(servers)) {
      fprintf(stderr, "servers n'est pas un tableau\n");
      strncpy(ip, default_ip, sizeof(ip));
      return ip;
   }

   /* parcours du tableau */
   cJSON_ArrayForEach(entry, servers) {
      const char *json_ssid = cJSON_GetObjectItem(entry, "ssid")->valuestring;
      const char *json_ip   = cJSON_GetObjectItem(entry, "ip")->valuestring;

      if (strcmp(ssid, json_ssid) == 0) {
         fprintf(stderr, "IP Domoticz = %s\n", json_ip);
         strncpy(ip, json_ip, sizeof(ip));
         return ip;
      }
   }

   /* fallback */
   fprintf(stderr, "SSID inconnu, utilisation IP par défaut\n");
   strncpy(ip, default_ip, sizeof(ip));
   return ip;
}

/*
// Récupération IP Domoticz en fonction du SSID 
const char *get_domoticz_host_depreciated(void)
{
    static char ssid[64];

    // Priorité 1 : variable d'environnement 
    const char *env = getenv("DOMOTICZ_HOST");
    if (env && strlen(env) > 0)
        return env;

    // WiFi actif ? 
    if (get_current_ssid(ssid, sizeof(ssid)) == 0) {

        if (strcmp(ssid, "SFR_62E8") == 0)
            return IP_DOMOTICZ_SFR62E8;

        if (strcmp(ssid, "AAVE - IoT") == 0)
            return  IP_DOMOTICZ_AAVEIOT ;

        if (strcmp(ssid, "Pixel 6a") == 0)
            return  IP_DOMOTICZ_PIXEL6A ;

        // WiFi inconnu 
        fprintf(stderr, "SSID inconnu [%s]\n", ssid);
    }

    // Ethernet ? 
    if (is_ethernet_up())
        return IP_DOMOTICZ_SFR62E8;   // ex : serveur local par défaut

    // Dernier recours 
    fprintf(stderr, "Impossible de déterminer le serveur Domoticz\n");
    return NULL;
}
*/

// test
/**
void main(void)
{
  cJSON *root;
  char *jsonText;
  char hostname[32];

  // lecture du fichier json de paramètres situé sur le serveur domoticz
  jsonText = readJson("/opt/domoaave/client.config.json");
  //  on parse la chaine JSON
  if(!(root = cJSON_Parse(jsonText))){
      fprintf(stderr, "Erreur parsing JSON\n");
      free(jsonText);
      exit(1);
  }

  //const char *ip = get_domoticz_host();
  const char *ip = getIpDomoticz(root);
  if (!ip) {
      fprintf(stderr, "Pas d'IP Domoticz\n");
      exit(1);
  }
  fprintf(stderr, "Detecté  IP Domoticz = %s\n", ip);
}
**/



