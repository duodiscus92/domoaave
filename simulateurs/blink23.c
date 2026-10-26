
#include <gpiod.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 13
#define PULSE_MS 80
#define PERIOD_MS 5000

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void sleep_ms(long ms)
{
    struct timespec ts = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR
           && !stop_requested) {
    }
}

int main(void)
{
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *line_config = NULL;
    struct gpiod_request_config *request_config = NULL;
    struct gpiod_line_request *request = NULL;
    int result = 1;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        goto cleanup;
    }

    settings = gpiod_line_settings_new();
    line_config = gpiod_line_config_new();
    request_config = gpiod_request_config_new();

    if (!settings || !line_config || !request_config) {
        fprintf(stderr, "Erreur de création de la configuration GPIO\n");
        goto cleanup;
    }

    if (gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0 ||
        gpiod_line_settings_set_output_value(
            settings, GPIOD_LINE_VALUE_ACTIVE) < 0) {
        perror("Configuration GPIO");
        goto cleanup;
    }

    unsigned int offset = GPIO_LINE;

    if (gpiod_line_config_add_line_settings(
            line_config, &offset, 1, settings) < 0) {
        perror("gpiod_line_config_add_line_settings");
        goto cleanup;
    }

    gpiod_request_config_set_consumer(request_config, "blink23");

    request = gpiod_chip_request_lines(chip, request_config, line_config);
    if (!request) {
        perror("gpiod_chip_request_lines");
        goto cleanup;
    }

    printf("Simulateur actif : GPIO13, impulsion basse de %d ms toutes les %d ms\n",
           PULSE_MS, PERIOD_MS);
    fflush(stdout);

    while (!stop_requested) {
        /* Impulsion basse : le compteur reçoit le front descendant. */
        if (gpiod_line_request_set_value(
                request, GPIO_LINE, GPIOD_LINE_VALUE_INACTIVE) < 0) {
            perror("Passage à l'état bas");
            goto cleanup;
        }

        sleep_ms(PULSE_MS);
        if (stop_requested)
            break;

        if (gpiod_line_request_set_value(
                request, GPIO_LINE, GPIOD_LINE_VALUE_ACTIVE) < 0) {
            perror("Retour à l'état haut");
            goto cleanup;
        }

        sleep_ms(PERIOD_MS - PULSE_MS);
    }

    result = 0;

cleanup:
    if (request) {
        /* Laisser la sortie au niveau haut avant de libérer la ligne. */
        gpiod_line_request_set_value(
            request, GPIO_LINE, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_release(request);
    }

    if (request_config)
        gpiod_request_config_free(request_config);
    if (line_config)
        gpiod_line_config_free(line_config);
    if (settings)
        gpiod_line_settings_free(settings);
    if (chip)
        gpiod_chip_close(chip);

    return result;
}
