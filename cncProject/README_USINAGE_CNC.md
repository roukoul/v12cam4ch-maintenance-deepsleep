# Guide d'Atelier & Usinage CNC - Boîtier AepBill

Ce dossier `cncProject/` contient tous les fichiers nécessaires pour programmer votre machine CNC et fabriquer le boîtier de l'appareil **AepBill** (Contrôleur de cloche avec ESP32, Écran LCD 1602 I2C, Relais, Horloge RTC, Buzzer et Bouton poussoir).

---

## 1. Inventaire des Fichiers dans `cncProject/`

1. **`AepBill_FacePlate.svg`** : 
   - Fichier vectoriel 2D à l'échelle 1:1 (140 mm × 100 mm).
   - Prêt pour import direct dans **Easel**, **Carbide Create**, **Estlcam**, **Vectric Aspire**, etc.
   - Contient :
     - Le contour de découpe extérieure (140 × 100 mm).
     - Le logo vectoriel **"AepBill"** et le sous-titre "BELL CONTROLLER".
     - La fenêtre rectangulaire pour l'écran LCD 16x2 (71.5 × 24.5 mm) et ses 4 trous de fixation M3.
     - Le perçage pour le bouton poussoir manuel (Ø 12.2 mm).
     - Le perçage pour le bouton Reset (Ø 4.0 mm).
     - La rosace acoustique (5 micro-trous) pour le buzzer.
     - Les 4 trous de fixation M3 du couvercle.

2. **`AepBill_Case_Model.scad`** :
   - Modèle 3D volumique paramétrique complet pour [OpenSCAD](https://openscad.org/).
   - Modélise la cuve avec :
     - La poche principale profonde de 28 mm pour loger ESP32, Relais, RTC et Buzzer.
     - **L'orifice et l'encoche de sortie pour les 2 fils de la cloche (flanc droit)**.
     - L'entrée d'alimentation 5V (flanc gauche).

3. **`sample_gravure_AepBill.gcode`** :
   - Programme G-code prêt à charger dans votre contrôleur (Candle, UGS, Mach3) pour tester la gravure du logo sur une chute.

---

## 2. Sortie des 2 Fils pour la Cloche Extérieure

Les 2 fils qui partent vers la cloche extérieure sont branchés sur le bornier du **relais interne** :
- **Borne COM (Commun)** du relais.
- **Borne NO (Normalement Ouvert)** du relais.

### Comment usiner le passage des fils sur la cuve :
- **Méthode A (Encoche en U par le dessus) :** Directement fraisée lors de l'usinage 3 axes de la cuve (largeur 7 mm, profondeur 8 mm). Les fils s'y glissent avec un joint et sont plaqués lors de la fermeture du couvercle.
- **Méthode B (Trou rond Ø 8.5 mm) :** Pour installer un presse-étoupe étanche type **PG7** ou un passe-fil caoutchouc anti-arrachement sur le flanc droit du boîtier.

---

## 3. Schéma de Câblage du Relais et de la Cloche

```
  [ ESP32 ] ---> Signal Déclenchement (GPIO) ---> [ Module Relais 5V ]
                                                         |     |
                                                        COM   NO
                                                         |     |
                                              [ SORTIE DU BOITIER (Trou 8.5mm) ]
                                                         |     |
                                                         v     v
                     [ Alimentation Cloche ~230V ou 12V ] ===> [ CLOCHE EXTERNE ]
```

---

## 4. Tableau des Vitesses & Outils Conseillés

| Opération | Outil | Vitesse Broche (RPM) | Vitesse Avance (XY) | Vitesse Plongée (Z) | Profondeur par passe |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Gravure Logo AepBill** | Fraise en V 60° (V-bit) | 12 000 - 15 000 | 700 - 900 mm/min | 250 mm/min | 0.6 mm (Passe unique) |
| **Perçages (LCD, Buzzer, Vis)** | Fraise 1 dent 3.175 mm | 10 000 - 12 000 | 500 mm/min | 150 mm/min | 1.0 mm (Hélicoïdal) |
| **Fenêtre LCD & Bouton 12mm** | Fraise 1 dent 3.175 mm | 12 000 | 800 - 1 000 mm/min | 250 mm/min | 1.2 mm |
| **Poche Cuve (Évidement)** | Fraise plate 2 dents 4 ou 6 mm | 12 000 | 1 000 - 1 400 mm/min | 300 mm/min | 1.5 - 2.0 mm |
| **Détourage extérieur** | Fraise 1 dent 3.175 mm | 12 000 | 800 - 1 100 mm/min | 250 mm/min | 1.2 mm (avec pontets de maintien) |
