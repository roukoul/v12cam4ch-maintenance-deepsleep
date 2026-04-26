# 🌐 PLAN DE MODERNISATION : ACCÈS RÉSEAU v11-Classic
## Objectif: Accès Simplifié via mDNS et Portail Captif (DNS Hijacking)

### 1. ARCHITECTURE TECHNIQUE
Le système utilisera deux nouveaux services tournant en parallèle du serveur Web actuel.

#### A. Service mDNS (Multicast DNS)
- **Bibliothèque:** `mdns.h` (Native ESP-IDF)
- **Hostname:** `aepbill`
- **URL d'accès:** `http://aepbill.local` ou `https://aepbill.local`
- **Avantage:** Plus besoin de retenir l'IP sur PC et iOS.

#### B. Serveur DNS Captif (UDP Port 53)
- **Objectif:** Redirection universelle en mode Access Point (AP).
- **Principe:** Toutes les requêtes DNS entrantes reçoivent l'adresse `192.168.4.1` en réponse.
- **Expérience Utilisateur:** Ouverture automatique du Dashboard dès la connexion au WiFi (Portail Captif).

---

### 2. CODE D'INTÉGRATION (EXTRAITS)

#### Initialisation mDNS (à ajouter dans main.c)
```c
#include "mdns.h"

void start_mdns_service() {
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE("MDNS", "Failed to init: %d", err);
        return;
    }
    mdns_hostname_set("aepbill");
    mdns_instance_name_set("AepBill Smart System");
    
    // Annonce des services
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    mdns_service_add(NULL, "_https", "_tcp", 443, NULL, 0);
    
    ESP_LOGI("MDNS", "Hostname set to: aepbill.local");
}
```

#### Serveur DNS Simplifié (Nouveau composant dns_server.c)
Ce serveur écoute sur le port 53 (UDP) et répond systématique l'IP de l'AP.
```c
// Structure simplifiée pour réponse DNS standard
void dns_server_task(void *pvParameters) {
    // Code de gestion des sockets UDP...
    // Réponse systématique: 192.168.4.1
}
```

---

### 3. SÉCURITÉ ET PRIORITÉ
- Ces services seront démarrés **uniquement après** l'initialisation complète des drivers matériels (Relais, RTC).
- La priorité des tâches réseau sera inférieure à celle du gestionnaire d'alarmes pour garantir la précision de la sonnerie.

### 4. BÉNÉFICE POUR L'APP ANDROID
En utilisant `aepbill.local` comme adresse de base dans l'application Android (via la bibliothèque JmDNS), l'application pourra se connecter au boîtier même si son adresse IP change dynamiquement sur un réseau local complexe.

---
**Document élaboré pour le passage à la version v11.0.**
