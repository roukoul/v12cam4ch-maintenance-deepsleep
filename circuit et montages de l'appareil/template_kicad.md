# Template KiCad - AepBill Smart System

Ce document vous guide pour créer le PCB dans **KiCad** (logiciel gratuit de conception PCB).

---

## Installation de KiCad

1. **Téléchargez KiCad** : [https://www.kicad.org/download/](https://www.kicad.org/download/)
2. **Version recommandée** : KiCad 7.0 ou supérieur
3. **Installation** : Suivez l'installateur (Windows/Mac/Linux)

---

## Étape 1 : Créer un Nouveau Projet

1. Lancez **KiCad**
2. **Fichier → Nouveau Projet**
3. Nom : `AepBill_PCB`
4. Emplacement : Sauvegardez dans un dossier dédié

---

## Étape 2 : Créer le Schéma (Schematic Editor)

### 2.1 Ouvrir l'Éditeur de Schéma

1. Double-cliquez sur le fichier `.kicad_sch` dans KiCad
2. Vous êtes maintenant dans **Schematic Editor**

### 2.2 Ajouter les Symboles

Appuyez sur **A** (Add Symbol) et cherchez les composants suivants :

#### ESP32 DevKit
```
Symbole : MCU_Module:ESP32-DEVKITV1
Référence : U1
Pins utilisés : GPIO16, 21, 22, 23, 25, 27, 34, VIN, GND, 3V3
```

> [!NOTE]
> Si le symbole ESP32-DEVKITV1 n'existe pas, créez un symbole personnalisé avec 30 pins selon le datasheet.

#### Module LCD I2C
```
Symbole : Display_Character:LCD-16x2 (ou créer symbole avec 4 pins)
Référence : U2
Pins : VCC, GND, SDA, SCL
Adresse I2C : 0x27 (annoté dans propriétés)
```

#### RTC DS3231
```
Symbole : Timer_RTC:DS3231 (ou créer symbole avec 4 pins)
Référence : U3
Pins : VCC, GND, SDA, SCL
Adresse I2C : 0x68
```

#### Module Relais
```
Symbole : Relay:RELAY_1P1T (ou créer symbole avec 3 pins)
Référence : K1 ou U4
Commande : VCC, GND, IN
Contacts : COM, NO, NC
```

#### Composants Passifs
```
Condensateurs électrolytiques :
- C1 : 100µF 16V (Device:CP)
- C2 : 10µF 16V (Device:CP)
- C6 : 100µF 16V (Device:CP)

Condensateurs céramiques :
- C3, C4, C5 : 100nF 50V (Device:C)

Diode :
- D1 : 1N4007 (Device:D)

Buzzer :
- BZ1 : Buzzer_Beeper (Device:Buzzer)

Boutons :
- SW1, SW2 : SW_Push (Switch:SW_Push)
```

### 2.3 Connecter les Composants

Utilisez **W** (Wire) pour connecter selon le schéma électrique :

#### Bus I2C
```
ESP32 GPIO21 (SDA) ──┬─ LCD SDA
                     └─ RTC SDA

ESP32 GPIO22 (SCL) ──┬─ LCD SCL
                     └─ RTC SCL
```

#### Alimentation
```
Power Input (+5V) ──┬─ ESP32 VIN
                    ├─ LCD VCC
                    ├─ RTC VCC
                    └─ Relais VCC

GND ───────────────┬─ Tous les GND des modules
                   └─ Plan de masse
```

#### GPIO
```
ESP32 GPIO16 ──► Relais IN
ESP32 GPIO23 ──┬─ SW1 (Reset) ── GND
               └─ Pull-up interne (annoté)
ESP32 GPIO25 ──► Buzzer + ── GND (-)
ESP32 GPIO27 ──┬─ SW2 (Restart) ── GND
               └─ Pull-up interne
ESP32 GPIO34 ──► ACS712 OUT (optionnel)
```

### 2.4 Ajouter les Labels et Annotations

- **Appuyez sur L** pour ajouter des labels aux nets (ex: `SDA`, `SCL`, `+5V`, `GND`)
- Ajoutez des **notes de texte** pour les valeurs importantes
- Annotez les composants : **Outils → Annoter le Schéma → Annoter** (assigne C1, C2, U1, etc.)

### 2.5 Vérifier les Erreurs

1. **Outils → Contrôle électrique des règles (ERC)**
2. Corrigez les erreurs (fils non connectés, pins manquants)
3. Les avertissements de "power pins not driven" pour les modules peuvent être ignorés

---

## Étape 3 : Créer le Layout PCB (PCB Editor)

### 3.1 Générer le Netlist

1. Dans Schematic Editor : **Outils → Générer le Netlist**
2. Sauvegardez le fichier .net

### 3.2 Ouvrir PCB Editor

1. Double-cliquez sur le fichier `.kicad_pcb` dans KiCad
2. **Outils → Charger le Netlist** → Sélectionnez votre .net
3. Cliquez **Update PCB** → Tous les composants apparaissent

### 3.3 Définir les Dimensions du PCB

1. Sélectionnez la couche **Edge.Cuts** (contour du PCB)
2. Dessinez un rectangle :
   - **Largeur** : 100mm
   - **Hauteur** : 80mm (ou 120mm avec ACS712)
3. Utilisez **Lignes** ou **Rectangle** de l'outil de dessin

### 3.4 Placer les Composants

Organisez les composants selon [pcb_layout.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/pcb_layout.md) :

#### Positions Recommandées (origine = coin supérieur gauche)

```
U1 (ESP32)    : (45mm, 25mm)   - Centre-droit, USB vers le haut
U2 (LCD)      : (10mm, 20mm)   - Haut gauche
U3 (RTC)      : (10mm, 60mm)   - Sous le LCD
U4 (Relais)   : (60mm, 60mm)   - Droite
SW1 (Reset)   : (10mm, 100mm)  - Bas gauche
SW2 (Restart) : (30mm, 100mm)  - Bas gauche
BZ1 (Buzzer)  : (50mm, 100mm)  - Centre bas
J1 (Power)    : (5mm, 5mm)     - Haut gauche
J2 (Relay Out): (65mm, 85mm)   - Près du relais
```

**Astuce** : Utilisez **M** (Move) et tapez la position exacte (X, Y) dans la fenêtre de propriétés.

### 3.5 Ajouter les Trous de Fixation

1. **Placer → Pastille** (ou **Add Footprint** → `MountingHole:MountingHole_3.2mm_M3`)
2. Positions (coordonnées) :
   - Coin 1 : (5mm, 5mm)
   - Coin 2 : (95mm, 5mm)
   - Coin 3 : (5mm, 115mm)
   - Coin 4 : (95mm, 115mm)

### 3.6 Routage des Pistes

#### 3.6.1 Plan de Masse (GND - Bottom Layer)

1. **Couche → Bottom (B.Cu)**
2. **Ajouter une zone de cuivre** : **Add filled zone** (icône ou touche **Ctrl+Shift+Z**)
3. Sélectionnez le net **GND**
4. Clearance : **0.3mm**
5. Remplissez toute la surface du PCB
6. **B** (Rebuild zones) pour remplir

#### 3.6.2 Pistes d'Alimentation (+5V)

1. **Couche → Top (F.Cu)**
2. **X** (Add Track) et routez les connexions **+5V** :
   - Largeur : **1.0mm** (modifier dans toolbar ou toucher **W**)
   - Connectez J1 (+5V) vers ESP32 VIN, LCD VCC, RTC VCC, Relais VCC

#### 3.6.3 Pistes I2C (SDA, SCL)

1. **Couche → Top**
2. Largeur des pistes : **0.6mm**
3. Routez :
   - ESP32 GPIO21 → LCD SDA → RTC SDA
   - ESP32 GPIO22 → LCD SCL → RTC SCL
4. Gardez les pistes parallèles et courtes (<10cm)

#### 3.6.4 Pistes GPIO (Relais, Buzzer, Boutons)

1. Largeur : **0.5mm** (signaux digitaux)
2. Routez :
   - GPIO16 → Relais IN
   - GPIO23 → SW1
   - GPIO25 → BZ1
   - GPIO27 → SW2
   - GPIO34 → ACS712 (optionnel)

#### 3.6.5 Vias

Pour connecter Top à Bottom :
- **Appuyez sur V** pendant le routage pour ajouter un via
- Diamètre via : **0.8mm** (trou 0.4mm)

### 3.7 Clearance Haute Tension

**Zone relais (J2 - COM/NO/NC)** :
1. Ajoutez une zone **Keepout** autour de J2
2. Clearance : **5mm minimum** de toute piste basse tension
3. **Edit → Board Setup → Design Rules → Constraints** : Clearance = 0.3mm (général), 5mm pour zone HT

### 3.8 Ajouter la Sérigraphie (Silkscreen)

1. **Couche → F.SilkS** (sérigraphie top)
2. **Ajouter du texte** (bouton texte ou **Ctrl+Shift+T**) :
   - Titre : `AepBill Smart System v11`
   - Taille police : 1.5mm
   - Position : En haut ou bas du PCB
3. Annotations :
   - **+** et **-** pour J1 (alimentation)
   - **⚠ HIGH VOLTAGE** près de J2
   - Désignateurs : U1, U2, SW1, SW2, etc. (automatique si activé)

### 3.9 Vérifier les Règles de Conception (DRC)

1. **Inspect → Design Rule Checker (DRC)**
2. Lancez le contrôle
3. Corrigez les erreurs :
   - Pistes trop proches (espacement <0.3mm)
   - Pistes déconnectées
   - Zones non remplies

---

## Étape 4 : Générer les Fichiers Gerber

### 4.1 Configuration du Traceur

1. **Fichier → Tracer**
2. **Format** : Gerber
3. **Répertoire de sortie** : Créez un dossier `Gerbers`
4. **Couches à inclure** (cochez toutes) :
   - F.Cu (Top Copper)
   - B.Cu (Bottom Copper)
   - F.SilkS (Top Silkscreen)
   - B.SilkS (Bottom Silkscreen)
   - F.Mask (Top Solder Mask)
   - B.Mask (Bottom Solder Mask)
   - Edge.Cuts (Board Outline)

5. **Options** :
   - ☑ Utiliser les noms étendus Protel
   - ☑ Soustraire le masque de la sérigraphie
   - ☑ Format coordonnées : 4.6, mm

6. Cliquez **Tracer**

### 4.2 Générer le Fichier de Perçage

1. Dans la même fenêtre, cliquez **Générer les fichiers de perçage**
2. **Format** : Excellon
3. **Unités** : Millimètres
4. Cliquez **Générer le fichier de perçage**

### 4.3 Compresser en ZIP

1. Allez dans le dossier `Gerbers`
2. Sélectionnez tous les fichiers (`.gbr`, `.drl`, `.gbrjob`)
3. Clic droit → **Envoyer vers → Dossier compressé (ZIP)**
4. Nom : `AepBill_PCB_Gerbers_v1.zip`

---

## Étape 5 : Vérification avec Gerber Viewer

Avant de commander, **vérifiez toujours** :

1. Accédez à [https://www.pcbway.com/project/OnlineGerberViewer.html](https://www.pcbway.com/project/OnlineGerberViewer.html)
2. Uploadez `AepBill_PCB_Gerbers_v1.zip`
3. Inspectez chaque couche :
   - ✅ Toutes les pistes bien connectées
   - ✅ Plan de masse uniforme sur Bottom
   - ✅ Sérigraphie lisible
   - ✅ Trous de fixation positionnés
   - ✅ Dimensions 100×80mm

> [!TIP]
> KiCad inclut aussi un **3D Viewer** : Menu **View → 3D Viewer** pour visualiser le PCB en 3D avant production.

---

## Ressources Supplémentaires

### Tutoriels KiCad

- **Vidéo (FR)** : [Créer un PCB avec KiCad - Tutorial complet](https://www.youtube.com/results?search_query=kicad+tutoriel+français)
- **Documentation officielle** : [https://docs.kicad.org/](https://docs.kicad.org/)
- **Forums KiCad** : [https://forum.kicad.info/](https://forum.kicad.info/)

### Librairies de Composants

Si un composant manque, téléchargez des librairies :
- **SnapEDA** : [https://www.snapeda.com/](https://www.snapeda.com/)
- **Component Search Engine** : [https://componentsearchengine.com/](https://componentsearchengine.com/)
- **Recherche** : "ESP32 DevKit KiCad footprint"

### Alternative : EasyEDA (Plus Simple)

Si KiCad est trop complexe :

1. **EasyEDA** (en ligne, gratuit) : [https://easyeda.com/](https://easyeda.com/)
2. Interface plus intuitive pour débutants
3. Librairie de composants massive (LCSC)
4. Commande directe via JLCPCB intégrée
5. Exportation Gerber automatique

---

## Checklist de Conception KiCad

Avant d'exporter les Gerbers :

- [ ] Tous les composants annotés (U1, U2, C1, etc.)
- [ ] Aucune erreur ERC (Schematic)
- [ ] Aucune erreur DRC (PCB)
- [ ] Plan de masse GND sur Bottom Layer
- [ ] Pistes alimentation ≥ 1.0mm
- [ ] Pistes signaux ≥ 0.5mm
- [ ] Clearance HT (relais) ≥ 5mm
- [ ] Condensateurs de découplage placés près des ICs
- [ ] Diode D1 sur relais (protection)
- [ ] Trous de fixation M3 présents
- [ ] Sérigraphie lisible et informative
- [ ] Dimensions PCB correctes (100×80mm)
- [ ] Fichiers Gerber + Drill générés
- [ ] ZIP créé et vérifié en ligne

---

## Assistance

Pour toute question sur KiCad :
- **Forums officiels** : [forum.kicad.info](https://forum.kicad.info/)
- **Reddit** : [r/KiCad](https://www.reddit.com/r/KiCad/)
- **Discord** : KiCad Community Server

---

**Bon design avec KiCad !** 🎨⚡
