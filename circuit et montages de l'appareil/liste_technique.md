# Liste Technique des Composants (BOM)

Cette liste détaille l'ensemble du matériel nécessaire pour la construction de l'appareil **AepBill Smart System**.

## 1. Cœur du Système
| Composant | Modèle recommandé | Quantité | Description / Notes |
| :--- | :--- | :---: | :--- |
| **Microcontrôleur** | **ESP32 DevKit V1** (WROOM-32) | 1 | Carte de développement principale. Wi-Fi + Bluetooth. |

## 2. Capteurs et Modules
| Composant | Modèle | Interface | Pin (ESP32) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Horloge RTC** | **DS3231** (Module I2C) | I2C (0x68) | SDA: 21, SCL: 22 | Pour le maintien de l'heure hors ligne. Pile CR2032 requise. |
| **Écran LCD** | **LCD 1602** ou **2004** avec **Module I2C** | I2C (0x27) | SDA: 21, SCL: 22 | Affichage des états et de l'IP. Adresse par défaut 0x27. |
| **Capteur de Courant** | **ACS712** (Modèle 5A) | Analogique | GPIO 34 (ADC1_CH6) | Mesure du courant. Sensibilité 185mV/A. **(⚠️ Code présent mais DÉSACTIVÉ dans firmware actuel)** |
| **Relais** | **Module Relais 5V** (1 canal) | Numérique | GPIO 16 | Contrôle de la charge (ON/OFF). Active High ou Low selon module (Code: High=ON). |

## 3. Interface Utilisateur & Signalisation
| Composant | Type | Pin (ESP32) | Fonction |
| :--- | :--- | :--- | :--- |
| **Buzzer** | Buzzer Passif ou Actif 5V | GPIO 25 | Alarmes sonores et feedback. |
| **Bouton Poussoir** | Tactile Switch | GPIO 23 | **Factory Reset** (Maintenir > 5s). Connecter entre GPIO et GND. |
| **Bouton Poussoir** | Tactile Switch | GPIO 27 | **Redémarrage** (Appui court). Connecter entre GPIO et GND. |

## 4. Alimentation et Divers
| Composant | Quantité | Description |
| :--- | :---: | :--- |
| **Alimentation** | 1 | **5V DC** (1A min). Pour alimenter l'ESP32 et les relais (via VIN ou USB). |
| **Câbles Jumper** | Lot | Dupont M-M, M-F, F-F pour les connexions. |
| **Breadboard** | 1 | Pour le prototypage (optionnel si soudé sur PCB). |
| **Résistances** | 2 | 10kΩ (Optionnel: Pull-up pour les boutons si le pull-up interne ne suffit pas). |

## Synthèse des Connexions (Pinout)

| GPIO | Fonction | Composant |
| :--- | :--- | :--- |
| **GPIO 16** | Sortie Numérique | Relais (Charge) |
| **GPIO 21** | I2C SDA | Écran LCD & RTC DS3231 |
| **GPIO 22** | I2C SCL | Écran LCD & RTC DS3231 |
| **GPIO 23** | Entrée (Pull-up) | Bouton Reset Usine |
| **GPIO 25** | Sortie | Buzzer |
| **GPIO 27** | Entrée (Pull-up) | Bouton Restart |
| **GPIO 34** | Entrée Analogique | Sortie capteur ACS712 |
