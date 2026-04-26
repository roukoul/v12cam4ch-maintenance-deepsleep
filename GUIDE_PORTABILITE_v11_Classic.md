# Guide de Portabilité AepBill v11-Classic

Oui, cette version "Classic" est totalement portable et prête à être utilisée comme nouveau projet pour de futures mises à jour.

Voici pourquoi et comment l'utiliser :

## 1. 📂 Pourquoi elle est portable ?
Pour cette version Classic, j'ai veillé à ce que les éléments suivants soient inclus, ce qui la rend autonome :

*   **Les Fichiers de Configuration (`sdkconfig`) :** C'est le fichier le plus important. Il contient tous vos réglages spécifiques (table de partitions, activation du HTTPS, configuration du processeur, réglages du Wi-Fi par défaut).
*   **La Table de Partitions (`partitions.csv`) :** Indispensable pour que l'ESP32 sache où ranger le code et les données (indispensable pour les mises à jour OTA).
*   **Les Scripts Utilitaires (`tools/` et `ota_upload.py`) :** Le dossier `tools` est inclus pour garantir que la génération du checksum SHA256 (pour l'OTA) fonctionne immédiatement lors d'une nouvelle compilation.
*   **Le Dossier `managed_components` :** Il contient les dépendances réseau (protocoles Espressif) déjà téléchargées, ce qui permet de compiler même sans connexion internet immédiate.

## 2. 🛠️ Comment l'ouvrir comme nouveau projet ?
Si vous souhaitez travailler sur cette version Classic demain ou sur un autre ordinateur :

### Pour le Firmware (ESP32) :
1.  Lancez votre éditeur (**Antigravity** ou **VS Code**).
2.  Faites **Fichier > Ouvrir le dossier** et sélectionnez le dossier (ex: `verssion11 classic final-testée`).
3.  L'extension ESP-IDF reconnaîtra automatiquement le projet grâce au fichier `CMakeLists.txt`.
4.  Vous pourrez compiler avec `idf.py build` sans rien avoir à reconfigurer.

### Pour l'Application Android :
1.  Ouvrez **Android Studio**.
2.  Sélectionnez **Open** et choisissez le dossier `AepBill_Android` situé à l'intérieur du backup.
3.  Gradle synchronisera le projet et vous pourrez modifier l'interface ou le code Kotlin.

---
*Document généré le 19 Décembre 2025*
