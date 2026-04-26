# Tutoriel Pas à Pas : Créer votre PCB AepBill sur EasyEDA

Ce guide vous explique comment transformer les **spécifications techniques** fournies en fichiers **Gerber** prêts pour la fabrication en utilisant EasyEDA (version en ligne ou bureau).

---

## Étape 1 : Création du Projet
1.  Allez sur [easyeda.com](https://easyeda.com/) et connectez-vous.
2.  Cliquez sur **"Bibliothèque Standard"** (Standard Edition).
3.  Faites **Fichier** > **Nouveau** > **Projet**.
4.  Nom : `AepBill_v12`.

---

## Étape 2 : Le Schéma Électrique (Schematic)
C'est ici que vous dessinez les connexions logiques.
1.  **Recherche des composants** : Utilisez le bouton **"Bibliothèque"** (Library) à gauche.
    - Recherchez `ESP32 DevKit V1 30P`. Placez-le au centre.
    - Recherchez `2N2222` (Transistor TO-92). Placez-en 4.
    - Recherchez `Screw Terminal 5.08mm`. Placez-en pour le 12V et les sorties relais.
    - Recherchez les Modules Relais ou utilisez des **Header 3 pins** pour simuler la connexion vers vos modules.
2.  **Câblage (Wiring)** : Utilisez l'outil **"Fil"** (Wire / touche `W`).
    - Connectez **GPIO 16, 4, 26, 32** chacun à une résistance de 1kΩ, puis à la base d'un transistor Q1-Q4.
    - Connectez l'émetteur de chaque transistor au **GND**.
    - Connectez le collecteur de chaque transistor à la pin **IN** de vos modules relais.
3.  **Alimentation** :
    - Reliez l'entrée 12V au régulateur Buck (ou cherchez un symbole de connecteur).
    - Sortez en 5V du régulateur vers la broche **Vin** de l'ESP32.

---

## Étape 3 : Conversion vers le PCB
1.  Une fois le schéma fini, cliquez sur l'icône **"Convertir le Schéma vers PCB"** (en haut).
2.  EasyEDA va vérifier les erreurs. S'il n'y en a pas, une plaque noire apparaît avec vos composants éparpillés.

---

## Étape 4 : Placement des Composants
1.  Définissez la taille de votre carte (Board OutLine - couche violette).
2.  Placez l'**ESP32** au centre.
3.  Placez les **borniers à vis** (Screw Terminals) sur les bords de la carte pour un accès facile.
4.  Placez les résistances et transistors près de l'ESP32.

---

## Étape 5 : Le Routage Automatique (Auto-Router)
C'est le moment où l'IA travaille à votre place !
1.  Allez dans le menu **Routage** > **Auto-Routeur**.
2.  **Règles de conception (Design Rules)** :
    - *Track Width* (Largeur de piste) : 0.254mm pour les signaux.
    - *Track Width* pour l'alimentation et les relais : **1.0mm minimum** (important !).
3.  Cliquez sur **"Exécuter"** (Run). EasyEDA va tracer toutes les pistes bleues et rouges.

---

## Étape 6 : Personnalisation (Silk Screen)
1.  Sélectionnez l'outil **"Texte"** (T).
2.  Assurez-vous d'être sur la couche **"TopSilkLayer"** (Jaune).
3.  Écrivez : `AepBill par Eddadssi Ahmed`.
4.  Vérifiez que chaque composant est nommé (`R1`, `Q1`, `Relais S`, etc.).

---

## Étape 7 : Génération des fichiers pour le fabricant
1.  Cliquez sur l'icône **"Génération de Fabrication"** (icône G ou Menu Export > Gerber).
2.  EasyEDA vous demandera si vous voulez faire un contrôle de règle de conception (DRC). Cliquez sur **OUI**.
3.  Si tout est vert, cliquez sur **"Générer le fichier Gerber"**.
4.  Cela va télécharger un fichier `.zip` sur votre ordinateur.

**C'est ce fichier .zip (Gerber + Drill) que vous envoyez au fabriquant !**
