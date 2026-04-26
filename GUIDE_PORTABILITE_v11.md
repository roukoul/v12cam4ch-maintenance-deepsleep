# Guide de Portabilité AepBill v11

Oui, ces deux versions sont totalement portables et prêtes à être utilisées comme de nouveaux projets pour de futures mises à jour.

Voici pourquoi et comment les utiliser :

## 1. 📂 Pourquoi elles sont portables ?
Pour chaque version, j'ai veillé à ce que les éléments suivants soient inclus, ce qui les rend autonomes :

*   **Les Fichiers de Configuration (`sdkconfig`) :** C'est le fichier le plus important. Il contient tous vos réglages spécifiques (table de partitions, activation du HTTPS, configuration du processeur, réglages du Wi-Fi par défaut).
*   **La Table de Partitions (`partitions.csv`) :** Indispensable pour que l'ESP32 sache où ranger le code et les données (indispensable pour les mises à jour OTA).
*   **Les Scripts Utilitaires (`tools/` et `ota_upload.py`) :** Le dossier `tools` est inclus pour garantir que la génération du checksum SHA256 (pour l'OTA) fonctionne immédiatement lors d'une nouvelle compilation.
*   **Le Dossier `managed_components` :** Il contient les dépendances réseau (protocoles Espressif) déjà téléchargées, ce qui permet de compiler même sans connexion internet immédiate.

## 2. 🛠️ Comment les ouvrir comme nouveau projet ?
Si vous souhaitez travailler sur l'une de ces versions demain ou sur un autre ordinateur :

### Pour le Firmware (ESP32) :
1.  Lancez votre éditeur (**Antigravity** ou **VS Code**).
2.  Faites **Fichier > Ouvrir le dossier** et sélectionnez le dossier (ex: `AepBill_v11.0_Project_Full_Backup_Final`).
3.  L'extension ESP-IDF reconnaîtra automatiquement le projet grâce au fichier `CMakeLists.txt`.
4.  Vous pourrez compiler avec `idf.py build` sans rien avoir à reconfigurer.

### Pour l'Application Android :
1.  Ouvrez **Android Studio**.
2.  Sélectionnez **Open** et choisissez le dossier `AepBill_Android` situé à l'intérieur du backup.
3.  Gradle synchronisera le projet et vous pourrez modifier l'interface ou le code Kotlin.

---
*Document généré le 19 Décembre 2025*
