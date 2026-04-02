#!/bin/bash

# ce script est appelé par cron vers minuit

# Charger config locale
source /opt/domoaave/config_local.sh

#SERVER="jehrlich@192.168.1.44"
#KEY="/home/jehrlich/.ssh/id_ed25519_domoticz"
#SRC="/home/jehrlich/domoaave/config/"
#DST="/opt/domoaave/"
rsync -avz -e "ssh -i $KEY" ${SERVER}:${SRC} "$DST"
