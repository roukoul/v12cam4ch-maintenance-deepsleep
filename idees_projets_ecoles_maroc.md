# Idées de Projets Innovants pour les Écoles Marocaines (Basées sur l'architecture AepBill / ESP32)

Ce document recense plusieurs concepts d'appareils autonomes et hors-ligne, conçus spécifiquement pour résoudre les problématiques rencontrées par les établissements scolaires marocains, en utilisant des technologies abordables (ESP32, relais, RTC, interface locale).

## 1. Système de Gestion Intelligente de l'Eau (Anti-Gaspillage)
*   **Le problème au Maroc :** Le stress hydrique est un enjeu national. Dans les écoles, les robinets des toilettes restent souvent ouverts (ou fuient) pendant le week-end ou la nuit, gaspillant des tonnes d'eau.
*   **Le concept :** Un boîtier avec une électrovanne (valve motorisée) placée sur l'arrivée d'eau principale ou celle des sanitaires.
*   **Fonctionnement :** En utilisant un Planning Hebdomadaire (RTC), le système coupe automatiquement l'eau à la fin des cours (ex: 18h30) et la rallume le matin (ex: 07h30), et la garde coupée le dimanche. Ajout d'un capteur de débit d'eau : si de l'eau coule en continu pendant plus de 30 minutes (fuite), le système coupe tout et envoie une alerte.

## 2. Contrôleur Autonome d'Éclairage et de Ventilation (Éco-Énergie)
*   **Le problème au Maroc :** Les lumières et les ventilateurs (ou climatiseurs) restent souvent allumés dans les salles de classe vides, gonflant la facture d'électricité.
*   **Le concept :** Un boîtier mural qui remplace les interrupteurs classiques.
*   **Fonctionnement :** Combine un système de relais avec un capteur de présence (PIR). Si la salle est vide pendant plus de 15 minutes, l'appareil coupe automatiquement les relais. Contrôle manuel toujours possible via l'interface locale.

## 3. Moniteur de Qualité d'Air et d'Aération (Santé & Concentration)
*   **Le problème au Maroc :** Fenêtres fermées avec beaucoup d'élèves en classe entraînent une hausse de CO2 (fatigue, baisse de concentration).
*   **Le concept :** Un petit afficheur LED placé dans la classe.
*   **Fonctionnement :** Un ESP32 équipé d'un capteur de CO2 (ex: MH-Z19) mesure la qualité de l'air. Si l'air devient trop saturé, une petite LED passe au rouge (ou un bip retentit) pour indiquer qu'il faut ouvrir les fenêtres. Historique sauvegardé en mémoire (NVS).

## 4. Pointeur RFID Hors-Ligne pour le Staff et les Élèves (Sécurité)
*   **Le problème au Maroc :** Le suivi des présences prend du temps et les systèmes commerciaux biométriques/RFID coûtent cher et nécessitent internet.
*   **Le concept :** Un lecteur de badge RFID connecté à l'interface Web locale.
*   **Fonctionnement :** Les professeurs ou le personnel administratif badgent. L'ESP32 enregistre l'heure exacte (grâce au RTC) dans sa mémoire interne ou carte SD. Le directeur peut télécharger la liste des présences de la semaine sous format de tableau (CSV/Excel) via Wi-Fi local.

## 5. Système d'Irrigation Autonome pour les Espaces Verts Scolaires
*   **Le problème au Maroc :** Entretien difficile des jardins/potagers pédagogiques, évaporation de l'eau en plein soleil.
*   **Le concept :** Un contrôleur de pompe à eau ou d'électrovanne avec capteur d'humidité du sol.
*   **Fonctionnement :** L'arrosage n'est activé que s'il fait nuit ou tôt le matin (RTC) ET si la terre est sèche (capteur). Configurable via une application locale.

---
**Avantage technique :** Ces projets réutilisent la base technique existante (gestion des relais, sauvegarde en mémoire, interface Web, application Android locale, horloge RTC).
