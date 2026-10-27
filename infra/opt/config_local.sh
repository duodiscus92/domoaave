#!/bin/bash
# Configuration commune aux sites Domoaave.
# Utilisée par scripts/update_config.sh

# Correspondance SSID Wi-Fi -> adresse IP du serveur
# Remplacer les valeurs d'exemple par les valeurs réelles.
declare -A SERVERS_BY_SSID=(
    ["SSID_DEV"]="192.168.1.100"
    ["SSID_PROD"]="192.168.1.200"
)

REMOTE_USER="jehrlich"
KEY="/home/jehrlich/.ssh/id_ed25519_domoticz"
SRC="/home/jehrlich/domoaave/config/"
DST="/opt/domoaave/"
