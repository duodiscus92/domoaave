#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_PC    8
#define THRESHOLD 0.01
#define MONTH_FILE "/opt/domoaave/monthKwh.ref"
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
// Envoi HTTP
// --------------------------------------------------
void send_alert(const char *device, const char *msg) {
    char cmd[1024];

    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST url:port/alert "
        "-H \"Content-Type: application/json\" "
        "-H \"X-API-KEY: xxx\" "
        "-d '{\"device\":\"%s\",\"message\":\"%s\"}'",
        device, msg);

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

    // 1. Lecture fichiers
    load_file("/opt/domoaave/totalKwh.text", totalKwh);
    load_month_ref(MONTH_FILE, monthRef, &refMonth, &refYear);

    // 2. Hostname
    char hostname[64];
    gethostname(hostname, sizeof(hostname));

    // 3. Date
    char date[32];
    time_t now = time(NULL);
    strftime(date, sizeof(date), "%d/%m/%Y", localtime(&now));

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
    char msg[512];
    snprintf(msg, sizeof(msg), "Le %s :\n<pre>", date);

    for (int i = 0; i < MAX_PC; i++) {

        /*if (totalKwh[i] < 0)
            continue;*/

        float C = totalKwh[i];
        float D = totalKwh[i];

        if (monthRef[i] >= 0) 
           D = totalKwh[i] - monthRef[i];

        if (D < 0)
           D = C;

        char tmp[128];
        snprintf(tmp, sizeof(tmp),
            "PC%d : Tot: %9.3f kWh   Mois: %8.3f kWh\n",
            i + 1, C < 0 ? 0.000 : C, D < 0 ? 0.000 : D);
        strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
    }
    strncat(msg, "</pre>\n", sizeof(msg) - strlen(msg) - 1);

    // 6. Envoi
    escape_newlines(msg);
    send_alert(hostname, msg);


    return 0;
}
