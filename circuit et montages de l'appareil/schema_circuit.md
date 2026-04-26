# Schéma Électrique - AepBill Smart System PCB

Ce document présente le schéma électrique complet pour la réalisation du circuit imprimé (PCB) du système AepBill.

## Vue d'ensemble du Circuit

Le circuit est conçu autour d'un ESP32 DevKit V1 avec les périphériques suivants :
- Bus I2C : LCD 1602/2004 + RTC DS3231
- Sorties : Relais 5V + Buzzer
- Entrées : 2 boutons tactiles
- Capteur : ACS712 (optionnel, désactivé par défaut)

---

## Schéma Électrique Principal

```
                                    ┌─────────────────────────────────────┐
                                    │      ESP32 DevKit V1 (WROOM-32)     │
                                    │                                     │
         ┌──────────────────────────┤ VIN                            3V3  ├──────┐
         │                          │ GND                            GND  ├──┐   │
         │                          │ EN                            GPIO0 │  │   │
         │                          │ GPIO36                        GPIO2 │  │   │
         │                          │ GPIO39                        GPIO4 │  │   │
    ACS712 OUT ──────────────────── │ GPIO34 (ADC1_CH6)            GPIO15 │  │   │
         │                          │ GPIO35                       GPIO13 │  │   │
         │                          │ GPIO32                       GPIO12 │  │   │
         │                          │ GPIO33                       GPIO14 │  │   │
    BUZZER + ────────────────────── │ GPIO25                      GPIO27  ├──┼───┤ Bouton RESTART
         │                          │ GPIO26                      GPIO26  │  │   │
    RELAY IN ────────────────────── │ GPIO16                      GPIO25  │  │   │
         │                          │ GPIO17                      GPIO33  │  │   │
         │                          │ GPIO5                       GPIO32  │  │   │
         │                          │ GPIO18                      GPIO35  │  │   │
         │                          │ GPIO19                      GPIO34  │  │   │
         │                          │ GPIO21 (I2C_SDA)            GPIO39  │  │   │
         │                     ┌────┤ GPIO22 (I2C_SCL)            GPIO36  │  │   │
         │                     │    │ GPIO23                      GPIO15  ├──┼───┤ Bouton RESET USINE
         │                     │    │ GND                            GND  ├──┘   │
         │                     │    └─────────────────────────────────────┘      │
         │                     │                                                 │
         │                     │                                                 │
         │        I2C BUS      │                                                 │
         │     ┌───────────────┴────────────────┐                                │
         │     │                                │                                │
         │     │                                │                                │
         │ ┌───▼────────────────┐    ┌──────────▼──────────┐                     │
         │ │   LCD 1602/2004    │    │   RTC DS3231        │                     │
         │ │   (Module I2C)     │    │   (Module I2C)      │                     │
         │ │  Addr: 0x27        │    │   Addr: 0x68        │                     │
         │ ├────────────────────┤    ├─────────────────────┤                     │
         │ │ VCC    │ SDA       │    │ VCC    │ SDA        │                     │
         │ │ GND    │ SCL       │    │ GND    │ SCL        │                     │
         │ └────┬───┴──┬────────┘    └────┬───┴──┬─────────┘                     │
         │      │      │                  │      │                               │
         │      │      │                  │      │                               │
    ┌────┼──────┴──────┴──────────────────┴──────┴───────────────────────────────┘
    │    │              I2C PULL-UPS (4.7kΩ internes sur modules)
    │    │
    │ ┌──▼──────────────────────────────────────────────────────┐
    │ │                 ALIMENTATION 5V DC                      │
    │ │              (Minimum 1A recommandé)                    │
    │ └─────────────────────────────────────────────────────────┘
    │
    │
    │  ┌──────────────────────┐
    │  │  Module Relais 5V    │
    │  │    (1 Canal)         │
    │  ├──────────────────────┤
    │  │ VCC ◄────────────────┼───── +5V
    │  │ GND ◄────────────────┼───── GND
    │  │ IN  ◄──── GPIO 16    │
    │  │                      │
    │  │ COM ◄──── Charge AC  │
    │  │ NO  ────► Charge AC  │
    │  └──────────────────────┘
    │
    │  ┌──────────────────────┐
    │  │   Buzzer Actif 5V    │
    │  ├──────────────────────┤
    │  │ + ◄──── GPIO 25      │
    │  │ - ◄──── GND          │
    │  └──────────────────────┘
    │
    │  ┌──────────────────────┐
    │  │  Capteur ACS712      │
    │  │  (5A - optionnel)    │
    │  ├──────────────────────┤
    └──┤ VCC ◄──── +5V        │
       │ GND ◄──── GND        │
       │ OUT ────► GPIO 34    │
       │                      │
       │ IP+ ◄──── Ligne AC   │
       │ IP- ────► Charge AC  │
       └──────────────────────┘

    ┌──────────────────────┐                 ┌──────────────────────┐
    │ Bouton RESET USINE   │                 │  Bouton RESTART      │
    ├──────────────────────┤                 ├──────────────────────┤
    │ Pin 1 ─── GPIO 23    │                 │ Pin 1 ─── GPIO 27    │
    │ Pin 2 ─── GND        │                 │ Pin 2 ─── GND        │
    └──────────────────────┘                 └──────────────────────┘
           (Pull-up interne)                        (Pull-up interne)
```

---

## Détails des Connexions par Sous-Système

### 1. Bus I2C (LCD + RTC)

