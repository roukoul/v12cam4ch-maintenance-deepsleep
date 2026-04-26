# Layout PCB - AepBill Smart System

Ce document présente le design du circuit imprimé (PCB) pour le système AepBill, avec le positionnement des composants et les dimensions.

---

## Spécifications du PCB

| Paramètre | Valeur |
|:----------|:-------|
| **Dimensions du PCB** | 100mm × 80mm (recommandé) |
| **Nombre de couches** | 2 couches (Top + Bottom) |
| **Épaisseur du PCB** | 1.6mm (standard) |
| **Épaisseur du cuivre** | 1 oz (35µm) |
| **Matériau** | FR-4 standard |
| **Couleur du masque** | Vert, bleu ou noir (au choix) |
| **Finition** | HASL (Hot Air Solder Leveling) ou ENIG |
| **Largeur des pistes** | 0.5mm (signaux), 1.0mm (alimentation) |
| **Espacement minimum** | 0.3mm |

---

## Vue de Dessus (Top Layer) - Placement des Composants

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           AepBill Smart System PCB                          │
│                                 100mm × 80mm                                 │
│                                                                             │
│  ┌──────────────┐                                                           │
│  │  J1: POWER   │  ○ VIN (+5V)                    ┌─────────────────┐      │
│  │  IN 5V DC    │  ○ GND                          │   U1: ESP32     │      │
│  └──────────────┘                                 │   DevKit V1     │      │
│                                                    │   (30 pins)     │      │
│  ┌──────────────────────────────┐                 │                 │      │
│  │  U2: LCD Module 1602/2004    │                 │  ┌───────────┐  │      │
│  │       (Module I2C)           │                 │  │  ESP32    │  │      │
│  │      Adresse: 0x27           │                 │  │  WROOM-32 │  │      │
│  │                              │                 │  └───────────┘  │      │
│  │  VCC  GND  SDA  SCL          │◄────I2C────────►│                 │      │
│  └──────────────────────────────┘                 │  GPIO Pins →    │      │
│                                                    └─────┬───────────┘      │
│  ┌──────────────────────────────┐                       │                  │
│  │  U3: RTC DS3231              │                       │                  │
│  │      (Module I2C)            │                       │                  │
│  │     Adresse: 0x68            │◄──────I2C─────────────┘                  │
│  │   [Battery CR2032 Socket]    │                                          │
│  │  VCC  GND  SDA  SCL          │                                          │
│  └──────────────────────────────┘                                          │
│                                                                             │
│                                                  ┌────────────────────┐    │
│                                                  │  U4: Relay Module  │    │
│  ┌──────────┐           ┌──────────┐            │     (5V - 1CH)     │    │
│  │ SW1:     │           │ SW2:     │            │                    │    │
│  │ RESET    │           │ RESTART  │            │ VCC GND IN         │    │
│  │ FACTORY  │           │          │            │  │   │  │          │    │
│  │  PUSH    │           │  PUSH    │            │  ○   ○  ◄──GPIO16  │    │
│  │ BUTTON   │           │ BUTTON   │            │                    │    │
│  │ (GPIO23) │           │ (GPIO27) │            │ COM  NO  NC        │    │
│  └─────┬────┘           └─────┬────┘            │  ○   ○   ○         │    │
│        │                      │                 └────┬───────┬───────┘    │
│        ○ GND                  ○ GND                  │       │            │
│                                                      │       │            │
│  ┌──────────────┐                                ┌──▼───────▼──────┐      │
│  │  BZ1: BUZZER │                                │  J2: RELAY OUT  │      │
│  │   (Actif 5V) │                                │  Bornier à vis  │      │
│  │              │                                │  ○ COM          │      │
│  │   +  ◄── GPIO25                               │  ○ NO           │      │
│  │   -  ◄── GND                                  │  ○ NC           │      │
│  └──────────────┘                                └─────────────────┘      │
│                                                                            │
│  ┌──────────────────────────────┐                                         │
│  │  U5: ACS712 (Optionnel)      │                ┌─────────────────┐      │
│  │  Capteur Courant 5A          │                │ J3: AC INPUT    │      │
│  │                              │                │ Bornier à vis   │      │
│  │  VCC GND OUT                 │                │ ○ AC LINE       │      │
│  │   │   │  └──► GPIO34         │                │ ○ AC NEUTRAL    │      │
│  │                              │                └────────┬────────┘      │
│  │  AC IN+  AC OUT-             │                         │               │
│  │    ○       ○                 │                         │               │
│  └────┼───────┼─────────────────┘                         │               │
│       └───────┴───────────────────────────────────────────┘               │
│                                                                            │
│  ┌──────────────────────────────────────────┐                             │
│  │  C1    C2    C3    C4    C5    C6    D1  │  ← Composants SMD/THT       │
│  │ 100µF  10µF 100nF 100nF 100nF 100µF 1N4007│                            │
│  │ (ESP) (ESP) (LCD) (RTC) (REL) (REL)(RELAY)│                            │
│  └──────────────────────────────────────────┘                             │
│                                                                            │
│  [Trous de fixation M3] ○                                        ○        │
│                                                                            │
│                         ○                                        ○        │
│                                                                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Vue de Dessous (Bottom Layer) - Pistes

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            Bottom Layer (Copper)                            │
│                                                                             │
│   ════════════════════════  Plan de Masse (GND Plane)  ══════════════════   │
│                                                                             │
│   [Zone de cuivre remplie connectée à tous les GND]                        │
│                                                                             │
│   Pistes d'alimentation 5V : ════════════════ (1.0mm de large)             │
│   Pistes de signal :         ─────────────── (0.5mm de large)              │
│   Pistes I2C (SDA/SCL) :     ═════════════── (0.6mm de large)              │
│                                                                             │
│   Notes :                                                                   │
│   - Plan de masse sur toute la surface pour réduire le bruit               │
│   - Vias thermiques sous l'ESP32 pour dissipation                          │
│   - Pistes d'alimentation courtes et larges                                │
│   - Éviter les angles à 90° (utiliser 45°)                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Détails du Placement des Composants

