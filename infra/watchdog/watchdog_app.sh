#!/bin/bash

#cette version evite les conflits avec systemd
#   - systemd redémarre déjà le service
#   - le watchdog ne fait que décider du reboot

#cette version détecte les frezze : l'appli tourne mais elle est bloquée

#!/bin/bash
source /opt/domoaave/report_kwh.ini

DEVICE=$(hostname)

APP_SERVICE="domoaave_client.service"
HEARTBEAT_FILE="/opt/domoaave/domoaave_client_heartbeat"
MAX_FAIL=4
FAIL=0
TIMEOUT=180   # secondes max sans heartbeat

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

send_alert() {
    MSG="$1"

    curl -s --cacert "$CERT" POST "$URL:$PORT/alert" \
        -H "Content-Type: application/json" \
        -H "X-API-KEY: $API_KEY" \
        -d "{\"device\":\"$DEVICE\",\"message\":\"$MSG\"}" \
        >/dev/null 2>&1
}

send_alert "Watchdog started"
while true; do
#:<<'COMMENT'
	if ! check_service; then

	    ((FAIL++))

	    if [ $FAIL -ge 2 ]; then
	               MSG="Service KO ($FAIL)"
	               log "$MSG"
	               send_alert "$MSG"
	               send_alert "Redémarrage de $APP_SERVICE"
	        systemctl restart "$APP_SERVICE"
	    fi

	elif ! check_heartbeat; then
#COMMENT
#	if ! check_heartbeat; then

	    ((FAIL++))

	    if [ $FAIL -ge 2 ]; then
	               MSG="Heartbeat KO ($FAIL)"
	               log "$MSG"
	               send_alert "$MSG"
	               send_alert "Redémarrage de $APP_SERVICE"
	        systemctl restart "$APP_SERVICE"
	    fi

	else
	    FAIL=0
	fi
	ֶֶֶ

    if [ $FAIL -ge $MAX_FAIL ]; then
        MSG="Reboot système (freeze détecté)"
        log "$MSG"
        send_alert "$MSG"
        reboot
    fi

    sleep 30
done
