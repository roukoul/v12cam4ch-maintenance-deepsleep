# 🛠️ Boîte à Outils (Tools)

Ce dossier contient les scripts maintenus pour le cycle de vie du projet AepBill.
Les scripts sont préfixés pour identifier leur fonction : `pkg_` (Packaging) ou `sec_` (Sécurité).

---

## 📦 Packaging & Livraison (`pkg_`)

Ces scripts gèrent la création et la restauration des livrables (ZIPs dans `releases/`).

### 1. Créer une Release (Depuis le Code)
*   **Script :** `pkg_create_release.py`
*   **Description :** Groupe le firmware, la doc et les sources en 3 archives standardisées (Client, Tech, Backup).
*   **Commande :**
    ```bash
    python tools/pkg_create_release.py
    ```

### 2. Restaurer une Release (Depuis un Backup)
*   **Script :** `pkg_restore_from_backup.py`
*   **Description :** Recrée les archives Client et Tech à partir d'un fichier ZIP `Full_Project_Backup`. Utile si vous n'avez que la sauvegarde froide.
*   **Commande :**
    ```bash
    python tools/pkg_restore_from_backup.py [chemin_du_backup.zip]
    ```

---

## 🔐 Sécurité & PKI (`sec_`)

Scripts pour la gestion des certificats et de l'intégrité.

### 3. Générer les Certificats HTTPS
*   **Script :** `sec_create_certs.py`
*   **Description :** Crée une Autorité de Certification (CA) locale et signe un certificat Serveur pour l'ESP32.
*   **Sortie :** `main/certs/server_cert.pem`, `main/certs/server_key.pem`.
*   **Usage :** Automatique (via CMake) ou manuel.

### 4. Calculer le Hash SHA256
*   **Script :** `sec_calc_sha256.py`
*   **Description :** Calcule l'empreinte de sécurité d'un binaire firmware. Utile pour vérifier l'intégrité avant OTA.
*   **Commande :**
    ```bash
    python tools/sec_calc_sha256.py chemin/vers/firmware.bin
    ```

---

## 🗑️ Archives (`legacy/`)
Les anciens scripts (`export_release.py`, etc.) qui ne sont plus utilisés dans le workflow v7.3.0+ sont déplacés ici. Ne les utilisez pas pour les nouvelles versions.
