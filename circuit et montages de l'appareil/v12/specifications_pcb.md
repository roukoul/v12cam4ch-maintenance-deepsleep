# Dossier de Fabrication PCB — Système AepBill v12.0

Ce document contient les spécifications techniques complètes pour la réalisation de la carte électronique (PCB) du système de contrôle **AepBill**. 

---

## 1. Description du Projet
- **Nom du Système** : AepBill
- **Propriétaire/Concepteur** : Eddadssi Ahmed
- **Microcontrôleur** : ESP32 DevKit V1 (30 broches)
- **Fonctions** : Gestion de 4 relais 5V, affichage LCD I2C, horloge RTC DS3231, alarmes sonores et boutons de commande.

---

## 2. Liste des Composants (BOM)

| Repère | Composant | Quantité | Empreinte (Footprint) |
|:--- |:--- |:---:|:--- |
| **U1** | ESP32 DevKit V1 (30 pins) | 1 | Header Double Rangée 15 pins (DIP) |
| **MOD1-4**| Module Relais 5V (Intégré) | 4 | Connecteur Header 3 ou 4 pins |
| **Q1-4** | Transistor 2N2222 | 4 | TO-92 (ou SOT-23) |
| **R1-4** | Résistance 1kΩ | 4 | Axiale ou 0805 |
| **BUZ1** | Buzzer Actif (5V) | 1 | DIP (Pas 7.6mm) |
| **RTC1** | Module DS3231 (I2C) | 1 | Header Femelle 6 pins |
| **LCD1** | Module LCD 16x2 I2C | 1 | Header Mâle 4 pins |
| **BT1** | Bouton Poussoir (Restart) | 1 | Tactile 6x6mm |
| **BT2** | Bouton Poussoir (Reset Factory) | 1 | Tactile 6x6mm |
| **U2** | Convertisseur DC-DC Buck (12V -> 5V)| 1 | LM2596 (ou module mini-buck) |
| **CONN1** | Bornier d'entrée 12V | 1 | Bornier à vis (Screw Terminal) 5.08mm |

---

## 3. Table des Connexions (Pinout)

| Signal | Broche ESP32 (GPIO) | Destination Finale |
|:--- |:---:|:--- |
| **Relais S** | **GPIO 16** | -> Base Q1 -> **IN** Module 1 |
| **Relais A** | **GPIO 4** | -> Base Q2 -> **IN** Module 2 |
| **Relais B** | **GPIO 26** | -> Base Q3 -> **IN** Module 3 |
| **Relais C** | **GPIO 32** | -> Base Q4 -> **IN** Module 4 |
| **Buzzer** | **GPIO 25** | -> Borne + du Buzzer |
| **I2C SDA** | **GPIO 21** | -> SDA (LCD + DS3231) |
| **I2C SCL** | **GPIO 22** | -> SCL (LCD + DS3231) |
| **Bouton Reset** | **GPIO 23** | -> BT2 (Bouton vers GND) |
| **Bouton Restart**| **GPIO 27** | -> BT1 (Bouton vers GND) |
| **LED Témoin** | **GPIO 13** | -> LED Heartbeat (Optionnelle) |

---

## 4. Schéma d'Alimentation
L'alimentation doit être conçue en cascade pour isoler les bruits de commutation :
1.  **Entrée** : 12V DC via bornier.
2.  **Ligne 5V** : Générée par le régulateur DC-DC à partir du 12V.
    - Alimente les **Modules Relais**, le **Buzzer** et le **LCD**.
3.  **Ligne ESP32** : Relier la sortie 5V du régulateur à la broche **Vin (5V)** de l'ESP32.
    - *Note : L'ESP32 possède son propre régulateur interne qui convertira ce 5V en 3.3V pour son fonctionnement.*

---

## 5. Personnalisation et Marquage (Silk Screen)
Veuillez imprimer les textes et étiquettes suivants sur la face supérieure (**Top Silk**) :
- **Identification des Composants** : Chaque composant doit avoir son nom (Repère) imprimé juste à côté de son emplacement pour faciliter la soudure manuelle (ex: **R1, R2, Q1, Q2, MOD1, BT1, BT2, U1, U2**, etc.). 
- **Titre Principal** : "AepBill par Eddadssi Ahmed"
- **Version** : "v12.0 Final"
- **Marquage des Bornes** : Identifier clairement toutes les connexions sur les borniers et headers (ex: "SDA", "SCL", "5V", "Vin", "GPIO 16", etc.).

---

## 6. Exigences de Fabrication
Le fabricant doit fournir :
1.  Les plaques de PCB nues.
2.  Le perçage complet (Drill holes).
3.  La sérigraphie (Solder Mask) de couleur au choix (Vert/Bleu recommandés).

**Note importante** : L'assemblage et la soudure des composants listés ci-dessus seront effectués manuellement par l'utilisateur. Aucun fichier CPL n'est requis.