### Zone 1 : Alimentation (Haut Gauche)
- **J1 (Connecteur d'alimentation)** : Barrel Jack 5.5/2.1mm ou bornier à vis 2 pôles
  - Dimension : 12mm × 10mm
  - Position : (5mm, 5mm) depuis le coin supérieur gauche
  - **C1** (100µF) : Placé à 3mm de J1 pour filtrage immédiat

### Zone 2 : Microcontrôleur (Centre Droit)
- **U1 (ESP32 DevKit)** : Module sur socket headers femelles
  - Dimension : 50mm × 28mm (30 pins)
  - Position : (45mm, 25mm)
  - Orientation : USB vers le haut
  - **C2** (10µF) et **C3** (100nF) : Sur le dessous, près des pins VIN et 3.3V

### Zone 3 : Affichage et RTC (Gauche)
- **U2 (LCD Module)** : Sur socket female headers 4 pins (VCC, GND, SDA, SCL)
  - Dimension : 80mm × 36mm (pour LCD 1602)
  - Position : (10mm, 20mm)
  - **C4** (100nF) : Près du connecteur LCD
  
- **U3 (RTC DS3231)** : Sur socket female headers 4 pins
  - Dimension : 38mm × 22mm
  - Position : (10mm, 60mm)
  - Socket batterie CR2032 intégré au module
  - **C5** (100nF) : Près du module RTC

### Zone 4 : Boutons (Bas Gauche)
- **SW1 (Reset Factory)** : Bouton tactile 6mm × 6mm
  - Position : (10mm, 100mm)
  - Orientation : vers GPIO23
  
- **SW2 (Restart)** : Bouton tactile 6mm × 6mm
  - Position : (30mm, 100mm)
  - Orientation : vers GPIO27

### Zone 5 : Buzzer (Centre Bas)
- **BZ1 (Buzzer)** : Buzzer actif THT 12mm diamètre
  - Position : (50mm, 100mm)
  - Orientation : + vers GPIO25, - vers GND

### Zone 6 : Relais (Droit)
- **U4 (Module Relais)** : Sur socket female headers 3 pins (VCC, GND, IN)
  - Dimension : 35mm × 20mm
  - Position : (60mm, 60mm)
  - **C6** (100µF) et **D1** (1N4007) : Protection bobine, soudés côté dessous
  
- **J2 (Sortie Relais)** : Bornier à vis 3 pôles (COM, NO, NC)
  - Position : (65mm, 85mm)
  - **Clearance** : 5mm autour pour haute tension

### Zone 7 : Capteur Courant (Optionnel - Bas Droit)
- **U5 (ACS712)** : Sur socket female headers 3 pins (VCC, GND, OUT)
  - Dimension : 30mm × 20mm
  - Position : (10mm, 140mm)
  
- **J3 (Entrée AC)** : Bornier à vis 2 pôles (AC Line, AC Neutral)
  - Position : (60mm, 140mm)
  - **Clearance** : 8mm autour pour haute tension
  - **Note** : Section optionnelle, peut être retirée pour un PCB plus compact

---

## Dimensions Compactes (Version Réduite sans ACS712)

Pour un PCB plus petit **sans le capteur ACS712** :

| Paramètre | Valeur |
|:----------|:-------|
| **Dimensions réduites** | 100mm × 80mm |
| **Zone ACS712** | Supprimée |
| **Connecteur J3** | Supprimé |
| **Gain d'espace** | 20mm en hauteur |

---

## Schéma de Routage (Guidelines)

### 1. Pistes d'Alimentation
```
VIN (+5V) : Largeur 1.0mm minimum
GND       : Plan de cuivre sur Bottom Layer
Courant max : 2A (OK pour 1.0mm à 1oz)
```

### 2. Pistes I2C
```
SDA (GPIO21) : Largeur 0.6mm
SCL (GPIO22) : Largeur 0.6mm
Longueur max : <10cm (OK sur PCB 100mm)
Impédance : Pull-ups internes aux modules (4.7kΩ)
```

### 3. Pistes GPIO
```
GPIO 16, 23, 25, 27, 34 : Largeur 0.5mm
Espacement : 0.3mm minimum entre pistes
Via : Diamètre 0.8mm (trou 0.4mm)
```

### 4. Isolation Haute Tension
```
Zone Relais (COM, NO, NC) : Clearance 5mm minimum
Zone ACS712 AC : Clearance 8mm minimum
Pistes 220V AC : Largeur 1.5mm, isolation renforcée
```

> [!CAUTION]
> **HAUTE TENSION** : Les zones relais et ACS712 manipulent du 220V AC. Respectez les distances d'isolation pour éviter les arcs électriques. Norme IEC 60950 : 3mm minimum pour 250V.

---

## Trous de Fixation

4 trous de fixation M3 (diamètre 3.2mm) aux coins du PCB :
- **Coin 1** : (5mm, 5mm)
- **Coin 2** : (95mm, 5mm)
- **Coin 3** : (5mm, 145mm) ou (5mm, 115mm) pour version compacte
- **Coin 4** : (95mm, 145mm) ou (95mm, 115mm) pour version compacte

Diamètre pad : 6mm (anneau de cuivre autour du trou)

---

## Vias et Connexions Entre Couches

- **Vias thermiques** : Sous l'ESP32 (grille 5mm × 5mm) pour dissipation vers Bottom GND plane
- **Vias de connexion GND** : Tous les GND des composants se connectent au plan GND via vias (∅0.8mm)
- **Via de découplage** : Près de chaque condensateur de découplage

---

## Sérigraphie (Silkscreen)

### Top Silkscreen
- Nom du projet : **"AepBill Smart System v11"**
- Désignateurs : U1, U2, U3, U4, U5, SW1, SW2, BZ1, J1, J2, J3
- Polarité : **+** et **-** pour J1, BZ1
- Avertissement : **⚠ HIGH VOLTAGE** près de J2 et J3
- Instructions boutons : "RESET 5s", "RESTART"

### Bottom Silkscreen
- Version : "PCB Rev 1.0"
- Date : "2025"
- Logo (optionnel)

---

## Checklist de Design PCB

- [x] Tous les composants ont des pads adaptés
- [x] Plan de masse sur Bottom Layer
- [x] Condensateurs de découplage près des ICs
- [x] Diode de protection sur relais
- [x] Clearance haute tension respectée (5mm+)
- [x] Trous de fixation M3 positionnés
- [x] Largeur pistes alimentation ≥ 1.0mm
- [x] Espacement pistes ≥ 0.3mm
- [x] Sérigraphie lisible et informative
- [x] Orientation ESP32 USB accessible
- [x] Boutons accessibles pour appui
- [x] Connecteurs en bordure de PCB

---

## Export pour Fabrication

Pour commander le PCB, exportez les fichiers Gerber suivants :
1. **Top Copper Layer** (.GTL)
2. **Bottom Copper Layer** (.GBL)
3. **Top Silkscreen** (.GTO)
4. **Bottom Silkscreen** (.GBO)
5. **Top Solder Mask** (.GTS)
6. **Bottom Solder Mask** (.GBS)
7. **Drill File** (.TXT ou .DRL)
8. **Board Outline** (.GKO)

Fabricants recommandés : JLCPCB, PCBWay, AllPCB, Eurocircuits (pour l'Europe)

---

## Prochaines Étapes

- [Guide de Fabrication PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_fabrication_pcb.md) : Comment commander le PCB
- [Guide d'Assemblage PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md) : Soudure pas à pas
