#include "domoaave.h"

int get_current_ssid(char *ssid, size_t len)
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



const char *get_domoticz_host(void)
{
    static char ssid[64];

    /* Priorité 1 : variable d'environnement */
    const char *env = getenv("DOMOTICZ_HOST");
    if (env && strlen(env) > 0)
        return env;

    /* WiFi actif ? */
    if (get_current_ssid(ssid, sizeof(ssid)) == 0) {

        if (strcmp(ssid, "SFR_62E8") == 0)
            return IP_DOMOTICZ_SFR62E8;

        if (strcmp(ssid, "AAVE - IoT") == 0)
            return  IP_DOMOTICZ_AAVEIOT ;

        /* WiFi inconnu */
        fprintf(stderr, "SSID inconnu [%s]\n", ssid);
    }

    /* Ethernet ? */
    if (is_ethernet_up())
        return IP_DOMOTICZ_SFR62E8;   // ex : serveur local par défaut

    /* Dernier recours */
    fprintf(stderr, "Impossible de déterminer le serveur Domoticz\n");
    return NULL;
}

// test
/*void main(void)
{
  const char *ip = get_domoticz_host();
  if (!ip) {
      fprintf(stderr, "Pas d'IP Domoticz\n");
      exit(1);
  }
  fprintf(stderr, "Detecté  IP Domoticz = %s\n", ip);
}*/



