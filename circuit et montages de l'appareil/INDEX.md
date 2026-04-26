# Index - Documentation PCB AepBill

Ce fichier index vous guide vers tous les documents nécessaires pour créer votre circuit imprimé.

---

## 📁 Fichiers Créés pour le PCB

Voici tous les documents disponibles dans ce dossier :

### 🎯 COMMENCER ICI

1. **[README_PCB.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/README_PCB.md)** ⭐
   - **Point d'entrée principal**
   - Vue d'ensemble complète du projet PCB
   - Visualisations 3D et schémas
   - Workflow complet de A à Z
   - FAQ et support
   - **À lire en premier !**

---

### 📐 Phase 1 : Conception et Planification

2. **[schema_circuit.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/schema_circuit.md)**
   - Schéma électrique complet
   - Détails de toutes les connexions
   - Bus I2C, GPIO, alimentation
   - Composants passifs et protection
   - Condensateurs de découplage recommandés

3. **[pcb_layout.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/pcb_layout.md)**
   - Layout du PCB (100×80mm)
   - Placement précis de tous les composants
   - Spécifications de routage
   - Guidelines pour les pistes
   - Export vers Gerber

4. **[liste_technique.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/liste_technique.md)**
   - Bill of Materials (BOM) complète
   - Tous les composants nécessaires
   - Références et quantités
   - Pinout ESP32 détaillé

---

### 🏭 Phase 2 : Fabrication du PCB

5. **[guide_fabrication_pcb.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_fabrication_pcb.md)**
   - Comment commander votre PCB en ligne
   - Choix du fabricant (JLCPCB, PCBWay)
   - Création des fichiers Gerber
   - Vérification avant commande
   - Coût estimé : 20-30€

6. **[template_kicad.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/template_kicad.md)**
   - Tutorial complet pour KiCad
   - Créer le schéma étape par étape
   - Créer le layout PCB
   - Générer les Gerbers
   - Alternative : EasyEDA

---

### 🔧 Phase 3 : Assemblage

7. **[guide_assemblage_pcb.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md)**
   - Instructions de soudure pas à pas
   - Outils nécessaires
   - Ordre de soudure recommandé
   - Tests de continuité
   - Programmation du firmware
   - Dépannage

---

### 📡 Phase 4 : Configuration et Déploiement

8. **[guide_rebuild_flash.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_rebuild_flash.md)**
   - Compiler et flasher le firmware ESP-IDF
   - Configuration avec/sans chiffrement
   - Commandes de flash

9. **[guide_connexion.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_connexion.md)**
   - Se connecter au système via Wi-Fi
   - Accès à l'interface web
   - Mode AP vs Station
   - Application Android

---

### 📚 Référence (Breadboard Original)

10. **[guide_montage.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_montage.md)**
    - Guide de câblage sur breadboard original
    - Référence pour comprendre les connexions
    - Utile pour prototypage rapide

---

## 🚀 Workflow Recommandé

