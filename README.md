# Tetris Catch
Implémentation du mode de jeu Tetris Catch sur Tetris DS pour la carte Open1768

## Environnement de développement
- IDE : CLion à la place de uVision
- Compilateur : `arm-none-eabi-gcc` (standard GNU90)
- Build system : CMake en remplacement de la toolchain uVision
- Flash : serveur GDB officiel `JLinkGDBServerCLExe`

## Objectif du jeu
Le but est de réaliser le plus de carrés 4×4 en récupérant les tétrominos qui tombent dans l'écran du haut. Le joueur déplace la masse avec le joystick pour attraper et fusionner les pièces.

Dès qu'un carré 4×4 est formé, un timer de 10 secondes démarre. À 0, une explosion en croix détruit tous les blocs sur les lignes et colonnes des blocs critiques.

Le score augmente avec chaque action et fait progresser le niveau. Plus le niveau est élevé, plus les pièces tombent vite et plus le score monte rapidement.

Si un tétromino touche la masse en rotation ou tombe hors du terrain, le joueur perd des PV. À 0 PV, c'est Game Over.

## Contrôles

| Entrée | Action |
|--------|--------|
| Joystick | Déplacer la masse |
| K1 | Rotation anti-horaire |
| K2 | Rotation horaire |
| K7 | Soft drop (accélère la chute des pièces) |
| K6 | Déclencher l'explosion manuellement |

Le système DAS/ARR (Delayed Auto Shift / Auto Repeat Rate) est implémenté comme un tetris normal

## Mapping des connecteurs IO (`joystick.c`)

Le keypad IO5 est branché sur les connecteurs IO1 à IO5 de la carte. Le mapping port/pin est défini dans le tableau `IO_PORT_PIN` au début de `joystick.c`.

| Connecteur | Port | Pin |
|------------|------|-----|
| IO1 | P0 | 11 |
| IO2 | P1 | 20 |
| IO3 | P1 | 21 |
| IO4 | P2 | 13 |
| IO5 | P2 | 12 |

Pour changer le connecteur utilisé, modifier le tableau `IO_PORT_PIN` dans `joystick.c`

## Système de score
Le score augmente à chaque fusion de pièce, détection de carré 4×4 et explosion. Chaque gain est multiplié par le niveau courant. Le niveau progresse selon des seuils de score croissants jusqu'au niveau 10, puis toutes les 20 000 points (max niveau 99).

## Système de vie
Le joueur démarre avec 100 PV. Une pièce tombée hors terrain coûte 5 PV, une pièce détruite par rotation coûte 10 PV.

## Structure du projet

| Fichier | Rôle |
|---------|------|
| `main.c` | Boucle principale de jeu et de rendu |
| `game.c` | Logique du jeu |
| `render.c` | Fonctions de rendu LCD |
| `joystick.c` | Lecture des entrées (keypad + joystick) |
| `timer.c` | Configuration du timer |
| `memory.c` | Communication I2C avec la carte mémoire |
| `math.h` | Fonctions mathématiques |
| `utils.c` | Fonctions utilitaires (`min`, `max`) |
| `sprites_data.c` | Données des sprites |

Fichier modifié dans les bibliothèques :
- `ili_lcd_general.c` : modification des fonctions bas niveau pour interagir avec le LCD de façon optimale (mode Window)

## Architecture du code
Deux variables globales centrales : `gameState` (état du jeu) et `renderState` (état du rendu).

Convention de nommage : `<Module>_<NomMethode>`, ex. `Game_ApplyGravity`.

Le timer TIMER0 génère une interruption toutes les 10 ms pour mettre à jour les flags de `gameState`. Le mot-clé `volatile` garantit que les valeurs sont lues directement depuis la mémoire sans optimisation du compilateur.

## Hardware utilisé
- Carte Open1768 (LPC1768, ARM Cortex-M3)
- Carte mémoire I2C pour stocker le highscore (`memory.c`)
- LCD pour afficher le jeu (`ili_lcd_general.c`)
- IO5 Keypad pour les boutons et le joystick (`joystick.c`)

## Programmes récupérés depuis internet
Les algorithmes suivants n'ont pas été développés par moi :
- `math.h` › `isqrt32` : racine carrée entière 32 bits
- `math.h` › `floordiv8` : division entière par 8
- `math.h` › `prng` : générateur pseudo-aléatoire `xorshift`. Un bug du compilateur fait que `rng_state` est alloué mais jamais initialisé, ce qui rend sa valeur indéterminée à chaque exécution — comportement intentionnellement exploité pour la variété des parties.



