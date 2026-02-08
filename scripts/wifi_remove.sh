#!/bin/bash
# wifi_remove.sh
# usage : ./wifi_remove.sh maison_wifi


CON_NAME="$1"

if ! nmcli -t -f NAME connection show | grep -qx "$CON_NAME"; then
    echo "Connexion '$CON_NAME' inexistante → rien à faire"
    exit 0
fi

echo "Suppression de la connexion '$CON_NAME'"
sudo nmcli connection delete "$CON_NAME"
