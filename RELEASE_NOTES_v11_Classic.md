# 📋 Fiche de Release : AepBill v11-Classic (Édition Épurée)

**Date :** 19 Décembre 2025  
**Version :** v11-Classic  
**Statut :** Stable (Variante sans capteur de courant)  

---

## 🎯 Objectif de la Version Classic
Cette version est une variante "Allégée" de la v11.0. Elle conserve 100% de la puissance de programmation des alarmes et de la connectivité, mais retire toute la complexité liée à la surveillance du courant électrique (Capteur ADC, Seuils, Anomalies, Buzzer). 

---

## 🚀 Fonctionnalités Conservées
*   **Gestion des Alarmes :** Programmation complète des 20 créneaux par jour sur 7 jours.
*   **Contrôle du Relais :** Pilotage précis On/Off via le planificateur.
*   **Connectivité Premium :** mDNS (`aepbill.local`), Scan WiFi intelligent, HTTPS sécurisé.
*   **Mise à l'heure :** Synchronisation NTP et RTC (DS3231).
*   **Interface Expert :** Design moderne Cairo, transitions fluides et mode responsive.

---

## 🛠️ Modélisation "Classic" (Ce qui a été retiré)
1.  **Surveillance Électrique :** Suppression de l'affichage de l'intensité (Ampères) sur le Web et Android.
2.  **Système d'Anomalies :** Désactivation de la détection de surcharge, fuite, et sous-charge.
3.  **Alarmes Sonores :** Retrait des bips d'alerte liés aux anomalies (Buzzer désactivé pour les erreurs).
4.  **Interface Simplifiée :** Masquage des réglages de seuils (Scale Factor, Min Load, etc.) dans les paramètres.

---

## 📂 Instructions d'Installation
1.  **Firmware :** Compiler et flasher normalement. La signature "Classic" apparaîtra dans les logs.
2.  **Android :** L'interface affiche désormais un tableau de bord focalisé uniquement sur l'état du relais et le planning.

---
**Version adaptée avec Expertise.**  
*Signature : _ Antigravity AI _*
