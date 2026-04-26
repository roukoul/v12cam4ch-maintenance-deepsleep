# Prompt pour Génération de Documentation

Utilisez ce prompt dans un nouveau projet pour générer automatiquement la documentation matérielle et technique similaire à celle de ce dossier.

***

Je souhaite créer un dossier de documentation matérielle complet pour ce projet, comme nous l'avons fait précédemment.

Merci de réaliser ces tâches étape par étape :

1.  **Créer un dossier** nommé `circuit et montages de l'appareil` à la racine.
2.  **Analyser le code source** (fichiers main, drivers, configuration) pour identifier tous les composants matériels et écrire une `liste_technique.md` (BOM) précise, en notant si certains composants sont désactivés dans le code.
3.  **Créer un `guide_montage.md`** expliquant comment brancher les composants (avec les pins GPIO exacts trouvés dans le code).
4.  **Créer un `guide_connexion.md`** expliquant comment se connecter à l'appareil (IP, mDNS, AP, App Android) selon le code analysé.
5.  **Créer un `guide_rebuild_flash.md`** détaillant les procédures de compilation et de flashage (avec et sans encryption) adaptées à la configuration du projet (`sdkconfig`).
