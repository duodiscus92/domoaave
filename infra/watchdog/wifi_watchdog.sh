#!/bin/bash

# ==============================
# CONFIGURATION
# ==============================

INTERFACE="wlan0"
PROFILE="wifi_maison"        # nom du profil nmcli
PING_TARGET="8.8.8.8"
MAX_FAIL=3
LOGFILE="/var/log/wifi_watchdog.log"

# ==============================
# VARIABLES
# ==============================

FAIL_COUNT=0

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> $LOGFILE
}

check_connection() {
    ping -I $INTERFACE -c 1 -W 2 $PING_TARGET > /dev/null 2>&1
    return $?
}

reconnect_wifi() {
    log "Tentative reconnexion Wi-Fi"
    nmcli connection up "$PROFILE"
}

reset_network() {
    log "Reset complet du réseau"
    nmcli networking off
    sleep 5
    nmcli networking on
}

# ==============================
# BOUCLE PRINCIPALE
# ==============================

log "Démarrage watchdog Wi-Fi"

while true; do

    if check_connection; then
        FAIL_COUNT=0
    else
        ((FAIL_COUNT++))
        log "Perte réseau détectée ($FAIL_COUNT)"

        if [ $FAIL_COUNT -eq 1 ]; then
            reconnect_wifi
        elif [ $FAIL_COUNT -eq $MAX_FAIL ]; then
            reset_network
            FAIL_COUNT=0
        fi
    fi

    sleep 10
done

