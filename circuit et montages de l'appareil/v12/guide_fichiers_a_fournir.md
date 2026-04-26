# Guide : Fichiers à fournir pour la commande du PCB

Pour commander votre PCB "AepBill", voici le rappel des fichiers et documents à manipuler :

### 1. Document de Spécification (Déjà prêt)
*   **Fichier** : `specifications_pcb.md`
*   **Utilisation** : À envoyer au technicien ou au commerçant qui va dessiner (router) votre carte. Ce document contient la liste des composants, les branchements et vos instructions de personnalisation (votre nom et le marquage des composants).

### 2. Dossier de Fabrication (À obtenir du technicien)
Une fois le dessin terminé par le technicien, il vous remettra un dossier compressé (souvent un `.zip`) contenant :
*   **Fichiers Gerber** : Les calques du circuit (pistes, texte, masque de soudure).
*   **Fichier de Perçage (Excellon / .DRL)** : Indique à la machine où percer les trous pour vos composants.

### Étapes à suivre :
1.  Fournissez le fichier `specifications_pcb.md` au commerçant.
2.  Récupérez les fichiers **Gerber** et **Drill** générés.
3.  Envoyez ces fichiers Gerber/Drill à l'usine de fabrication de votre choix.

---
*Note : Aucun fichier CPL (Pick and Place) n'est requis car vous effectuerez la soudure vous-même.*
