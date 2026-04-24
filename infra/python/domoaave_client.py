import requests
import sys
import os

# lecture config simple
def load_config():
    config = {}
    with open("/home/jehrlich/domoaave/infra/python/domoaave.conf") as f:
        for line in f:
            k, v = line.strip().split("=", 1)
            config[k] = v
    return config

cfg = load_config()

SERVER_URL = cfg["SERVER_URL"]
API_KEY = cfg["API_KEY"]

def send_alert(device, message):
    headers = {
        "Content-Type": "application/json",
        "X-API-KEY": API_KEY
    }

    data = {
        "device": device,
        "message": message
    }

    r = requests.post(SERVER_URL, json=data, headers=headers, timeout=5)
    print("Status:", r.status_code)
    print("Response:", r.text)

if __name__ == "__main__":
    send_alert(sys.argv[1], " ".join(sys.argv[2:]))

