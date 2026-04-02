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
