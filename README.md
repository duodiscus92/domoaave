# Domoaave

Système de mesure des consommations électriques de l'AAVE, basé sur Raspberry Pi, Domoticz et des compteurs à impulsions.

## 1. Programmes

- `domoaave_client` : acquisition des impulsions des compteurs et publication des données vers Domoticz.
- `blink23` : simulateur d'impulsions sur GPIO13 (broche physique 33). Il permet de vérifier le fonctionnement des cartes à travers l'incrémentation des compteurs de test PC8 dans Domoticz.

Le simulateur est utilisé sur les cartes concernées du site. Il est lancé par le service systemd `blink23.service`.

## 2. Dépendances de compilation

Installer les outils et bibliothèques de développement nécessaires :

```bash
sudo apt update
sudo apt install build-essential cmake libmosquitto-dev libpigpio-dev libcjson-dev libgpiod-dev
