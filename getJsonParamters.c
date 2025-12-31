#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include "cJSON.h"

#define MAX_CP	8

/* Lecture d'un fichier JSON */
char *readJson(const char *filename)
{
    FILE *f;
    long len;
    char *data;

   if((f = fopen(filename, "rb")) == NULL){
      fprintf(stderr, "Erreur : impossible ouvrir fichier Json\n");
      exit(1);
   }

   fseek(f, 0, SEEK_END);
   len = ftell(f);
   rewind(f);

   //allocation d'un buffer pour la chaine Json */
   if(!(data = (char *)malloc(len + 1))){
      fprintf(stderr, "Impossible allouer buffer pour la chaine Json\n");
      fclose(f);
      return NULL;
    }

    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    //fprintf(stderr, "%s\n", data);
    return data;
}

/* IP du serveur domoticz */
char *getJsonIpDomoticz(cJSON *root)
{

   cJSON *network;
   char *ip;


   /* descendre dans l'arborescence */
   network = cJSON_GetObjectItem(root, "network");
   ip = cJSON_GetObjectItem(network, "ip_domoticz")->valuestring;

   return ip;
}


/* port MQTT Domoticz */
int getJsonPortMQTT(cJSON *root)
{

   cJSON *mqtt;
   int port;


   /* descendre dans l'arborescence */
   mqtt = cJSON_GetObjectItem(root, "mqtt");
   port = cJSON_GetObjectItem(mqtt, "port_mqtt_domoticz")->valueint;

   return port;
}

/* Idx des MES instruments */
cJSON *getJsonTidx(cJSON *root, int tidx[], int tidxhp[], int tidxhc[], int tidxhpeak[], int tidxcout[])
{

   cJSON *clients, *host, *idx;;
   int port;
   char hostname[32], cpx[8];

   gethostname(hostname, sizeof(hostname));
   fprintf(stderr, "%s\n", hostname);

   /* descendre dans l'arborescence */
   clients = cJSON_GetObjectItem(root, "clients");
   if((host = cJSON_GetObjectItem(clients, hostname)) == NULL)
      fprintf(stderr, "Imposssible de trouver %s dans l'arborescence JSON\n", hostname);
   else {
      idx = cJSON_GetObjectItem(host, "idx");
      for (int i=0; i<MAX_CP; i++){
         snprintf(cpx, sizeof(cpx), "PC%d", i+1);
         tidx[i] = cJSON_GetObjectItem(idx, cpx)->valueint; 
      }
      idx = cJSON_GetObjectItem(host, "idxhp");
      for (int i=0; i<MAX_CP; i++){
         snprintf(cpx, sizeof(cpx), "PC%d", i+1);
         tidxhp[i] = cJSON_GetObjectItem(idx, cpx)->valueint; 
      }
      idx = cJSON_GetObjectItem(host, "idxhc");
      for (int i=0; i<MAX_CP; i++){
         snprintf(cpx, sizeof(cpx), "PC%d", i+1);
         tidxhc[i] = cJSON_GetObjectItem(idx, cpx)->valueint; 
      }
      idx = cJSON_GetObjectItem(host, "idxhpeak");
      for (int i=0; i<MAX_CP; i++){
         snprintf(cpx, sizeof(cpx), "PC%d", i+1);
         tidxhpeak[i] = cJSON_GetObjectItem(idx, cpx)->valueint; 
      }
      idx = cJSON_GetObjectItem(host, "idxcout");
      for (int i=0; i<MAX_CP; i++){
         snprintf(cpx, sizeof(cpx), "PC%d", i+1);
         tidxcout[i] = cJSON_GetObjectItem(idx, cpx)->valueint; 
      }
   }
   return host;
}


int main(void)
{
   char *jsonText;
   cJSON *root;
   int tidx[MAX_CP], tidxhp[MAX_CP], tidxhc[MAX_CP], tidxhpeak[MAX_CP], tidxcout[MAX_CP];

   jsonText = readJson("client.config.json");

   /*  on parse la chaine JSON */
   if(!(root = cJSON_Parse(jsonText))){
      fprintf(stderr, "Erreur parsing JSON\n");
      free(jsonText);

      exit(1);
   }

   printf("IP Domoticz %s\n", getJsonIpDomoticz(root));
   printf("Port MQTT Domoticz : %d\n", getJsonPortMQTT(root));
   if(getJsonTidx(root, tidx, tidxhp, tidxhc, tidxhpeak, tidxcout) != NULL) {
      printf( "Mes Idx total: ");
      for (int i = 0; i< MAX_CP; i++)
         printf("%2d, ", tidx[i]);
      printf("\n");

      printf( "Mes Idx heures pleines: ");
      for (int i = 0; i< MAX_CP; i++)
         printf("%2d, ", tidxhp[i]);
      printf("\n");

      printf( "Mes Idx heures creuses: ");
      for (int i = 0; i< MAX_CP; i++)
         printf("%2d, ", tidxhc[i]);
      printf("\n");

      printf( "Mes Idx heures de pointes: ");
      for (int i = 0; i< MAX_CP; i++)
         printf("%2d, ", tidxhpeak[i]);
      printf("\n");

      printf( "Mes Idx cout: ");
      for (int i = 0; i< MAX_CP; i++)
         printf("%2d, ", tidxcout[i]);
      printf("\n");
   }
   cJSON_Delete(root);
   free(jsonText);
}
