#!/bin/bash

#cette version evite les conflits avec systemd
#   - systemd redémarre déjà le service
#   - le watchdog ne fait que décider du reboot

#cette version détecte les frezze : l'appli tourne mais elle est bloquée

#!/bin/bash

APP_SERVICE="domoaave_client.service"
HEARTBEAT_FILE="/opt/domoaave/domoaave_client_heartbeat"
MAX_FAIL=3
FAIL=0
TIMEOUT=30   # secondes max sans heartbeat

log() {
    logger -t watchdog_app "$1"
}

check_service() {
    systemctl is-active --quiet "$APP_SERVICE"
}

check_heartbeat() {
    if [ ! -f "$HEARTBEAT_FILE" ]; then
        return 1
    fi

    LAST=$(cat "$HEARTBEAT_FILE")
    NOW=$(date +%s)

    AGE=$((NOW - LAST))

    if [ $AGE -gt $TIMEOUT ]; then
        return 1
    fi

    return 0
}

while true; do

    if check_service && check_heartbeat; then
        FAIL=0
    else
        ((FAIL++))
        log "Service OK mais heartbeat KO ($FAIL)"

        systemctl restart "$APP_SERVICE"
    fi

    if [ $FAIL -ge $MAX_FAIL ]; then
        log "Reboot système (freeze détecté)"
        reboot
    fi

    sleep 15
done
