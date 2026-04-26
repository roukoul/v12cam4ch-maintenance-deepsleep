# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/lang/fr/).

---

## [7.1.1] - 2025-12-12

### ✅ Ajouté
- **OTA SHA256 Validation** : Validation stricte des mises à jour avec hash SHA256
- **Delayed Write Buffer** : Gestion robuste des boundaries multipart coupés
- **Script Python** : Génération automatique SHA256 pendant le build
- **Logs de debug** : Hexdumps pour diagnostic OTA (à retirer en production)

### 🔧 Modifié
- **CMakeLists.txt** : Custom target pour génération SHA256 au bon moment
- **http_server.c** : Logique OTA avec fenêtre glissante (128 bytes)
- **Ordre champs HTML** : SHA256 avant firmware pour extraction correcte

### 🐛 Corrigé
- **Multipart boundary split** : Détection correcte même si coupé entre paquets
- **Magic byte corruption** : Écriture exclusive des données binaires
- **SHA256 timing** : Script exécuté après génération complète du .bin

### 🧪 Testé
- Upload OTA avec SHA256 valide : ✅ Succès
- Upload OTA avec SHA256 invalide : ✅ Rejet
- Redémarrage automatique : ✅ Fonctionnel
- Intégrité firmware : ✅ Hash correspond

---

## [7.1.0] - 2025-12-11

### ✅ Ajouté
- **Backup/Restore** : Export/Import configuration JSON
- **Logs Download** : Téléchargement logs système avec filtrage
- **Bouton Logs** : Ajout dans menu navigation

### 🔧 Modifié
- **Interface web** : Amélioration responsive mobile
- **Pages validation** : CSS optimisé pour petits écrans

---

## [7.0.0] - 2025-12-10

### ✅ Ajouté
- **OTA Updates** : Mises à jour Over-The-Air via interface web
- **Partitionnement OTA** : 2 partitions app (896KB chacune)
- **Interface /update** : Formulaire upload firmware
- **Lien menu** : Accès direct aux mises à jour

### 🔧 Modifié
- **partitions.csv** : Nouvelle table avec ota_0 et ota_1
- **CMakeLists.txt** : Configuration partition personnalisée

---

## [6.0.0] - 2025-12-09

### ✅ Ajouté
- **WiFi NVS Loading** : Chargement automatique identifiants WiFi
- **AP Fallback** : Mode Access Point si connexion STA échoue
- **SSID AP** : `AepBill_Config` (IP: 192.168.4.1)

### 🐛 Corrigé
- **Reset alarmes** : Correction réinitialisation automatique au boot

---

## [5.0.0] - 2025-12-08

### ✅ Ajouté
- **Surveillance courant** : Capteur ACS712 avec ADC
- **Détection anomalies** : Sous-charge, surcharge avec seuils configurables
- **Alertes buzzer** : Activation automatique avec timeout 5 minutes
- **Calibration** : Zero offset automatique au démarrage

### 🔧 Modifié
- **Interface web** : Ajout page configuration seuils courant
- **NVS** : Persistance calibration et seuils

---

## [4.0.0] - 2025-12-07

### ✅ Ajouté
- **Système d'alarmes** : 20 alarmes programmables par jour
- **Activation par jour** : Sélection jours de la semaine
- **Persistance NVS** : Sauvegarde automatique horaires
- **Interface web** : Configuration complète alarmes

### 🔧 Modifié
- **Gestionnaire alarmes** : Module séparé `alarm_manager.c`
- **Contrôle relais** : Activation automatique selon horaires

---

## [3.0.0] - 2025-12-06

### ✅ Ajouté
- **Serveur HTTP** : Interface web complète
- **Authentification** : Mot de passe configurable
- **Pages web** : Accueil, configuration, status
- **API REST** : Endpoints JSON pour status système

---

## [2.0.0] - 2025-12-05

### ✅ Ajouté
- **Synchronisation NTP** : Horloge précise via Internet
- **Timezone** : Configuration fuseau horaire (CET)
- **RTC virtuel** : Maintien heure même sans NTP

---

## [1.0.0] - 2025-12-04

### ✅ Ajouté
- **Migration ESP-IDF** : Port complet depuis Arduino
- **Architecture modulaire** : Séparation drivers/managers
- **WiFi STA** : Connexion réseau de base
- **GPIO Driver** : Contrôle relais, buzzer, reset
- **Logs système** : ESP_LOG pour debug

### 🔧 Modifié
- **Build system** : CMake + ESP-IDF
- **Configuration** : sdkconfig pour paramètres

---

## [0.9.0] - 2025-12-03

### ✅ Ajouté
- **Version Arduino initiale** : Prototype fonctionnel
- **Alarmes basiques** : 10 alarmes par jour
- **Interface série** : Configuration via Serial Monitor

---

## Légende

- ✅ **Ajouté** : Nouvelles fonctionnalités
- 🔧 **Modifié** : Changements dans fonctionnalités existantes
- 🐛 **Corrigé** : Corrections de bugs
- 🗑️ **Supprimé** : Fonctionnalités retirées
- 🔒 **Sécurité** : Correctifs de vulnérabilités
- 🧪 **Testé** : Résultats de validation
