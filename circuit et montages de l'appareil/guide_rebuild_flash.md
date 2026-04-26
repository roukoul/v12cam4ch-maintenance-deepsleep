# Guide de Compilation et Flashage

Ce document détaille les étapes pour reconstruire (build) le projet et flasher l'ESP32, ainsi que l'application Android.

## Pré-requis
*   **ESP-IDF v5.x** installé et configuré (Extension VS Code ou ligne de commande).
*   **Android Studio** (pour l'application mobile).
*   **Câble USB** de données (pas seulement de charge).

---

## 1. Firmware ESP32 (AepBill)

### A. Compilation (Build)
Pour compiler le projet sans le flasher :

1.  Ouvrez le terminal dans le dossier racine (`c:\verssion11classicfinaltestee`).
2.  Exécutez la commande :
    ```powershell
    idf.py build
    ```
    *Cela va générer les fichiers binaires dans le dossier `build/`.*

### B. Flashage - Cas 1 : ESP32 Vierge (Sans Encryption)
C'est le cas standard pour un nouveau développement ou un ESP32 neuf.

1.  Connectez l'ESP32 en USB.
2.  Identifiez le port COM (ex: `COM3`).
3.  Lancez le flash et le moniteur série :
    ```powershell
    idf.py -p COM3 flash monitor
    ```
    *(Remplacez COM3 par votre port réel).*

### C. Flashage - Cas 2 : Activer l'Encryption (Nouveau dispositif sécurisé)
Pour protéger le code contre la lecture (dump).
> [!WARNING]
> **IRRÉVERSIBLE** : Une fois activé en mode "Release", l'ESP32 ne pourra plus exécuter de code non signé ni être lu. En mode "Development", il est possible de reflasher (limité).

1.  Ouvrez la configuration :
    ```powershell
    idf.py menuconfig
    ```
2.  Allez dans : **Security features** > **Enable Flash Encryption on boot**.
3.  Choisissez le mode (Development recommandé pour les tests).
4.  Sauvegardez (`S`) et quittez (`Q`).
5.  Compilez et flashez avec la commande **spécifique** :
    ```powershell
    idf.py encrypted-flash monitor
    ```
    *L'ESP32 va crypter sa mémoire au premier démarrage. Cela peut prendre 1 minute.*

### D. Flashage - Cas 3 : ESP32 Déjà Encrypté (Mise à jour)
Si vous avez un ESP32 qui a déjà été encrypté avec ce projet (même clé).

*   **Si mode "Development"** : Utilisez simplement `idf.py encrypted-flash`.
*   **Si mode "Release"** : Le flashage par USB est bloqué. Vous devez passer par **OTA** (Mise à jour à distance via Wi-Fi) ou utiliser une image signée spécifique (procédure complexe nécessitant les clés privées `.pem` générées lors de la première encryption).

---

## 2. Application Android

Le code source se trouve dans le dossier `AepBill_Android`.

1.  Ouvrez **Android Studio**.
2.  Faites **File > Open** et sélectionnez le dossier `AepBill_Android`.
3.  Laissez Gradle synchroniser les dépendances.
4.  Pour installer sur votre téléphone :
    *   Activez le "Débogage USB" sur votre téléphone.
    *   Branchez-le au PC.
    *   Cliquez sur le bouton **Run** (Flèche verte ▶️) dans Android Studio.
5.  Pour générer un APK (fichier d'installation) :
    *   Menu **Build > Build Bundle(s) / APK(s) > Build APK(s)**.
    *   Le fichier `.apk` sera dans `AepBill_Android/app/build/outputs/apk/debug/`.

---

## Résumé des Commandes Clés

| Action | Commande |
| :--- | :--- |
| **Compiler** | `idf.py build` |
| **Nettoyer (si erreur)** | `idf.py fullclean` |
| **Flasher (Standard)** | `idf.py -p COMx flash` |
| **Flasher (Encrypté)** | `idf.py -p COMx encrypted-flash` |
| **Monitorer (Logs)** | `idf.py -p COMx monitor` |
| **Flash + Monitor** | `idf.py -p COMx flash monitor` |
