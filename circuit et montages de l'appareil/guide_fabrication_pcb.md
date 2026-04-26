# Guide de Fabrication PCB - AepBill Smart System

Ce document vous guide pour commander votre circuit imprimé (PCB) auprès d'un fabricant professionnel.

---

## Récapitulatif des Spécifications PCB

Utilisez ces paramètres lors de la commande en ligne :

| Paramètre | Valeur à sélectionner |
|:----------|:----------------------|
| **Dimensions** | 100mm × 80mm (ou 100mm × 120mm avec ACS712) |
| **Nombre de couches** | 2 layers |
| **Épaisseur PCB** | 1.6mm |
| **Épaisseur cuivre** | 1 oz (35µm) |
| **Matériau** | FR-4 TG130-140 |
| **Couleur du masque de soudure** | Vert (économique), Bleu, Noir, Blanc (au choix) |
| **Couleur de la sérigraphie** | Blanc (sur vert/bleu/noir), Noir (sur blanc) |
| **Finition de surface** | HASL (moins cher) ou ENIG (meilleure qualité) |
| **Trous métallisés** | Oui (PTH - Plated Through Hole) |
| **Nombre de PCB** | 5 ou 10 (minimum selon fabricant) |
| **Test électrique** | Oui (Flying Probe ou Netlist Test) |

> [!TIP]
> Pour une première commande, choisissez HASL et couleur verte pour minimiser les coûts. ENIG est recommandé pour une production en série ou un usage professionnel.

---

## Étape 1 : Choisir un Fabricant

### Fabricants Recommandés (Prix et Délais)

| Fabricant | Prix pour 5 PCB | Délai de fabrication | Livraison vers France | Notes |
|:----------|:----------------|:---------------------|:----------------------|:------|
| **JLCPCB** | ~2-5€ | 24-48h | 7-15 jours (DHL/FedEx) | Le moins cher, très populaire |
| **PCBWay** | ~5-10€ | 24-72h | 7-15 jours | Options avancées, bon support |
| **AllPCB** | ~5-8€ | 48h | 10-20 jours | Bonne qualité-prix |
| **Eurocircuits** | ~30-50€ | 3-5 jours | 3-5 jours | Fabrication européenne, rapide mais cher |
| **Aisler** | ~10-20€ | 3-5 jours | 5-7 jours | Européen (Pologne), bon compromis |

> [!NOTE]
> Les prix sont indicatifs pour 5 PCB de 100×80mm, 2 couches, HASL. Frais de port non inclus (~5-20€ selon urgence).

---

## Étape 2 : Créer les Fichiers Gerber

Les fichiers Gerber sont le format standard pour la fabrication PCB. Vous pouvez les créer avec :

### Option A : Logiciel de Conception PCB

Si vous concevez le PCB vous-même, utilisez un de ces logiciels :