```
ESP32 GPIO21 (SDA) ──┬─── LCD SDA
                     └─── RTC SDA

ESP32 GPIO22 (SCL) ──┬─── LCD SCL
                     └─── RTC SCL

+5V ─────────────────┬─── LCD VCC
                     └─── RTC VCC

GND ─────────────────┬─── LCD GND
                     └─── RTC GND
```

> [!NOTE]
> Les modules I2C LCD et RTC possèdent leurs propres résistances de pull-up (généralement 4.7kΩ). Pas besoin d'ajouter de pull-ups externes.

### 2. Sorties Numériques

#### Relais
```
ESP32 GPIO16 ────► Module Relais IN
+5V ─────────────► Module Relais VCC
GND ─────────────► Module Relais GND
```

#### Buzzer
```
ESP32 GPIO25 ────► Buzzer (+)
GND ─────────────► Buzzer (-)
```

> [!TIP]
> Pour un buzzer trop fort, ajoutez une résistance série de 100-220Ω sur GPIO25.

### 3. Entrées (Boutons)

```
GPIO23 ──┬── Bouton Reset Usine ── GND
         └── Pull-up interne (activé logiciellement)

GPIO27 ──┬── Bouton Restart ── GND
         └── Pull-up interne (activé logiciellement)
```

> [!IMPORTANT]
> Les boutons utilisent les pull-ups **internes** de l'ESP32 configurés dans le firmware. Aucune résistance externe n'est nécessaire.

### 4. Capteur de Courant (Optionnel)

```
ESP32 GPIO34 (ADC) ◄─── ACS712 OUT (Signal analogique)
+5V ───────────────────► ACS712 VCC
GND ───────────────────► ACS712 GND

LIGNE AC ──► ACS712 IP+
CHARGE AC ◄─ ACS712 IP-
```

> [!CAUTION]
> **HAUTE TENSION** : Le capteur ACS712 est branché en **série** avec la ligne 220V AC. Manipulation réservée aux personnes qualifiées. Isolation galvanique obligatoire.

---

## Alimentation du Système

### Spécifications
- **Tension** : 5V DC régulé
- **Courant minimum** : 1A (recommandé 2A pour le relais et les périphériques)
- **Connecteur** : Barrel Jack 5.5mm/2.1mm ou USB Micro/Type-C

### Distribution d'Alimentation

```
Alimentation 5V ──┬──► ESP32 VIN
                  ├──► LCD VCC
                  ├──► RTC VCC
                  ├──► Relais VCC
                  ├──► ACS712 VCC (si utilisé)
                  └──► Buzzer VCC (via GPIO25)

Masse Commune ────┬──► ESP32 GND
                  ├──► LCD GND
                  ├──► RTC GND
                  ├──► Relais GND
                  ├──► ACS712 GND
                  ├──► Buzzer GND
                  ├──► Bouton Reset GND
                  └──► Bouton Restart GND
```

> [!WARNING]
> Tous les GND doivent être reliés ensemble pour former une **masse commune unique**. Un défaut de masse peut causer des dysfonctionnements.

---

## Condensateurs de Découplage (Recommandés)

Pour une alimentation stable et réduire les interférences :

| Emplacement | Valeur | Type |
|:------------|:------:|:-----|
| ESP32 VIN | 100µF | Électrolytique |
| ESP32 3.3V | 10µF + 100nF | Céramique |
| LCD VCC | 100nF | Céramique |
| RTC VCC | 100nF | Céramique |
| Relais VCC | 100µF | Électrolytique |

Placement : Au plus près des broches VCC/GND de chaque composant.

---

## Protection

### Diode de Roue Libre (Relais)
```
        Relais Bobine
          │      │
          │      ├─── Diode 1N4007 (Cathode)
          │      │
          └──────┴─── (Anode vers GND)
```

> [!IMPORTANT]
> Une diode 1N4007 en parallèle inverse sur la bobine du relais protège l'ESP32 contre les surtensions lors de la commutation.

### Protection ESD (Optionnel)
Pour un usage industriel, ajouter des diodes TVS sur les lignes I2C et les entrées/sorties exposées.

---

## Liste des Composants Passifs Additionnels

| Composant | Quantité | Valeur | Notes |
|:----------|:--------:|:------:|:------|
| Condensateur électrolytique | 2 | 100µF 16V | ESP32 VIN, Relais |
| Condensateur céramique | 5 | 100nF 50V | Découplage |
| Condensateur céramique | 1 | 10µF 16V | ESP32 3.3V |
| Diode de protection | 1 | 1N4007 | Relais |
| Connecteur barrel jack | 1 | 5.5/2.1mm | Alimentation (optionnel) |
| Borniers à vis | 3-4 | 2-3 bornes | Connexions externes |

---

## Notes de Conception

1. **Compatibilité** : Ce schéma est compatible avec tout ESP32 DevKit V1 standard (30 pins).
2. **Modularité** : Tous les modules utilisent des connecteurs (headers), permettant le remplacement facile.
3. **Extensibilité** : GPIO disponibles pour extensions futures : 0, 2, 4, 5, 12, 13, 14, 15, 17, 18, 19, 26, 32, 33, 35, 36, 39.
4. **Sécurité** : Isolation galvanique recommandée entre la partie basse tension (5V DC) et haute tension (relais 220V AC).

---

## Prochaines Étapes

Consultez les documents suivants pour continuer :
- [Layout PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/pcb_layout.md) : Positionnement des composants sur le PCB
- [Guide de Fabrication](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_fabrication_pcb.md) : Instructions pour commander le PCB
- [Guide d'Assemblage PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md) : Soudure et montage des composants
