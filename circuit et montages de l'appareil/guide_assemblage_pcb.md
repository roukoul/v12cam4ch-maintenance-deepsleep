# Guide d'Assemblage PCB - AepBill Smart System

Ce guide détaille pas à pas comment souder tous les composants sur votre circuit imprimé.

---

## Matériel Nécessaire

### Outils de Soudure

| Outil | Recommandation | Prix indicatif |
|:------|:---------------|:---------------|
| **Fer à souder** | Station de soudage 60W, température réglable (300-400°C) | 20-50€ |
| **Panne de fer** | Panne fine conique (0.5-1mm) pour précision | Inclus |
| **Fil d'étain** | Sn60/Pb40 ou lead-free (0.6-0.8mm de diamètre) | 5-10€ |
| **Tresse à dessouder** | Pour corriger les erreurs | 3-5€ |
| **Pompe à dessouder** | Alternative à la tresse | 5-10€ |
| **Pince à bec fin** | Pour manipuler les composants | 5-10€ |
| **Pince coupante** | Pour couper les pattes des composants THT | 5-10€ |
| **Multimètre** | Test de continuité et tension | 10-30€ |
| **Loupe/microscope** | Optionnel, pour inspecter les soudures | 10-50€ |
| **Flux** | Facilite la soudure (optionnel, déjà dans l'étain) | 5€ |

> [!TIP]
> Pour débuter, un kit de soudure à 30-50€ contient généralement tout le nécessaire : fer, support, éponge, fil d'étain, tresse.

### Composants à Préparer

Rassemblez tous les composants listés dans [liste_technique.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/liste_technique.md) :

**Modules :**
- [ ] 1× ESP32 DevKit V1 (WROOM-32)
- [ ] 1× Module LCD I2C 1602 ou 2004
- [ ] 1× Module RTC DS3231
- [ ] 1× Module Relais 5V (1 canal)
- [ ] 1× Module ACS712 (optionnel)

**Composants électroniques :**
- [ ] 2× Boutons poussoirs tactiles 6×6mm
- [ ] 1× Buzzer actif 5V (THT)
- [ ] 2× Condensateurs électrolytiques 100µF 16V
- [ ] 1× Condensateur électrolytique 10µF 16V
- [ ] 3× Condensateurs céramiques 100nF 50V
- [ ] 1× Diode 1N4007

**Connecteurs :**
- [ ] 2× Headers femelles 15 pins (pour ESP32)
- [ ] 1× Header femelle 4 pins (pour LCD)
- [ ] 1× Header femelle 4 pins (pour RTC)
- [ ] 1× Header femelle 3 pins (pour Relais)
- [ ] 1× Bornier à vis 2 pôles (alimentation)
- [ ] 1× Bornier à vis 3 pôles (sortie relais)
- [ ] 1× Barrel Jack 5.5/2.1mm (optionnel, alternative au bornier)

---

## Étape 1 : Préparation du PCB

### 1.1 Nettoyage du PCB
1. Retirez le film protecteur (si présent)
2. Nettoyez le PCB avec de l'alcool isopropylique (70-99%) et un chiffon doux
3. Laissez sécher complètement

### 1.2 Vérification Visuelle
- [ ] Pas de fissures ou rayures
- [ ] Tous les pads sont intacts
- [ ] Les trous sont bien percés et métallisés

> [!NOTE]
> La métallisation des trous (PTH) permet de souder des deux côtés. Vérifiez qu'elle est présente en regardant l'intérieur des trous (brillant = métallisé).

---

## Étape 2 : Ordre de Soudure Recommandé

**Principe** : Souder du plus petit au plus grand, du plus bas au plus haut.

```
Ordre de soudure :
1. Composants SMD (si présents) et résistances
2. Diode 1N4007
3. Condensateurs céramiques (100nF)
4. Boutons poussoirs
5. Condensateurs électrolytiques (10µF, 100µF)
6. Headers femelles (connecteurs)
7. Borniers à vis
8. Buzzer
9. (Dernier) Modules enfichables (ESP32, LCD, etc.)
```

---

## Étape 3 : Soudure des Composants Passifs

### 3.1 Diode 1N4007 (Protection Relais)

**Emplacement** : Près du module relais (D1 sur sérigraphie)

1. **Repérez la polarité** :
   - Bague noire sur la diode = **Cathode** (-)
   - Vérifiez le marquage sur le PCB
   
2. **Insérez la diode** :
   - Cathode (bague) vers le côté marqué sur PCB
   - Pliez les pattes à 90° pour maintenir en place
   
3. **Soudez** :
   - Retournez le PCB
   - Maintenez 3-4 secondes avec le fer à 350°C
   - Ajoutez une petite quantité d'étain
   - La soudure doit former un cône brillant
   
4. **Coupez** :
   - Coupez les pattes dépassant à 2mm du PCB

> [!WARNING]
> **Polarité critique** : Une diode inversée ne protège pas le circuit. Vérifiez 2 fois avant de souder !

### 3.2 Condensateurs Céramiques (100nF) ×3

**Emplacements** : C3 (LCD), C4 (RTC), C5 (Relais)

1. **Pas de polarité** : Les condensateurs céramiques ne sont pas polarisés
2. **Insérez et écartez légèrement les pattes** pour les maintenir
3. **Soudez** et **coupez** les pattes

### 3.3 Condensateurs Électrolytiques

**Emplacements** : C1 (100µF ESP32 VIN), C2 (10µF ESP32 3.3V), C6 (100µF Relais)

1. **Repérez la polarité** :
   - Bande blanche/grise sur le condensateur = côté **négatif** (-)
   - Patte longue = **positif** (+)
   - Vérifiez les marquages **+** et **-** sur le PCB
   
2. **Insérez** en respectant la polarité
3. **Soudez** : Température 350°C, 4-5 secondes
4. **Coupez** les pattes

> [!CAUTION]
> Un condensateur électrolytique inversé peut **exploser** ! Vérifiez toujours la polarité.

---

## Étape 4 : Soudure des Boutons et Buzzer

### 4.1 Boutons Poussoirs ×2

**Emplacements** : SW1 (Reset Factory), SW2 (Restart)

1. **Orientation** : Les boutons 6×6mm ont 4 pattes, 2 paires connectées
2. **Alignez** les pattes avec les trous du PCB
3. **Insérez** en appuyant fermement
4. **Soudez** les 4 pattes
5. **Testez** : Appuyez pour vérifier le "clic"

### 4.2 Buzzer BZ1

**Emplacement** : Centre-bas du PCB

1. **Repérez la polarité** :
   - Buzzer actif : marquage **+** sur le dessus ou patte longue = +
   - Vérifiez le PCB : marquage + vers GPIO25
   
2. **Insérez** et **soudez**
3. **Ne coupez pas encore** : facilitera le test

---

## Étape 5 : Soudure des Connecteurs (Headers)

### 5.1 Headers Femelles pour ESP32 (2×15 pins)

**Emplacement** : Zone U1 (centre-droit)

1. **Astuce de maintien** :
   - Placez d'abord les headers femelles sur le PCB
   - Insérez l'ESP32 **dans les headers** (sans le souder)
   - L'ESP32 maintient les headers alignés
   
2. **Soudez** :
   - Soudez d'abord **1 pin de chaque header** (coins opposés)
   - Vérifiez l'alignement
   - Si incliné, réchauffez et réajustez
   - Soudez toutes les autres pins
   
3. **Retirez l'ESP32** (ne le soudez jamais directement !)

### 5.2 Headers Femelles pour Modules (LCD, RTC, Relais)

**Emplacements** : U2 (LCD 4 pins), U3 (RTC 4 pins), U4 (Relais 3 pins)

1. **Même technique** : Insérez le module dans le header pour maintenir l'alignement
2. **Soudez** de biais pour éviter que le fer touche le module
3. **Retirez** les modules

### 5.3 Borniers à Vis

**Emplacements** : J1 (Alimentation 2 pôles), J2 (Relais 3 pôles)

1. **Orientation** : Vis accessibles depuis le bord du PCB
2. **Maintenez bien à plat** : Les borniers sont lourds
3. **Soudez** avec un fer bien chaud (380°C) et plus d'étain (connexion mécanique)

> [!TIP]
> Pour J2 (sortie relais 220V), ajoutez généreusement l'étain pour une connexion robuste supportant le courant.

---

## Étape 6 : Inspection et Tests à Mi-Parcours

### 6.1 Inspection Visuelle

Avec une loupe, vérifiez chaque soudure :

**Bonne soudure** :
- Forme de cône ou volcan brillant
- L'étain relie la patte et le pad
- Pas de pont entre pins adjacentes

**Mauvaise soudure** :
- Boule d'étain mate et granuleuse (soudure froide)
- Étain ne mouille pas le pad (manque de chaleur)
- Pont entre pins (trop d'étain)

> [!NOTE]
> Une **soudure froide** se produit si le fer n'est pas assez chaud ou retiré trop vite. Réchauffez pour corriger.

### 6.2 Test de Continuité

Avec un multimètre en mode "continuité" (symbole son 🔊) :

1. **Test GND** :
   - Placez une sonde sur un pad GND du bornier J1
   - Touchez tous les autres GND (boutons, modules)
   - Doit sonner à chaque fois (résistance ~0Ω)

2. **Test isolation VIN/GND** :
   - Sonde 1 sur VIN (J1 +5V)
   - Sonde 2 sur GND (J1 -)
   - **Ne doit PAS sonner** (résistance infinie)
   - Si ça sonne → court-circuit, cherchez le pont d'étain

> [!CAUTION]
> Un court-circuit VIN/GND détruirait l'alimentation. **Test obligatoire avant mise sous tension !**

---

## Étape 7 : Assemblage Final et Test Fonctionnel

### 7.1 Insertion des Modules

**Ordre d'insertion** :

1. **RTC DS3231** (U3) :
   - Alignez les 4 pins (VCC, GND, SDA, SCL)
   - Vérifiez que la pile CR2032 est en place
   - Enfoncez fermement
   
2. **LCD Module I2C** (U2) :
   - Alignez avec les 4 pins
   - Ajustez le potentiomètre de contraste (vis au dos) une fois alimenté
   
3. **Module Relais** (U4) :
   - Respectez l'ordre VCC, GND, IN
   - Vérifiez le cavalier (jumper) : Position High/Low Level Trigger
   
4. **ESP32 DevKit** (U1) :
   - **ATTENTION** : Ne le flashez **pas encore**
   - USB doit être orienté vers le haut
   - Enfoncez bien les 30 pins

5. **ACS712** (U5 - optionnel) :
   - Si vous utilisez le capteur de courant

### 7.2 Câblage Externe

**Alimentation (J1)** :
- Bornier + (VIN) → Alimentation +5V DC
- Bornier - (GND) → Alimentation masse

**Sortie Relais (J2)** :
- COM : Ligne 220V AC (phase ou neutre)
- NO (Normally Open) : Vers la charge
- NC : Non utilisé (laissez vide)

> [!CAUTION]
> **HAUTE TENSION** : Le relais manipule du 220V AC. Manipulation par personne qualifiée uniquement. Coupez le courant avant tout branchement.

### 7.3 Premier Test (Sans 220V)

**Test à vide (5V seulement)** :

1. **Branchez uniquement l'alimentation 5V** sur J1 (ou USB sur ESP32)
2. **Vérifiez** :
   - [ ] ESP32 LED d'alimentation allumée (rouge généralement)
   - [ ] LCD s'allume (rétroéclairage bleu/vert)
   - [ ] Pas de fumée, odeur de brûlé, surchauffe
   - [ ] Relais **ne clique pas** (normal, pas encore programmé)
   
3. **Testez les tensions** (multimètre en DC Volt) :
   - Entre VIN et GND : **~5V**
   - Entre 3.3V ESP32 et GND : **~3.3V**

> [!WARNING]
> Si fumée ou odeur → **Débranchez immédiatement** ! Vérifiez polarité et court-circuits.

### 7.4 Programmation du Firmware

**Avant de flasher** :

1. **Débranchez** l'alimentation externe du bornier J1
2. **Branchez** uniquement le câble USB de l'ESP32 au PC
3. **Flashez** le firmware selon [guide_rebuild_flash.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_rebuild_flash.md)

Commandes (depuis le répertoire du projet) :

```powershell
# Effacer la flash (première fois)
esptool.py --chip esp32 --port COM3 erase_flash

# Flasher le firmware
idf.py -p COM3 flash monitor
```

Remplacez `COM3` par le port de votre ESP32 (vérifiez dans Gestionnaire de périphériques Windows).

### 7.5 Test Complet Fonctionnel

**Une fois flashé** :

1. **Connectez-vous au réseau Wi-Fi** :
   - SSID : `AepBill_XXXXXX` (mode AP au premier démarrage)
   - Mot de passe : `12345678` (par défaut, vérifiez le code)
   
2. **Accédez à l'interface web** :
   - Navigateur : `http://192.168.4.1` (en mode AP)
   - Ou `http://aepbill.local` (mDNS en mode Station)
   
3. **Vérifiez l'affichage LCD** :
   - Doit afficher l'adresse IP ou état du système
   - Ajustez le contraste si nécessaire (potentiomètre au dos du LCD)
   
4. **Testez les boutons** :
   - **SW1 (GPIO23)** : Appui long (>5s) → Reset d'usine (LCD doit indiquer)
   - **SW2 (GPIO27)** : Appui court → Redémarrage
   
5. **Testez le relais** :
   - Via l'interface web, activez/désactivez le relais
   - Écoutez le "clic" mécanique
   - **Ne branchez pas encore de charge 220V !**
   
6. **Testez le buzzer** :
   - Le firmware doit émettre des bips lors d'événements (alarme, notification)
   - Si trop fort, désoudez et ajoutez une résistance 220Ω en série sur GPIO25

7. **Vérifiez l'heure (RTC)** :
   - Interface web doit afficher l'heure correcte
   - Débranchez, attendez 1min, rebranchez → Heure conservée (pile CR2032 fonctionne)

---

## Étape 8 : Finitions et Boîtier

### 8.1 Nettoyage Post-Soudure

1. **Retirez le flux résiduel** :
   - Alcool isopropylique (99%) + brosse à dents douce
   - Frottez délicatement
   - Laissez sécher

2. **Inspection finale** :
   - Aucune trace d'étain entre pistes
   - Soudures brillantes et solides

### 8.2 Boîtier de Protection

**Options** :

1. **Boîtier plastique sur mesure** :
   - Dimensions : 110×90×40mm (pour PCB 100×80mm + composants)
   - Avec découpes pour USB, boutons, LCD, borniers
   - Impression 3D ou achat chez Aliexpress/Amazon
   
2. **Boîtier industriel** :
   - Boîtier rail DIN si installation électrique (tableau)
   - Norme IP20 minimum (protection contre doigts)
   - Pour 220V : IP44+ recommandé (étanche)

3. **Support de montage** :
   - Utilisez les 4 trous M3 pour fixer sur :
     - Plaque de montage
     - Rail DIN avec adaptateur
     - Fond de boîtier avec entretoises M3×10mm

> [!IMPORTANT]
> **Sécurité 220V** : Si vous utilisez le relais avec du courant secteur, le boîtier **doit** isoler complètement les borniers J2 et J3. Norme IEC 60950 obligatoire pour usage commercial.

---

## Dépannage (Problèmes Courants)

### ESP32 ne démarre pas
- **Cause** : Court-circuit, mauvaise alimentation
- **Solution** :
  - Vérifiez continuité VIN/GND (doit être infinie)
  - Testez avec USB seulement (pas de bornier)
  - Vérifiez condensateurs C1/C2 polarité

### LCD ne s'allume pas
- **Cause** : Adresse I2C incorrecte ou mauvaise connexion
- **Solution** :
  - Vérifiez soudures SDA (GPIO21) et SCL (GPIO22)
  - Scannez I2C avec code de test (adresse 0x27 ou 0x3F)
  - Re-soudez les 4 pins du header LCD

### RTC heure incorrecte
- **Cause** : Pile CR2032 déchargée ou absente
- **Solution** :
  - Insérez/remplacez la pile (3V, vérifiez avec multimètre)
  - Re-flashez pour initialiser l'heure

### Relais ne clique pas
- **Cause** : GPIO16 non connecté ou firmware désactivé
- **Solution** :
  - Vérifiez soudure header pin IN du relais
  - Testez GPIO16 avec multimètre (doit passer 0→3.3V lors de l'activation)
  - Vérifiez le cavalier High/Low Level du module relais

### Boutons ne répondent pas
- **Cause** : Soudure froide ou pull-up désactivé
- **Solution** :
  - Re-soudez les 4 pattes des boutons
  - Vérifiez le firmware : `gpio_set_pull_mode(GPIO_NUM_23, GPIO_PULLUP_ONLY);`

---

## Checklist Finale

Avant de déclarer l'assemblage terminé :

- [ ] Toutes les soudures inspectées et brillantes
- [ ] Aucun court-circuit VIN/GND
- [ ] Tous les modules correctement enfichés
- [ ] Firmware flashé avec succès
- [ ] LCD affiche correctement
- [ ] RTC conserve l'heure (avec pile)
- [ ] Boutons répondent (Reset/Restart)
- [ ] Relais clique à la commande
- [ ] Buzzer émet des sons
- [ ] Interface web accessible via Wi-Fi
- [ ] PCB nettoyé et monté dans un boîtier
- [ ] Documentation sauvegardée
- [ ] Test final complet sans charge 220V OK

---

## Ressources Complémentaires

- [Schéma Circuit](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/schema_circuit.md)
- [Layout PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/pcb_layout.md)
- [Guide Fabrication](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_fabrication_pcb.md)
- [Liste Technique](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/liste_technique.md)
- [Guide Montage Original](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_montage.md)

---

**Félicitations !** 🎉 Votre carte AepBill Smart System est maintenant assemblée et fonctionnelle !