Suivez cet ordre pour créer votre PCB :

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. Lisez README_PCB.md (vue d'ensemble)                        │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 2. Étudiez schema_circuit.md (comprenez le circuit)           │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 3. Consultez pcb_layout.md (visualisez le PCB)                │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 4. Créez le PCB avec KiCad (template_kicad.md)                │
│    OU utilisez EasyEDA (plus simple pour débutants)            │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 5. Commandez le PCB (guide_fabrication_pcb.md)                │
│    Coût : 20-30€, Délai : 5-35 jours                          │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 6. Achetez les composants (liste_technique.md)                │
│    Coût : 30-50€                                               │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 7. Assemblez le PCB (guide_assemblage_pcb.md)                 │
│    Soudure, tests, vérifications                              │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 8. Flashez le firmware (guide_rebuild_flash.md)               │
└────────────────┬────────────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────────────┐
│ 9. Configurez et testez (guide_connexion.md)                  │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 ▼
              ✅ PCB fonctionnel !
```

---

## 📊 Résumé Rapide

| Aspect | Détails |
|:-------|:--------|
| **Dimensions PCB** | 100mm × 80mm (version compacte) |
| **Nombre de couches** | 2 (Top + Bottom avec plan GND) |
| **Coût total** | 80-130€ (première carte, outils inclus) |
| **Délai total** | 1-6 semaines (fabrication + assemblage) |
| **Difficulté** | Intermédiaire (soudure THT requise) |
| **Composants principaux** | ESP32, LCD I2C, RTC DS3231, Relais 5V |
| **Alimentation** | 5V DC, 1-2A |
| **Sortie** | Relais 220V AC, 10A max |

---

## 🎯 Pour les Débutants

Si c'est votre **première fois** avec un PCB :

1. **Commencez simple** :
   - Lisez [README_PCB.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/README_PCB.md) en entier
   - Regardez les images 3D pour visualiser

2. **Utilisez EasyEDA** au lieu de KiCad :
   - Plus simple pour débuter
   - Interface en ligne, pas d'installation
   - Commande directe JLCPCB intégrée

3. **Commandez le PCB** :
   - Suivez [guide_fabrication_pcb.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_fabrication_pcb.md)
   - JLCPCB est le plus simple et économique

4. **Pratiquez la soudure** :
   - Avant de souder le PCB final, entraînez-vous sur un PCB de test
   - [guide_assemblage_pcb.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md) a des astuces

---

## ⚠️ Points d'Attention

### Sécurité

> [!CAUTION]
> **HAUTE TENSION** : Le relais manipule du 220V AC. Sections J2 et J3 (si ACS712) sont dangereuses. Ne manipulez que si vous êtes qualifié.

### Composants Optionnels

- **ACS712** : Capteur de courant (désactivé dans firmware actuel)
- Si non utilisé : PCB peut être réduit à 100×80mm (sans zone U5/J3)

### Condensateurs de Découplage

- **Critiques !** Ne pas oublier C1, C2, C3, C4, C5, C6
- Mauvaise alimentation = resets intempestifs de l'ESP32

### Diode D1

- **Obligatoire** sur le relais (protection contre surtension bobine)
- Ne pas oublier ou inverser la polarité

---

## 🔗 Ressources Externes

### Logiciels

- **KiCad** : [kicad.org](https://www.kicad.org/) - Conception PCB professionnelle (gratuit)
- **EasyEDA** : [easyeda.com](https://easyeda.com/) - Conception en ligne simplifiée (gratuit)
- **Gerber Viewer** : [pcbway.com/gerberviewer](https://www.pcbway.com/project/OnlineGerberViewer.html)

### Fabricants PCB

- **JLCPCB** : [jlcpcb.com](https://jlcpcb.com/) - Le moins cher (~2-5€/5pcs)
- **PCBWay** : [pcbway.com](https://www.pcbway.com/) - Qualité, bon support
- **Aisler** : [aisler.net](https://aisler.net/) - Européen (Pologne)

### Fournisseurs Composants

- **AliExpress** : Prix les plus bas, délai 2-6 semaines
- **Amazon.fr** : Plus cher mais livraison rapide (2-5 jours)
- **Mouser/Digikey** : Composants pros, fiables

### Communautés

- **ESP32 Forum** : [esp32.com](https://www.esp32.com/)
- **KiCad Forum** : [forum.kicad.info](https://forum.kicad.info/)
- **EEVblog** : Aide conception électronique

---

## 📝 Notes et Modifications

### Version Actuelle : v1.0

Documentations créées pour :
- **Projet** : AepBill Smart System v11 Classic
- **Date** : Décembre 2025
- **Compatible avec** : ESP32 WROOM-32, ESP-IDF

### Futures Améliorations Possibles

Idées pour versions futures du PCB :

1. **Version compacte** : Utiliser un module ESP32 soudé (gain de place)
2. **Connecteurs JST** : Au lieu de headers pour câblage propre
3. **Boîtier imprimé 3D** : Fichiers STL pour impression 3D
4. **Version SMD** : Composants montés en surface (plus compact)
5. **Power LED** : Indicateur visuel 5V alimenté
6. **Protection ESD** : Diodes TVS sur I2C et GPIO exposés

---

## ✅ Checklist Complète

Cochez au fur et à mesure de votre progression :

### Conception
- [ ] Lecture README_PCB.md
- [ ] Compréhension du schéma_circuit.md
- [ ] Étude du pcb_layout.md
- [ ] Création du design KiCad/EasyEDA
- [ ] Vérification ERC (schéma)
- [ ] Vérification DRC (PCB)
- [ ] Génération fichiers Gerber
- [ ] Vérification Gerber Viewer en ligne

### Fabrication
- [ ] Commande PCB (JLCPCB ou autre)
- [ ] Commande composants parallèlement
- [ ] Réception PCB (inspection visuelle)
- [ ] Réception composants

### Assemblage
- [ ] Préparation outils soudure
- [ ] Soudure composants passifs
- [ ] Soudure connecteurs et headers
- [ ] Soudure buzzer et boutons
- [ ] Inspection soudures (loupe)
- [ ] Test continuité GND
- [ ] Test isolation VIN/GND
- [ ] Insertion modules (ESP32, LCD, RTC, Relais)

### Test et Déploiement
- [ ] Test alimentation 5V (sans charge)
- [ ] Programmation firmware ESP32
- [ ] Test LCD (affichage)
- [ ] Test RTC (conservation heure)
- [ ] Test boutons (reset/restart)
- [ ] Test relais (clic, pas de charge AC encore)
- [ ] Test buzzer (son)
- [ ] Test interface web (Wi-Fi)
- [ ] Mise en boîtier
- [ ] Test final complet
- [ ] Documentation utilisateur

---

## 📞 Support

Si vous rencontrez des difficultés :

1. **Consultez la FAQ** dans [README_PCB.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/README_PCB.md)
2. **Section Dépannage** dans [guide_assemblage_pcb.md](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md)
3. **Forums communautaires** (KiCad, ESP32)
4. **Revérifiez vos soudures** (99% des problèmes viennent de là)

---

**Bonne création de votre PCB AepBill Smart System !** 🎉⚡

*Documentation créée le 26 décembre 2025*
