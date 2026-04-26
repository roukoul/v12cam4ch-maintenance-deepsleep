# ESP-CAM : Instructions de Flash (MANUEL)

**ATTENTION** : N'exécutez ces commandes que si vous êtes certain d'être dans le bon dossier (`esp-cam-firmware`).

## 1. Préparation
Ouvrez un terminal (PowerShell) à la **racine** du projet.
Activez l'environnement ESP-IDF :
```powershell
. $HOME\esp\v5.5.1\esp-idf\export.ps1
```

## 2. Accès au projet Caméra
Entrez IMPÉRATIVEMENT dans le sous-dossier :
```powershell
cd esp-cam-firmware
```
*Votre invite de commande doit terminer par `\esp-cam-firmware`.*

## 3. Configuration (Si premier flash ou après nettoyage)
```powershell
idf.py set-target esp32
```
(Le fichier `sdkconfig.defaults` appliquera automatiquement la config 4MB nécessaire).

## 4. Compilation (Build)
```powershell
idf.py build
```

## 5. Flash + Monitor
Connectez l'ESP-CAM sur le port **COM6** (ou adaptez la commande).
```powershell
idf.py -p COM6 flash monitor
```

*Si le flash échoue : Maintenez IO0 à GND, appuyez sur RST, relâchez RST, puis enlevez IO0.*

---
**En cas de doute ou d'erreur "Partition dont fit"** :
Relancez `idf.py menuconfig`, allez dans `Serial Flasher Config` et vérifiez que `Flash Size` est bien **4MB**.
