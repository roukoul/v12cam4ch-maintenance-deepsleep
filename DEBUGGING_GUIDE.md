# Guide de Débogage des Logs GitHub Actions (Android)

Ce guide vous aide à trouver rapidement les erreurs pertinentes dans les logs de build GitHub Actions.

## 1. Mots-clés de Recherche (Ctrl+F)

Copiez et cherchez ces termes dans l'ordre de priorité :

1.  **`e: file://`**
    *   **C'est le PLUS IMPORTANT.**
    *   Il pointe directement vers le fichier et la ligne de l'erreur Kotlin.
    *   *Exemple :* `e: file:///.../MainActivity.kt:25:10 Unresolved reference`

2.  **`error:`** (avec les deux points)
    *   Montre souvent des erreurs génériques ou de compilation Java/C++.

3.  **`WHAT WENT WRONG`**
    *   Le début du résumé de l'échec Gradle. Regardez les lignes juste en dessous.

4.  **`Execution failed for task`**
    *   Indique quelle tâche a échoué (ex: `:app:compileDebugKotlin`).
    *   **Astuce :** Remontez de 20 à 50 lignes au-dessus de cette phrase pour voir la vraie cause.

5.  **`Caused by:`**
    *   Souvent enfoui dans les "stack traces". La première occurrence est souvent la plus pertinente.

6.  **`Unresolved reference`**
    *   Indique qu'une variable ou une classe est inconnue (oubli d'import ?).

## 2. Astuces pour la Lecture

*   **Ignorez les `w: ...`** : Ce sont des avertissements (warnings), ils ne bloquent généralement pas le build.
*   **Ignorez `Task ... FAILED`** si vous ne voyez pas de détails. Cherchez *au-dessus* de cette ligne.
*   **Si vous voyez `KaptExecution`,** le problème vient souvent d'une librairie (Hilt, Room). Cherchez des erreurs de *syntaxe* plus haut.

## 3. Format Idéal pour le Partage

Quand vous trouvez l'erreur, copiez un bloc comme ceci pour l'assistant :

```text
e: file:///path/to/MyFile.kt:10:15 Unresolved reference: MyClass
e: file:///path/to/MyFile.kt:12:5 Type mismatch: inferred type is String but Int was expected

> Task :app:compileDebugKotlin FAILED
```

---
*Gardez ce fichier à la racine de votre projet pour référence future.*
