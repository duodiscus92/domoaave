#!/bin/bash

# Synchronisation de la configuration depuis le serveur du site.

set -euo pipefail

CONFIG="/opt/domoaave/config_local.sh"

if [[ ! -r "$CONFIG" ]]; then
    echo "ERREUR : configuration absente ou illisible : $CONFIG" >&2
    exit 1
fi

source "$CONFIG"

SSID=$(nmcli -t -f IN-USE,SSID dev wifi list |
       awk -F: '$1=="*" {sub(/^\*:/, ""); print; exit}')

if [[ -z "$SSID" ]]; then
    echo "ERREUR : aucun SSID Wi-Fi actif détecté." >&2
    exit 1
fi

echo "SSID détecté : $SSID"

SERVER_IP="${SERVERS_BY_SSID[$SSID]:-}"

if [[ -z "$SERVER_IP" ]]; then
    echo "ERREUR : aucun serveur associé au SSID détecté." >&2
    echo "Synchronisation annulée." >&2
    exit 1
fi

SERVER="${REMOTE_USER}@${SERVER_IP}"

echo "Serveur sélectionné : $SERVER"

rsync -avz -e "ssh -i $KEY" "${SERVER}:${SRC}" "$DST"
