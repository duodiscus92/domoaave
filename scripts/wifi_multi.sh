#!/bin/bash

# Usage:
# ./wifi_multi.sh <con_name> <ssid> <psk> <priority>

CON_NAME="$1"
SSID="$2"
PSK="$3"
PRIO="$4"

if [[ -z "$CON_NAME" || -z "$SSID" || -z "$PSK" || -z "$PRIO" ]]; then
    echo "Usage: $0 <con_name> <ssid> <psk> <priority>"
    exit 1
fi

if nmcli -t -f NAME connection show | grep -qx "$CON_NAME"; then
    echo "Connexion '$CON_NAME' existe déjà → mise à jour"
else
    echo "Création de la connexion '$CON_NAME'"
    sudo nmcli connection add type wifi con-name "$CON_NAME" ssid "$SSID"
fi

sudo nmcli connection modify "$CON_NAME" \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "$PSK" \
    connection.autoconnect yes \
    connection.autoconnect-priority "$PRIO"

echo "Connexion '$CON_NAME' configurée (priorité $PRIO)"
