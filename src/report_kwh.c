#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_PC    8
#define THRESHOLD 0.01
#define MONTH_FILE "/opt/domoaave/monthKwh.ref"
#define CONFIG_FILE "/opt/domoaave/report_kwh.ini"

static char g_url[128];
static char g_port[16];
static char g_apikey[128];

// --------------------------------------------------
// charge la config (url, port, aPI key
// --------------------------------------------------
void load_config(const char *path, char *url, char *port, char *apikey)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Erreur ouverture config\n");
        return;
    }

    char line[256], key[64], value[128];

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%[^=]=%s", key, value) == 2) {

            if (strcmp(key, "URL") == 0)
                strcpy(url, value);

            else if (strcmp(key, "PORT") == 0)
                strcpy(port, value);

            else if (strcmp(key, "API_KEY") == 0)
                strcpy(apikey, value);
        }
    }

    fclose(f);
}


// --------------------------------------------------
// Remplace \n par \\n pour JSON
// --------------------------------------------------
void escape_newlines(char *msg) {
    char buffer[1024];
    int j = 0;

    for (int i = 0; msg[i] != '\0'; i++) {
        if (msg[i] == '\n') {
            buffer[j++] = '\\';
            buffer[j++] = 'n';
        } else {
            buffer[j++] = msg[i];
        }
    }

    buffer[j] = '\0';
    strcpy(msg, buffer);
}

// --------------------------------------------------
// Envoi HTTP pour telegram
// --------------------------------------------------
void send_data_telegram(const char *device, const char *msg)
{
    char cmd[1024];

    /*snprintf(cmd, sizeof(cmd),
        "curl -s -X POST http://url:port/alert "
        "-H \"Content-Type: application/json\" "
        "-H \"X-API-KEY: xxx\" "
        "-d '{\"device\":\"%s\",\"message\":\"%s\"}'",
        device, msg);*/
    snprintf(cmd, sizeof(cmd),
        "curl -k -s -X POST %s:%s/alert "
        "-H \"Content-Type: application/json\" "
        "-H \"X-API-KEY: %s\" "
        "-d '{\"device\":\"%s\",\"message\":\"%s\"}'",
        g_url, g_port, g_apikey,
        device, msg);
    system(cmd);
}

// --------------------------------------------------
// Envoi HTTP pour fichier csv (sur le serveur Optiplex)
// --------------------------------------------------
void send_data_csv(const char *device, const char *line)
{
    char cmd[1024];

    /*snprintf(cmd, sizeof(cmd),
        "curl -s -X POST http://tinkerforge.ddns.net:6000/conso "
        "--data-urlencode \"line=%s\"",
        line);*/
    snprintf(cmd, sizeof(cmd),
        "curl -k -s -X POST %s:%s/conso "
        "-H \"X-API-KEY: %s\" "
        "--data-urlencode \"line=%s\"",
        g_url, g_port, g_apikey,
        line);
    system(cmd); 
}

// --------------------------------------------------
// Lecture fichier kWh (data ou prev)
// --------------------------------------------------
void load_file(const char *path, float values[]) {

    for (int i = 0; i < MAX_PC; i++)
        values[i] = -1;

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[128], key[32];
    float value;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%[^=]=%f", key, &value) == 2) {
            int pc;
            if (sscanf(key, "PC%d", &pc) == 1 && pc >= 1 && pc <= MAX_PC) {
                values[pc - 1] = value;
            }
        }
    }

    fclose(f);
}

// --------------------------------------------------
// Sauvegarde fichier prev
// --------------------------------------------------
void save_prev(const char *path, float values[]) {

    FILE *f = fopen(path, "w");
    if (!f) return;

    for (int i = 0; i < MAX_PC; i++) {
        //if (values[i] > 0) {
            fprintf(f, "PC%d=%.3f\n", i + 1, values[i]);
            // debug
            printf("PC%d=%.3f\n", i + 1, values[i]);
        //}
    }

    fclose(f);
}

