# Rapport d'Utilisation des GPIO (ESP32 - Projet AepBill)

Ce document recense l'état actuel des broches (GPIO) de l'ESP32 pour le projet **classicfinaltestee** et identifie les broches disponibles pour l'ajout de nouveaux relais.

## 1. Broches Actuellement Utilisées (NE PAS TOUCHER)

Ces broches sont déjà affectées par le code actuel (`main.c`, `drivers/*`) et le schéma de câblage.

| GPIO | Fonction | Composant / Usage | Note |
| :--- | :--- | :--- | :--- |
| **GPIO 1** | UART0_TX | Console Série / Téléversement | Réservé système |
| **GPIO 3** | UART0_RX | Console Série / Téléversement | Réservé système |
| **GPIO 6-11**| Flash SPI | Mémoire Flash ESP32 | **INTERDIT** (Fait planter l'ESP) |
| **GPIO 13** | Sortie | Heartbeat LED (Watchdog) | Clignote pour indiquer la vie du système |
| **GPIO 16** | Sortie | **Relais 1** (Charge Principale) | Déjà utilisé |
| **GPIO 21** | I2C SDA | Écran LCD & Horloge RTC | Bus de communication |
| **GPIO 22** | I2C SCL | Écran LCD & Horloge RTC | Bus de communication |
| **GPIO 23** | Entrée | Bouton Reset Usine | Pull-up interne activé |
| **GPIO 25** | Sortie (PWM) | Buzzer | Alarme sonore |
| **GPIO 27** | Entrée | Bouton Restart | Pull-up interne activé |
| **GPIO 34** | Entrée Analog | Capteur de Courant (ACS712) | Input Only |

## 2. Broches Libres Recommandées pour Relais (Sorties)

Ces broches sont **sûres** à utiliser pour piloter des modules relais (Output). Elles ne posent pas de conflit de démarrage (Bootstrap) ni de conflit matériel connu sur les cartes DevKit V1 standard.

| GPIO | Recommandation | Remarques |
| :--- | :---: | :--- |
| **GPIO 4** | ✅ **EXCELLENT** | Totalement libre. |(choisie pour relais2
)
| **GPIO 17** | ✅ **EXCELLENT** | Libre (attention si version WROVER avec PSRAM, mais OK sur WROOM). |
| **GPIO 18** | ✅ **EXCELLENT** | Libre (VSPI CLK non utilisé). |
| **GPIO 19** | ✅ **EXCELLENT** | Libre (VSPI MISO non utilisé). |
| **GPIO 26** | ✅ **EXCELLENT** | Libre (DAC2). |(choisi pour relais3
)
| **GPIO 32** | ✅ **EXCELLENT** | Libre. |(choisi pour relais4)
| **GPIO 33** | ✅ **EXCELLENT** | Libre. |
| **GPIO 14** | 🆗 **BON** | Libre. Émet un signal PWM bref au boot, mais généralement sûr pour relais. |
| **GPIO 5** | ⚠️ **MOYEN** | *Strapping Pin*. Doit être HIGH au boot (pour carte SD). Si votre relais la tire à LOW au démarrage, l'ESP ne bootera pas. À utiliser avec prudence ou si le relais est isolé. |

## 3. Broches Libres pour Capteurs (Entrées Uniquement)

Ces broches ne peuvent **PAS** commander de relais (elles ne peuvent pas générer de tension), mais sont parfaites pour ajouter des boutons ou capteurs additionnels.

| GPIO | Type | Usage Possible |
| :--- | :--- | :--- |
| **GPIO 35** | Input Only | Capteur, Bouton |
| **GPIO 36** (VP)| Input Only | Capteur, Bouton |
| **GPIO 39** (VN)| Input Only | Capteur, Bouton |

## 4. Broches à ÉVITER Absolument

| GPIO | Risque | Pourquoi ? |
| :--- | :--- | :--- |
| **GPIO 0** | 🛑 **DANGER** | Pin de Boot. Si tirée à LOW, l'ESP attend une programmation et ne démarre pas. |
| **GPIO 2** | 🛑 **DANGER** | Souvent reliée à la LED bleue interne. *Strapping Pin*. |
| **GPIO 12** | 🛑 **DANGER** | *Strapping Pin* (Voltage Flash). Si tirée à HIGH au boot -> 1.8V -> **Brick** possible ou non-démarrage. |
| **GPIO 15** | 🛑 **DANGER** | *Strapping Pin* (Debug). Influence les logs de démarrage. |

## Résumé : Vos Meilleurs Choix
Pour ajouter des relais sans risque, utilisez dans l'ordre de préférence :
1. **GPIO 4, 17, 18, 19, 26, 32, 33** (7 relais possibles sans aucun souci).
