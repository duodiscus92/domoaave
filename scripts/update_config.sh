#!/bin/bash

SERVER="jehrlich@192.168.1.44"
KEY="/home/jehrlich/.ssh/id_domotique"
SRC="/home/jehrlich/domoaave/config/"
DST="/home/jehrlich/domoaave/config/"

#scp -i "$KEY" ${SERVER}:${SRC}*.json "$DST"
rsync -avz -e "ssh -i $KEY" ${SERVER}:${SRC} "$DST"