#### **KiCad** (Gratuit, Open Source) - RECOMMANDÉ
1. Téléchargez KiCad : [https://www.kicad.org/](https://www.kicad.org/)
2. Importez le schéma depuis `schema_circuit.md`
3. Créez le layout selon `pcb_layout.md`
4. Exportez les Gerbers :
   - Menu : **File → Plot**
   - Layers : Cochez F.Cu, B.Cu, F.SilkS, B.SilkS, F.Mask, B.Mask, Edge.Cuts
   - Format : **Gerber**
   - Cliquez sur **Plot** puis **Generate Drill Files**

#### **EasyEDA** (Gratuit, en ligne)
1. Accédez à [https://easyeda.com/](https://easyeda.com/)
2. Créez un nouveau projet
3. Importez le schéma et créez le PCB
4. Exportez : **File → Export → Gerber**
5. Commandez directement via JLCPCB (intégration native)

#### **Eagle Autodesk** (Gratuit pour usage non commercial)
1. Téléchargez Eagle : [https://www.autodesk.com/products/eagle/](https://www.autodesk.com/products/eagle/)
2. Créez le schéma et le board
3. Exportez : **File → CAM Processor** → Template Gerber RS274X

### Option B : Service de Conception sur Mesure

Si vous n'avez pas l'expérience, vous pouvez faire appel à un designer PCB :
- **Fiverr** : 20-100€ pour un PCB simple
- **Upwork** : 50-200€ selon complexité
- **Forums EEVblog** : Communauté d'entraide

Fournissez-leur les fichiers :
- `schema_circuit.md`
- `pcb_layout.md`
- `liste_technique.md`

---

## Étape 3 : Vérifier les Fichiers Gerber

Avant de commander, **vérifiez toujours** vos Gerbers avec un viewer gratuit :

### **Gerber Viewer en Ligne** (Facile)
1. Allez sur [https://www.pcbway.com/project/OnlineGerberViewer.html](https://www.pcbway.com/project/OnlineGerberViewer.html)
2. Uploadez votre fichier ZIP de Gerbers
3. Inspectez :
   - ✅ Toutes les pistes sont connectées
   - ✅ Pas de pistes coupées ou isolées
   - ✅ Les pads des composants sont présents
   - ✅ Les trous de fixation sont visibles
   - ✅ La sérigraphie est lisible

### **Checklist de Vérification**

- [ ] Top Copper : Toutes les connexions GPIO, I2C, alimentation sont présentes
- [ ] Bottom Copper : Plan de masse GND uniforme, pas de zones isolées
- [ ] Top Silkscreen : Désignateurs (U1, U2, SW1, etc.) visibles et corrects
- [ ] Drill File : Trous pour vias, composants THT, et fixation M3
- [ ] Board Outline : Dimensions exactes (100×80mm)
- [ ] Solder Mask : Pads exposés uniquement (pas de cuivre nu ailleurs)

> [!WARNING]
> Une erreur dans les Gerbers peut rendre le PCB inutilisable. Prenez le temps de vérifier **chaque couche** avant de commander !

---

## Étape 4 : Commander le PCB

### Exemple avec JLCPCB (Le Plus Utilisé)

1. **Accédez au site** : [https://cart.jlcpcb.com/quote](https://cart.jlcpcb.com/quote)

2. **Uploadez les Gerbers** :
   - Cliquez sur **"Add gerber file"**
   - Sélectionnez votre fichier ZIP contenant les Gerbers
   - Le système détecte automatiquement les dimensions

3. **Configurez les paramètres** :
   ```
   Dimensions : 100 x 80 mm (détecté)
   Layers     : 2
   Quantity   : 5 (minimum, coût identique pour 5 ou 10 parfois)
   Thickness  : 1.6
   Color      : Green (ou au choix)
   Silkscreen : White
   Surface    : HASL (avec lead) ou HASL lead-free
   Copper     : 1 oz
   ```

4. **Options supplémentaires** (facultatif) :
   - **Remove Order Number** : Oui (pour PCB propre, +1.5€)
   - **Via Covering** : Tented (vias couverts par masque)
   - **Edge Connector** : No

5. **Ajouter au panier** :
   - Prix affiché : ~2-5€ pour 5 PCB
   - Frais de port : Choisissez l'option selon urgence
     - **Global Standard** : 5-10€, 15-30 jours
     - **DHL/FedEx Express** : 15-25€, 3-7 jours

6. **Paiement** :
   - Carte bancaire, PayPal acceptés
   - Total attendu : 10-30€ (PCB + port)

### Exemple avec PCBWay (Alternative)

1. **Accédez au site** : [https://www.pcbway.com/orderonline.aspx](https://www.pcbway.com/orderonline.aspx)

2. **Uploadez et configurez** (même processus que JLCPCB)

3. **Avantages PCBWay** :
   - Support technique plus réactif
   - Options de fabrication avancées (PCB flexible, aluminium, etc.)
   - Service d'assemblage (si vous voulez qu'ils soudent les composants)

### Service d'Assemblage (PCBA)

Si vous ne voulez **pas souder vous-même**, JLCPCB et PCBWay proposent l'assemblage :

1. **Uploadez** : Fichiers Gerber + BOM (Bill of Materials) + CPL (Component Placement List)
2. **Sélectionnez les composants** depuis leur catalogue
3. **Coût** : ~10-30€ supplémentaires + prix des composants

> [!TIP]
> Pour un premier prototype, assemblez vous-même pour économiser. Pour une production >10 unités, le PCBA devient rentable.

---

## Étape 5 : Réception et Inspection

### Délais de Livraison

| Méthode | Délai total |
|:--------|:------------|
| Fabrication | 2-5 jours |
| Expédition économique | 15-30 jours |
| Expédition express (DHL) | 3-7 jours |

**Total** : 5 jours (express) à 35 jours (économique)

### Inspection à la Réception

Lorsque vous recevez vos PCB :

1. **Inspection visuelle** :
   - [ ] Nombre de PCB correct (5 ou 10 commandés)
   - [ ] Pas de rayures ou fissures
   - [ ] Sérigraphie nette et lisible
   - [ ] Pads bien définis sans bavures

2. **Test de continuité** (Multimètre) :
   - [ ] Plan GND : Tous les GND sont bien connectés ensemble
   - [ ] Pas de court-circuit entre VIN (+5V) et GND
   - [ ] Isolation : Relais COM/NO/NC séparés (résistance infinie)

3. **Comparaison avec le design** :
   - [ ] Trous de fixation M3 au bon endroit
   - [ ] Espacement des pins ESP32 correct (testez avec le module)

> [!NOTE]
> Les fabricants chinois comme JLCPCB ont un excellent contrôle qualité. Les défauts sont rares, mais mieux vaut vérifier.

---

## Résolution de Problèmes

### PCB reçu avec des erreurs

| Problème | Solution |
|:---------|:---------|
| Mauvaises dimensions | Vérifiez vos Gerbers avant re-commande |
| Piste coupée | Réparation au fil de cuivre ou re-commande |
| Trous non métallisés | Erreur de fabrication, contactez le support pour remplacement |
| Numéro de commande visible | Commandé sans l'option "Remove Order Number", cosmétique seulement |

Les fabricants offrent généralement un **remplacement gratuit** si l'erreur vient de leur côté.

---

## Coût Total Estimé

Pour **5 PCB nus** (sans composants) :

| Poste | Coût |
|:------|:-----|
| Fabrication PCB (JLCPCB) | 2-5€ |
| Frais de port, express | 15-25€ |
| Options (remove order number) | 1.5€ |
| **TOTAL** | **20-30€** |

> [!TIP]
> Commandez 10 PCB au lieu de 5 : le surcoût est souvent minime (1-2€), et vous aurez des PCB de secours en cas d'erreur de soudure.

---

## Fichiers Nécessaires pour Commander

Assurez-vous d'avoir ces fichiers dans un **ZIP unique** :

```
AepBill-PCB-Gerbers.zip
├── AepBill-F.Cu.gbr          (Top Copper)
├── AepBill-B.Cu.gbr          (Bottom Copper)
├── AepBill-F.SilkS.gbr       (Top Silkscreen)
├── AepBill-B.SilkS.gbr       (Bottom Silkscreen)
├── AepBill-F.Mask.gbr        (Top Solder Mask)
├── AepBill-B.Mask.gbr        (Bottom Solder Mask)
├── AepBill-Edge.Cuts.gbr     (Board Outline)
└── AepBill-PTH.drl           (Drill File)
```

---

## Prochaine Étape

Une fois vos PCB reçus et inspectés, passez à l'assemblage :

→ [Guide d'Assemblage PCB](file:///c:/verssion11classicfinaltestee%20-%20Copie/circuit%20et%20montages%20de%20l%27appareil/guide_assemblage_pcb.md)

Ce guide vous montrera comment souder tous les composants sur votre PCB, étape par étape.
