#!/bin/bash
# wifi_install.sh
#usage : ./wifi_install.sh maison_wifi MaisonSSID MonMotDePasse


CON_NAME="$1"
SSID="$2"
PSK="$3"

if nmcli -t -f NAME connection show | grep -qx "$CON_NAME"; then
    echo "Connexion '$CON_NAME' déjà existante → rien à faire"
    exit 0
fi

echo "Création de la connexion WiFi '$CON_NAME'"

sudo nmcli connection add type wifi con-name "$CON_NAME" ssid "$SSID"
sudo nmcli connection modify "$CON_NAME" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$PSK"

echo "Connexion '$CON_NAME' installée"
