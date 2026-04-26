# Guide de Connexion à l'Interface

Ce document explique comment se connecter à l'interface de gestion de votre **AepBill Smart System** dans différentes situations.

## Situation 1 : Première Mise en Route (Mode AP)
Lors de la première utilisation (ou si le Wi-Fi configuré est indisponible), l'appareil crée son propre réseau.

1.  **Recherchez le réseau Wi-Fi** :
    *   Nom (SSID) : **`AepBill_Config`**
2.  **Connectez-vous** :
    *   Mot de passe : **`12345678`**
3.  **Accédez à l'interface** :
    *   Ouvrez un navigateur web.
    *   Allez à l'adresse : **`http://192.168.4.1`**
    *   (Sur mobile, une page de connexion peut s'ouvrir automatiquement).

> **Note** : Depuis cette page, vous pourrez configurer votre Wi-Fi domestique pour passer au mode normal.

---

## Situation 2 : Utilisation Normale (Mode Station)
Une fois connecté à votre Wi-Fi domestique ("Station Mode").

1.  **Via le nom d'hôte (mDNS)** :
    *   Dans votre navigateur, tapez : **`http://aepbill.local`**
    *   *Fonctionne sur Apple, Linux, et Windows avec Bonjour/iTunes. Sur Android, le support dépend de la version.*

2.  **Via l'adresse IP** :
    *   Regardez l'écran LCD de l'appareil au démarrage (l'IP s'affiche pendant 10 secondes).
    *   Tapez cette IP dans votre navigateur (ex: `http://192.168.1.15`).

---

## Situation 3 : Via l'Application Android
Si vous utilisez l'application dédiée (code source `AepBill_Android`).

1.  Assurez-vous que votre téléphone est sur le **même réseau Wi-Fi** que l'appareil.
2.  Ouvrez l'application **AepBill Monitoring**.
3.  Allez dans **Paramètres** (Settings).
4.  Dans le champ "Adresse IP / Hôte", entrez :
    *   Soit l'IP affichée sur l'écran LCD (ex: `192.168.1.15`).
    *   Soit le nom mDNS : **`aepbill.local`**
    *   Soit l'IP par défaut du mode AP si vous êtes connecté en direct : **`192.168.4.1`**
5.  Sauvegardez. Le Dashboard devrait afficher les données en temps réel.

---

## Dépannage Rapide

*   **Impossible de se connecter au Wi-Fi "AepBill_Config" ?**
    *   Vérifiez que vous êtes assez proche de l'appareil.
    *   Si l'appareil a déjà été configuré, il essaie peut-être de se connecter à votre box. Faites un **Factory Reset** (Bouton GPIO 23 > 5s) pour forcer le mode AP.

*   **"Site inaccessible" avec `aepbill.local` ?**
    *   Essayez l'adresse IP directe (affichée sur l'écran LCD).
    *   Sur certains Android, le mDNS n'est pas supporté nativement dans le navigateur Chrome => Utilisez l'Application ou l'IP.

*   **Mot de passe oublié ?**
    *   Wi-Fi AP par défaut : `12345678`
    *   Interface Web : Pas de mot de passe par défaut dans cette version (accès ouvert sur le réseau local).
