from flask import Flask, request, jsonify
import requests
import smtplib
import os
from email.mime.text import MIMEText
from datetime import datetime
import json
import paho.mqtt.publish as publish

#CONFIG
API_KEY = os.environ.get("API_KEY")
TELEGRAM_TOKEN = os.environ.get("TELEGRAM_TOKEN")
CHAT_ID = os.environ.get("CHAT_ID")
EMAIL_TO = os.environ.get("EMAIL_TO")
CSV_FILE = "global_conso.csv"
MQTT_BROKER = "localhost"
MQTT_PORT = 1884

app = Flask(__name__)

# --------------------------------------------------
# Initialisation du fichier CSV (si absent)
# --------------------------------------------------
def init_csv():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "w") as f:
            f.write("sep=;\n")
            f.write("date;heure;site;PC1;PC2;PC3;PC4;PC5;PC6;PC7;PC8\n")

def send_telegram(msg):
    url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendMessage"
    requests.post(url, data={"chat_id": CHAT_ID, "text": msg, "parse_mode": "HTML"})

def send_email(msg):
    m = MIMEText(msg)
    m["Subject"] = "Alerte Domoaave"
    m["From"] = "server@domoaave"
    m["To"] = EMAIL_TO

    with smtplib.SMTP("smtp.gmail.com", 587) as s:
        s.starttls()
        s.login("TON_EMAIL", "APP_PASSWORD")
        s.send_message(m)

# --------------------------------------------------
# Route CSV
# --------------------------------------------------
@app.route('/conso', methods=['POST'])
def conso():

    init_csv()

    line = request.form.get("line")

    if not line:
        return "Missing data", 400

    with open(CSV_FILE, "a") as f:
        f.write(line + "\n")

    return "OK", 200


# --------------------------------------------------
# Route Telegram
# --------------------------------------------------
@app.route("/alert", methods=["POST"])
def alert():
    if request.headers.get("X-API-KEY") != API_KEY:
        return "Unauthorized", 403

    data = request.json
    device = data.get("device", "unknown")
    message = data.get("message", "")

    full_msg = f"{device}: {message}"
    print(full_msg)

    send_telegram(full_msg)
    return jsonify({"status": "ok"})


# --------------------------------------------------
# Route broker pour domoticz
# --------------------------------------------------
@app.route('/broker', methods=['POST'])
def broker():

    # Vérification API KEY
    if request.headers.get("X-API-KEY") != API_KEY:
        return "Unauthorized", 403

    # Lecture JSON reçu
    data = request.get_json(silent=True)

    print(f"Broker request from {request.remote_addr}")
    print(data)

    if not data:
        return jsonify({"error": "No JSON received"}), 400

    # Topic MQTT
    topic = data.get("topic")

    # Payload MQTT
    payload = data.get("payload")

    if not topic or payload is None:
        return jsonify({"error": "Missing topic or payload"}), 400

    # Sécurité : topic autorisé uniquement
    if topic != "domoticz/in":
        return jsonify({"error": "Invalid topic"}), 400

    try:

        # Publication MQTT vers Mosquitto local
        publish.single(
            topic,
            payload=json.dumps(payload),
            hostname=MQTT_BROKER,
            port=MQTT_PORT
        )

        print(f"MQTT -> {topic} : {payload}")

        return jsonify({"status": "published"}), 200

    except Exception as e:
        print(f"MQTT ERROR: {e}")
        return jsonify({"error": str(e)}), 500

# --------------------------------------------------
# MAIN
# --------------------------------------------------
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=6000)
#    app.run(
#       host="0.0.0.0", 
#       port=6000,
#       ssl_context=('/home/jehrlich/domoaave/certs/cert.pem', 
#                    '/home/jehrlich/domoaave/certs/key.pem')
#    )
