#!/bin/bash
# wifi_toggle.sh
#usage : ./wifi_toggle.sh maison_wifi up
#usage : ./wifi_toggle.sh maison_wifi down

CON_NAME="$1"
ACTION="$2"   # up | down

if ! nmcli -t -f NAME connection show | grep -qx "$CON_NAME"; then
    echo "Connexion '$CON_NAME' inexistante"
    exit 1
fi

if [[ "$ACTION" == "up" ]]; then
    sudo nmcli connection up "$CON_NAME"
elif [[ "$ACTION" == "down" ]]; then
    sudo nmcli connection down "$CON_NAME"
else
    echo "Usage: $0 <connexion> up|down"
    exit 1
fi
