cmake_minimum_required(VERSION 3.10)
project(domoaave_client C)

#sources
add_executable(domoaave_client
    getEnergyAndPublish.c
    getJsonParamters.c
    getIPDomoticzServer.c
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
