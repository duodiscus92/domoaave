#!/bin/bash

#SERVER="jehrlich@192.168.1.44"
SERVER="jehrlich@marconi.local"
KEY="/home/jehrlich/.ssh/id_ed25519_domoticz"
SRC="/home/jehrlich/domoaave/config/"
DST="/opt/domoaave/"
#scp -i "$KEY" ${SERVER}:${SRC}*.json "$DST"
rsync -avz -e "ssh -i $KEY" ${SERVER}:${SRC} "$DST"