// --------------------------------------------------
// Chargement fichier du 1er jour du moois
// --------------------------------------------------
void load_month_ref(const char *path, float values[], int *month, int *year) {

    for (int i = 0; i < MAX_PC; i++)
        values[i] = -1;

    FILE *f = fopen(path, "r");
    if (!f) return;

    fscanf(f, "MONTH=%d\nYEAR=%d\n", month, year);

    char line[128], key[32];
    float value;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%[^=]=%f", key, &value) == 2) {
            int pc;
            if (sscanf(key, "PC%d", &pc) == 1 && pc >= 1 && pc <= MAX_PC) {
                values[pc - 1] = value;
            }
        }
    }

    fclose(f);
}

// --------------------------------------------------
// Sauvegarde fichier du 1er jour du moois
// --------------------------------------------------
void save_month_ref(const char *path, float values[], int month, int year) {

    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "MONTH=%d\nYEAR=%d\n", month, year);

    for (int i = 0; i < MAX_PC; i++) {
        fprintf(f, "PC%d=%.3f\n", i + 1, values[i]);
    }

    fclose(f);
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main() {

    float totalKwh[MAX_PC];
    float monthRef[MAX_PC];
    int refMonth = 0, refYear = 0;

    // chargement config
    load_config(CONFIG_FILE, g_url, g_port, g_apikey); 

    // 1. Lecture fichiers
    load_file("/opt/domoaave/totalKwh.text", totalKwh);
    load_month_ref(MONTH_FILE, monthRef, &refMonth, &refYear);

    // 2. Hostname
    char hostname[64];
    gethostname(hostname, sizeof(hostname));

    // 3. Date et heure
    char date[32], heure[16];
    time_t now = time(NULL);
    strftime(date, sizeof(date), "%d/%m/%Y", localtime(&now));
    strftime(heure, sizeof(heure), "%H:%M", localtime(&now));

    // 4. Gestion du mois
    struct tm *tm_now = localtime(&now);
    int curMonth = tm_now->tm_mon + 1;
    int curYear  = tm_now->tm_year + 1900;

    // Initialisation ou changement de mois
    if (refMonth != curMonth || refYear != curYear) {

        for (int i = 0; i < MAX_PC; i++) {
            monthRef[i] = totalKwh[i];
        }

        refMonth = curMonth;
        refYear  = curYear;

        save_month_ref(MONTH_FILE, monthRef, refMonth, refYear);
    }

    // 5. Construction message
    char msg_telegram[512], msg_csv[512];
    // pour telegram
    snprintf(msg_telegram, sizeof(msg_telegram), "Le %s :\n<pre>", date);
    // pour fichier csv
    snprintf(msg_csv, sizeof(msg_csv), "%04d-%02d-%02d;%s;%s",
        tm_now->tm_year + 1900,
        tm_now->tm_mon + 1,
        tm_now->tm_mday,
        heure,
        hostname);

    for (int i = 0; i < MAX_PC; i++) {

        float C = totalKwh[i];
        float D = totalKwh[i];

        if (monthRef[i] >= 0) 
           D = totalKwh[i] - monthRef[i];

        if (D < 0)
           D = C;

        // pour fichier csv
        char tmp_csv[32];
        snprintf(tmp_csv, sizeof(tmp_csv), ";%.3f", C < 0 ? 0.0 : C);
        strncat(msg_csv, tmp_csv, sizeof(msg_csv) - strlen(msg_csv) - 1);

        // pour message telegram
        char tmp_telegram[128];
        snprintf(tmp_telegram, sizeof(tmp_telegram),
            "PC%d : Tot: %9.3f kWh   Mois: %8.3f kWh\n",
            i + 1, C < 0 ? 0.000 : C, D < 0 ? 0.000 : D);
        strncat(msg_telegram, tmp_telegram, sizeof(msg_telegram) - strlen(msg_telegram) - 1);
    }
    strncat(msg_telegram, "</pre>\n", sizeof(msg_telegram) - strlen(msg_telegram) - 1);

    // 6. Envoi
    escape_newlines(msg_telegram);
    send_data_telegram(hostname, msg_telegram);
    send_data_csv(hostname, msg_csv);

    return 0;
}
