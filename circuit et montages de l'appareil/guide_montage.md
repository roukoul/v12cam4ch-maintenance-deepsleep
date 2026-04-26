# Guide de Montage (Câblage)

Voici les étapes pour relier les composants de votre appareil **AepBill Smart System**.

> [!CAUTION]
> **PRÉCAUTIONS IMPORTANTES :**
> 1. **DÉBRANCHEZ** toute source d'alimentation avant de commencer.
> 2. **Vérifiez** deux fois les connexions VCC (5V) et GND. Une inversion peut détruire les composants.
> 3. Ne travaillez pas sur une surface conductrice (métal).

## Étape 1 : Préparation de l'Alimentation
L'ESP32 a besoin d'une alimentation stable.
1. Reliez la sortie **5V** de votre alimentation au rail **(+)** de votre breadboard.
2. Reliez la masse (**GND**) de votre alimentation au rail **(-)** de votre breadboard.
3. Reliez la pin **VIN** (ou 5V) de l'ESP32 au rail **(+)**.
4. Reliez une pin **GND** de l'ESP32 au rail **(-)**.

## Étape 2 : Bus I2C (Écran LCD + RTC)
Ces deux modules partagent les mêmes pins sur l'ESP32.

| Pin ESP32 | Connexion LCD | Connexion RTC (DS3231) |
| :---: | :---: | :---: |
| **GPIO 21** | SDA | SDA |
| **GPIO 22** | SCL | SCL |
| **5V (Rail)** | VCC | VCC |
| **GND (Rail)** | GND | GND |

## Étape 3 : Sorties (Relais & Buzzer)
Pour commander la charge et les alertes.

### Relais (Contrôle Charge)
- **Pin Signal** : Reliez **GPIO 16** à l'entrée **IN** du module relais.
- **Alimentation** : VCC vers 5V, GND vers GND.

### Buzzer
- **Pin Signal** : Reliez **GPIO 25** à la borne positive (+) du buzzer.
- **Masse** : Reliez la borne négative (-) au GND.

## Étape 4 : Entrées (Boutons)
L'ESP32 est configuré avec des "Pull-up" internes. **Pas besoin de résistances externes.**

### Bouton Reset Usine (Factory Reset)
- **Côté 1** du bouton : Reliez à **GPIO 23**.
- **Côté 2** du bouton : Reliez à **GND**.

### Bouton Restart (Redémarrage)
- **Côté 1** du bouton : Reliez à **GPIO 27**.
- **Côté 2** du bouton : Reliez à **GND**.

## Étape 5 : Capteur de Courant (ACS712)
*Note : Logiciel désactivé actuellement, mais voici le câblage prévu.*
- **Sortie Signal (OUT)** : Reliez à **GPIO 34**.
- **Alimentation** : VCC vers 5V, GND vers GND.
- **Attention** : Ce capteur se place en **SÉRIE** avec la charge 220V. **DANGER HAUTE TENSION**. Soyez extrêmement prudent.

---
**Vérification Finale** : Avant de brancher, assurez-vous qu'aucun câble 5V ne touche le 3.3V ou le GND.
