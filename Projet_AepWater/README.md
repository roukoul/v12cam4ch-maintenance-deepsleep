# Système de Gestion Intelligente de l'Eau (Anti-Gaspillage)

## Problématique
Au Maroc, le stress hydrique est un enjeu national. Dans les écoles, les robinets des toilettes restent souvent ouverts (ou fuient) pendant le week-end ou la nuit, gaspillant des tonnes d'eau.

## Concept
Un boîtier avec une électrovanne (valve motorisée) placée sur l'arrivée d'eau principale ou celle des sanitaires.

## Fonctionnement
- **Planning Hebdomadaire (RTC) :** Le système coupe automatiquement l'eau à la fin des cours (ex: 18h30) et la rallume le matin (ex: 07h30), et la garde coupée le dimanche.
- **Détection de fuite :** Ajout d'un capteur de débit d'eau. Si de l'eau coule en continu pendant plus de 30 minutes, le système coupe tout et envoie une alerte sur l'application locale du gardien.

## Composants Principaux (Prévisionnels)
- ESP32
- Module RTC (ex: DS3231) pour l'heure hors-ligne
- Relais pour contrôler l'électrovanne
- Électrovanne (Valve motorisée)
- Capteur de débit d'eau (Water flow sensor)
