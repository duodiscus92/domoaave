cmake_minimum_required(VERSION 3.10)
project(domoaave_client C)

#sources
add_executable(domoaave_client
    src/getEnergyAndPublish.c
    src/getJsonParamters.c
    src/getIPDomoticzServer.c
)

#librairies
target_link_libraries(domoaave_client
    mosquitto
#    wiringPi
    pigpio
    pthread
    rt
    cjson
)

install(TARGETS domoaave_client
        DESTINATION /opt/domoaave)

#include
target_include_directories(domoaave_client PRIVATE include)

# Simulateur d'impulsions GPIO pour les tests
add_executable(blink23
    simulateurs/blink23.c
)

target_link_libraries(blink23
    gpiod
)

install(TARGETS blink23
        DESTINATION /opt/domoaave)

# Synchronisation automatique de la configuration
install(PROGRAMS scripts/update_config.sh
        DESTINATION /opt/domoaave)
