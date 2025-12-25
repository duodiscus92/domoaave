#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t now = time(NULL);          // Récupère l'heure actuelle
    struct tm *local = localtime(&now);

    int mois = local->tm_mon + 1;     // tm_mon : 0 = janvier, 11 = décembre

    printf("Nous sommes au mois : %d\n", mois);

    return 0;
}
