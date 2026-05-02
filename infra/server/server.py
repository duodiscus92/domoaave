from flask import Flask, request, jsonify
import requests
import smtplib
import os
from email.mime.text import MIMEText
from datetime import datetime

#API_KEY = "CHANGE_ME_123456"
API_KEY = os.environ.get("API_KEY")
app = Flask(__name__)
CSV_FILE = "global_conso.csv"

# CONFIG
TELEGRAM_TOKEN = "8791098770:AAGc40oQvNqLeCimkyrH6eAR5bdwFRJUL14"
CHAT_ID = "8794909789"
EMAIL_TO = "jacques.ehrlich@orange.fr"

# --------------------------------------------------
# Initialisation du fichier CSV (si absent)
# --------------------------------------------------
def init_csv():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "w") as f:
            f.write("sep=;\n")
            f.write("date;site;PC1;PC2;PC3;PC4;PC5;PC6;PC7;PC8\n")

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
# Route CSV (nouveau)
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
# MAIN
# --------------------------------------------------
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=6000)

