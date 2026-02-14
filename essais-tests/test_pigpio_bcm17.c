#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <unistd.h>

volatile int pulses[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char tgpio[] = {11, 9, 10, 22, 17, 4, 99, 5};
unsigned char tindex[] = {0, 1, 2, 3, 4, 5, 6, 7};

/* Callback pigpio */
void alert_callback(int gpio,
                    int level,
                    unsigned int tick,
                    void *userdata)
{
    unsigned char *i = (unsigned char*) userdata;

    if (level == FALLING_EDGE) {
        if (*i < 0 || *i >= 8){
           printf("Erreur : IRQ d'origine inconnue\n");
           exit(1);
        }
        pulses[*i]++;
        printf("ac : IRQ on GPIO %d -> pulses[%u] = %d\n", gpio, *i, pulses[*i]);
        fflush(stdout);
    }
}


int main(void)
{
    int pi, /*bcm,*/ i;
    unsigned char bcm;

    /* Connexion au daemon pigpiod */
    pi = gpioInitialise();
    if (pi < 0) {
        fprintf(stderr, "pigpio_start failed\n");
        return 1;
    }

    printf("pigpio connected (pi=%d)\n", pi);

    for (i = 0; i< 8; i++) {
       if ((bcm = tgpio[i]) == 99)
          continue;
       gpioSetMode(bcm, PI_INPUT);
       gpioSetPullUpDown(bcm, PI_PUD_OFF);
       gpioGlitchFilter(bcm, 5000);   // 5000 µs = 5 ms
       gpioSetAlertFuncEx(bcm, alert_callback, &tindex[i]);
    }


    printf("Waiting for falling edges on GPIO 5...\n");

    /* Boucle simple */
    while (1) {
        sleep(2);
        //printf("still alive, pulses={%u,%u}\n", pulses[0], pulses[1]);
        fflush(stdout);
    }

    gpioTerminate();
    return 0;
}
