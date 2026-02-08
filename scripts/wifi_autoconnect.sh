x#!/bin/bash
# wifi_autoconnect.sh
#usage : ./wifi_autoconnect.sh maison_wifi yes
#usage : ./wifi_autoconnect.sh maison_wifi no

CON_NAME="$1"
MODE="$2"   # yes | no

if ! nmcli -t -f NAME connection show | grep -qx "$CON_NAME"; then
    echo "Connexion '$CON_NAME' inexistante"
    exit 1
fi

if [[ "$MODE" != "yes" && "$MODE" != "no" ]]; then
    echo "Usage: $0 <connexion> yes|no"
    exit 1
fi

sudo nmcli connection modify "$CON_NAME" connection.autoconnect "$MODE"
echo "Autoconnect pour '$CON_NAME' = $MODE"
