#include <stdio.h>
#include <time.h>
#include <stdbool.h>

bool dans_tranche(int h_deb, int m_deb, int h_fin, int m_fin)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    int maintenant = local->tm_hour * 60 + local->tm_min;
    int debut      = h_deb * 60 + m_deb;
    int fin        = h_fin * 60 + m_fin;

    if (debut <= fin) {
        // Tranche classique (ex: 08:00 → 18:00)
        return (maintenant >= debut && maintenant < fin);
    } else {
        // Tranche qui passe par minuit (ex: 22:00 → 06:00)
        return (maintenant >= debut || maintenant < fin);
    }
}

int main(void)
{
    int h_deb = 6, m_deb = 30;
    int h_fin = 22,  m_fin = 0;

    if (dans_tranche(h_deb, m_deb, h_fin, m_fin)) {
        printf("Nous sommes DANS la tranche horaire.\n");
    } else {
        printf("Nous sommes HORS de la tranche horaire.\n");
    }

    return 0;
}
