Voici une feuille de route (roadmap) détaillée pour la création d’un mini moteur de regex en C. L’idée est de couvrir les principales étapes, du découpage du pattern jusqu’à l’exécution des correspondances (matching) via un automate. Les points forts mentionnés : captures, métacaractères ( ., *, +, ?, |, [], {} ), et l’utilisation d’un AST pour organiser les éléments syntaxiques.

---

## 1. Définir l’objectif et le périmètre du projet

1. **Fonctionnalités cibles :**
   - **Caractère joker** : `.` (n’importe quel caractère)
   - **Quantificateurs** : `*`, `+`, `?`, `{m,n}`
   - **Alternatives** : `|`
   - **Classes de caractères** : `[ ]` (avec ranges, par ex. `[a-z]`)
   - **Captures** : `(...)`
   - **Groupes non capturants** (optionnel) : `(?: ... )` (selon vos besoins)
   - **Échappements** : `\ ` (échapper des symboles spéciaux, par ex. `\*`, `\(`, etc.)

2. **Contraintes techniques :**
   - **Implémentation en C** : Respecter la gestion mémoire manuelle.
   - **Sans macro** : Code plus “classique”, pas d’artifices.
   - **Performance** : Pas forcément votre premier critère, mais un code “propre” facilitera les optimisations ultérieures.

3. **Approche générale** :  
   - **Parser** qui lit la regex et construit une structure de données (souvent un AST).
   - **Interpréteur** (ou **compilation**) qui transforme l’AST en un automate (généralement un NFA ou un DFA simplifié).
   - **Exécution** : Parcourir la chaîne à matcher à l’aide de l’automate, en gérant les captures.

---

## 2. Définir la grammaire et la représentation interne

Avant d’implémenter la moindre ligne de code, définissez clairement la **grammaire** (la “syntaxe” de vos regex). Voici un exemple (simplifié) de grammaire EBNF :

```
Regex         = Alternative ( "|" Alternative )* ;
Alternative   = Concatenation+ ;
Concatenation = QuantifiedAtom+ ;
QuantifiedAtom = Atom Quantifier? ;
Atom          = Char
              | "."
              | "(" Regex ")"
              | "[" CharacterClass "]" ;
Quantifier    = "*" | "+" | "?" | "{" Number ( "," Number )? "}" ;
CharacterClass = ... ;  // Contiendra la logique pour les ranges, ex: [a-zA-Z0-9]
Number        = [0-9]+ ;
```


## 2.1 Qu’est-ce qu’une grammaire EBNF ?

- **EBNF** signifie *Extended Backus–Naur Form*. C’est une façon de décrire la syntaxe d’un langage (ici, un sous-ensemble de regex).  
- Quand on lit une règle comme :
  ```
  Regex = Alternative ( "|" Alternative )* ;
  ```
  on l’interprète de cette façon : “Un **Regex** est composé d’une **Alternative**, suivie de zéro ou plusieurs occurrences de `|` puis d’une autre **Alternative**.”

- Les symboles suivants sont importants :  
  - `*` après un élément signifie “zéro ou plusieurs fois”  
  - `+` signifie “une ou plusieurs fois”  
  - `?` signifie “zéro ou une fois”  
  - Les parenthèses `( ... )` servent à regrouper des sous-éléments  

---

## 2.2 Décomposition règle par règle

Voici la grammaire donnée :

```
Regex         = Alternative ( "|" Alternative )* ;
Alternative   = Concatenation+ ;
Concatenation = QuantifiedAtom+ ;
QuantifiedAtom = Atom Quantifier? ;
Atom          = Char
              | "."
              | "(" Regex ")"
              | "[" CharacterClass "]" ;
Quantifier    = "*" | "+" | "?" | "{" Number ( "," Number )? "}" ;
CharacterClass = ... ;  // Contiendra la logique pour les ranges, ex: [a-zA-Z0-9]
Number        = [0-9]+ ;
```

### 2.2.1. `Regex = Alternative ( "|" Alternative )* ;`
- **Signification** : Un `Regex` est une ou plusieurs “Alternatives” séparées par le symbole `|`.  
- **Pourquoi ?** : Dans de nombreuses syntaxes de regex, le `|` correspond à l’opérateur “ou” (alternation).  
- **Exemple** : l’expression `ab|cd` contient deux `Alternative` : `ab` d’un côté et `cd` de l’autre.  
- **En pratique** :  
  - On lit la première `Alternative`.  
  - Puis, si on trouve un `|`, on en lit une deuxième, et ainsi de suite.  
  - Cela signifie que la regex `Regex = A|B|C` matchera soit A, soit B, soit C.

---

### 2.2.2. `Alternative = Concatenation+ ;`
- **Signification** : Une “Alternative” est une ou plusieurs “Concatenations”.  
- **Pourquoi “Concatenation+” ?** :  
  - En fait, une “Alternative” peut être vue comme une suite de morceaux qui se suivent (une concaténation), parfois d’ailleurs on peut simplifier en disant “une Alternative est simplement une concaténation d’éléments”.  
  - Le `+` veut dire : au moins un “Concatenation”.  
- **Exemple** : si on prend la partie `ab` dans la regex `ab|cd`, on peut voir `ab` comme un enchaînement (une concaténation) de deux atomes `a` et `b`.

(*Selon la grammaire, vous pourriez parfois combiner `Alternative` et `Concatenation` de différentes manières. Ici, l’auteur sépare conceptuellement l’opérateur “ou” (Alternative) de l’opérateur “suivi” (Concatenation).*)

---

### 2.2.3. `Concatenation = QuantifiedAtom+ ;`
- **Signification** : Une “Concatenation” est une suite d’un ou plusieurs “QuantifiedAtom”.  
- **Idée** : Dans une regex, on enchaîne souvent plusieurs unités. Par exemple, `abc\d+` est une concaténation de trois parties :  
  1. `a`  
  2. `b`  
  3. `c\d+` (ce dernier serait un “Atom” suivi d’un quantificateur +)  
- **Le `+`** : Encore une fois, indique qu’on doit avoir au moins un “QuantifiedAtom”, et on peut en avoir plusieurs (ex: `QuantifiedAtom QuantifiedAtom QuantifiedAtom...`).

---

### 2.2.4. `QuantifiedAtom = Atom Quantifier? ;`
- **Signification** : Un “QuantifiedAtom” est un “Atom” suivi (optionnellement) d’un “Quantifier”.  
- **Exemples** :  
  - `a*` : Ici, `Atom = 'a'`, `Quantifier = '*'`  
  - `a` (sans rien) : Ici, c’est un “Atom” sans quantificateur.  
  - `(ab)?` : Ici, `Atom = '(ab)'` et `Quantifier = '?'`.  
- **Le `?`** après `Quantifier` indique qu’on peut mettre un quantificateur ou non.  

En gros, cette règle capture l’idée que dans une regex, on a un “truc” (un atome) qui peut se répéter ou non, de diverses manières (par *, +, ?, ou {m,n}).

---

### 2.2.5. `Atom = Char | "." | "(" Regex ")" | "[" CharacterClass "]" ;`
Cette règle décrit **tout ce qui peut être un “atome”** au sens regex :

1. `Char` : un caractère normal, par exemple `'a'` ou `'b'`.  
2. `.` : le caractère joker, qui match “n’importe quel caractère”.  
3. `(` Regex `)` : un groupe (qui peut être capturant). Cela signifie qu’à l’intérieur des parenthèses, on a toute une regex complète (on retombe donc sur la règle `Regex`).  
4. `[` CharacterClass `]` : une classe de caractères, comme `[a-z0-9]`.  

**Idée générale** : un “atome” est l’unité de base qui peut se répéter (via les quantificateurs).  

---

### 2.2.6. `Quantifier = "*" | "+" | "?" | "{" Number ( "," Number )? "}" ;`
- **Signification** : Le “Quantifier” peut être :
  1. `*` : signifie “zéro ou plusieurs fois”  
  2. `+` : signifie “une ou plusieurs fois”  
  3. `?` : signifie “zéro ou une fois”  
  4. `{ Number ( "," Number )? }` : signifie “répétitions bornées”, par ex. `{2}` ou `{2,4}`.  
     - `Number ( "," Number )?` veut dire : un nombre suivi éventuellement d’une virgule et d’un deuxième nombre.  
- **Exemple** :  
  - `{3}` veut dire “exactement 3 fois”  
  - `{2,5}` veut dire “entre 2 et 5 fois”  
  - `{,5}` ou `{2,}` (selon la syntaxe autorisée) peuvent être des abréviations pour “0 à 5 fois” ou “au moins 2 fois”.  

---

### 2.2.7. `CharacterClass = ... ;`
- **Signification** : Une “CharacterClass” représente ce qu’il y a entre les crochets `[ ]`.  
  - Par exemple : `[a-zA-Z0-9]`.  
  - On peut avoir des plages (ranges) `a-z`, des caractères séparés, `^` pour la négation, etc.  
- **Ici**, la grammaire dit simplement `...` parce que la logique peut devenir plus complexe. On pourrait préciser :

  ```
  CharacterClass = ( "^"? ClassAtomRange+ ) ;
  ClassAtomRange = ClassAtom ( "-" ClassAtom )? ;
  ClassAtom = ... // un caractère, ou un caractère échappé
  ```

  ... et ainsi de suite, selon le niveau de détail voulu.  

**Le principe** est : à l’intérieur de `[ ]`, on définit un ensemble de caractères possibles.

---

### 2.2.8. `Number = [0-9]+ ;`
- **Signification** : Un “Number” est une suite d’un ou plusieurs chiffres (0-9).  
- **Usage** : On l’emploie notamment pour les quantificateurs `{m,n}`.  

---

# 3. Comment tout cela s’articule ?

En pratique, quand vous parsez (analysez) une chaîne de caractères représentant une regex, vous procédez dans l’ordre :

1. **Regex** : Chercher des `Alternative`s séparées par `|`.  
2. **Alternative** : Chacune se décompose en plusieurs “Concatenation”.  
3. **Concatenation** : Chaque partie est un “QuantifiedAtom”.  
4. **QuantifiedAtom** : Un “Atom” qui peut être suivi d’un quantificateur (optionnel).  
5. **Atom** : Peut être un caractère normal, un point `.`, un groupe `(...)`, ou une classe `[ ... ]`.  

Chaque fois qu’on descend d’une règle à l’autre, on lit la chaîne de caractères selon la structure définie par la grammaire.

---

## 3. Conception de l’AST

### 3.1. Quels nœuds prévoir ?

Un AST (Abstract Syntax Tree) contiendra typiquement des nœuds correspondant aux éléments de la grammaire. Par exemple :

- **NodeType** :
  - `NODE_LITERAL` : pour un caractère simple (ex. `a`, `b`, etc.)
  - `NODE_DOT` : pour le `.`
  - `NODE_ALTERNATION` : pour le `|`
  - `NODE_CONCAT` : pour la concaténation
  - `NODE_QUANTIFIER` : pour les répétitions (`*`, `+`, `?`, `{m,n}`)
  - `NODE_CHARCLASS` : pour `[ ]`
  - `NODE_GROUP` : pour `( ... )` (avec indication sur la capture)

Le type du nœud et ses enfants permettent de représenter la structure hiérarchique de la regex.

### 3.2. Structure en C

On peut imaginer quelque chose comme :

```c
typedef enum {
    NODE_LITERAL,
    NODE_DOT,
    NODE_ALTERNATION,
    NODE_CONCAT,
    NODE_QUANTIFIER,
    NODE_CHARCLASS,
    NODE_GROUP
} NodeType;

typedef struct AST {
    NodeType type;
    // Pour gérer les enfants
    struct AST *left;
    struct AST *right;
    // Pour quantifier, capturer, etc.
    int min;    // ex. pour {m,n}
    int max;    // ex. pour {m,n}
    char *chars; // pour stocker les chars d’une classe de caractères ou un littéral
    int group_id; // index de capture, si nécessaire
} AST;
```

Ceci est un schéma simplifié et modifiable selon vos besoins.

---

## 4. Écriture du parseur (construction de l’AST)

### 4.1. Lexing (optionnel ou simplifié)

- Extraire des **tokens** depuis la regex brute :  
  - `TOKEN_LITERAL('a')`, `TOKEN_DOT`, `TOKEN_STAR`, `TOKEN_PLUS`, `TOKEN_QUESTION`, `TOKEN_LBRACKET`, etc.
- Gérer les échappements (`\(`, `\)`, etc.) correctement.

Dans un premier temps, un “lexer” minimaliste peut se contenter d’itérer caractère par caractère et renvoyer des tokens. Mais pour un tout petit moteur, vous pouvez parfois vous en passer et parser “à la volée”.

### 4.2. Implémentation du parseur récursif

- **parseRegex()** : gère les `|` (alternation).
- **parseAlternative()** : gère la concaténation de sous-expressions.
- **parseQuantifiedAtom()** : regarde si un `Atom` est suivi d’un quantificateur (`*`, `+`, `?`, `{}`).
- **parseAtom()** : 
  1. Si c’est un caractère normal (littéral), crée un `NODE_LITERAL`.
  2. Si c’est un `.`, crée un `NODE_DOT`.
  3. Si c’est `(`, alors faire un appel récursif pour tout ce qui se trouve entre `(` et `)`.
  4. Si c’est `[`, gérer la classe de caractères jusqu’au `]`.

L’objectif est de remplir correctement votre AST à chaque étape.

### 4.3. Gestion des groupes de capture

- Lorsque vous rencontrez `(`, vous pouvez affecter un **identifiant de capture** (ex. `group_id`) à ce groupe.  
- Stockez dans une structure globale ou dans un tableau la liste des groupes.  

---

## 5. Transformation de l’AST en un automate (NFA ou autre)

### 5.1. Choix de l’automate : NFA (Non-deterministic Finite Automaton)

Les moteurs de regex “classiques” (p. ex. via la méthode de Thompson) génèrent un **NFA** qui peut être évalué avec un algorithme de backtracking ou de simulation.

1. **NFA** : Chaque nœud correspond à un ensemble d’états, avec des transitions ε (epsilon) pour la gestion des quantificateurs.  
2. **Exemple** (simplifié) :  
   - Un `NODE_LITERAL('a')` se traduit par un état qui vérifie si le caractère courant est `a`, puis va à l’état suivant.  
   - Un `NODE_QUANTIFIER(*, +, etc.)` introduit souvent des transitions ε pour autoriser la répétition ou l’optionnalité.  

### 5.2. Génération de l’automate

- Parcourir l’AST en profondeur (DFS ou visite récursive).
- À chaque nœud, créer un (ou plusieurs) états et lier les transitions.  
- Les **captures** (groupes) peuvent être implémentées en mémorisant dans quels états de l’automate commence/termine chaque groupe.

---

## 6. Mécanisme de matching

### 6.1. Évaluation du NFA

Une méthode simple (inspirée de l’algorithme de Thompson) :

1. **Initialisation** : On place le moteur dans l’état initial du NFA (en tenant compte des transitions ε).
2. **Pour chaque caractère** de la chaîne d’entrée :
   - On détermine tous les états accessibles qui exigent le caractère courant.
   - On avance la position dans la chaîne.
   - On tient compte des transitions ε pour “propager” ces états.
3. **Acception** : Si, à la fin, on est dans un état “terminal”, alors la regex match.

### 6.2. Gestion des captures

- Lorsque vous entrez dans un groupe de capture, notez la position dans la chaîne d’entrée (ex. `start[group_id] = current_index`).  
- Lorsque vous en sortez, notez la position de fin (ex. `end[group_id] = current_index`).  
- À la fin, si la regex match, vous pouvez extraire les sous-chaînes capturées.

**Astuce** : Il faut gérer les captures dans un contexte potentiellement non déterministe, donc vous stockez souvent les positions de captures dans un “contexte d’exécution” qui est cloné ou mis à jour en fonction des branches.  
Pour un petit moteur, un backtracking explicite peut être plus simple à coder au début qu’une simulation NFA totalement “pure”.  

---

## 7. Implémentation pas-à-pas (proposition concrète)

1. **Étape 0 : Boîte à outils**  
   - Fonctions de manipulation de chaînes (ex. `strncpy`, `strcmp`, etc.)  
   - Fonctions d’allocation pour créer de nouveaux nœuds d’AST ( `AST *new_node(NodeType type)` ).  
   - Structures pour le parseur (curseur, pointeur sur la chaîne en cours, etc.).  

2. **Étape 1 : Parseur**  
   - Implémenter les fonctions de parsing ( `parseRegex()`, `parseAlternative()`, etc. ) dans un fichier `parser.c`.  
   - Tester sur des cas simples : `"a"`, `"ab"`, `"(ab)"`, `"a|b"`, `"(a|b)*"`, etc.

3. **Étape 2 : Visualisation / debug de l’AST**  
   - Écrire une fonction de debug qui affiche l’AST en texte (unpretty print) pour vérifier la structure.

4. **Étape 3 : Génération du NFA**  
   - Implémenter un fichier `nfa.c` dans lequel vous avez des fonctions de création d’états et de transitions.  
   - Parcourir l’AST et générer les états.  
   - Tester avec des expressions simples.  

5. **Étape 4 : Évaluation du NFA**  
   - Implémenter la logique (type algorithme de Thompson) pour avancer dans la chaîne.  
   - Vérifier si un état terminal est atteint.  
   - Gérer (dans un premier temps) l’absence de captures ou des captures basiques.  

6. **Étape 5 : Gestion complète des captures**  
   - Étendre la structure d’état pour conserver la position d’entrée où ont débuté les groupes.  
   - Associer l’entrée/sortie de chaque groupe à une transition dans l’automate, ou gérer cela via un stack de backtracking.  

7. **Étape 6 : Ajout de fonctionnalités avancées**  
   - **Classes de caractères** : `[a-zA-Z0-9]`, inclure ou exclure. Gérer les métacaractères dedans.  
   - **Quantificateurs bornés `{m,n}`** : rajouter la logique dans le parseur + la génération du NFA.  
   - **Optimisations** : par exemple, si le moteur est trop lent pour certains patterns, envisager la construction d’un DFA ou l’optimisation des quantificateurs (possiblement plus complexe).  

8. **Étape 7 : Nettoyage et documentation**  
   - Organiser le code en modules (`parser.h/c`, `ast.h/c`, `nfa.h/c`, `matcher.h/c`, etc.).  
   - Rédiger la documentation de l’API (comment construire une `Regex *`, comment l’utiliser pour matcher, comment extraire les captures, etc.).  

---

## 8. Plan de test

Pour assurer que tout fonctionne, préparez un ensemble de tests :

1. **Cas unitaires simples** : 
   - Expression : `"a"` ; Chaîne test : `"a"`, `"b"`, `""`
   - Expression : `"ab"` ; Chaîne test : `"ab"`, `"abc"`, `"xb"`  
2. **Quantificateurs** : 
   - `"a*"` ; chaînes : `""`, `"a"`, `"aaa"`, etc.  
   - `"(ab)+"` ; chaînes : `"ab"`, `"abab"`, `"ababa"`, etc.  
3. **Alternatives** : 
   - `"a|b"` ; chaînes : `"a"`, `"b"`, `"c"`, `""`
   - `"(ab|cd)*"` ; chaînes : `"ab"`, `"cd"`, `"abcd"`, `"cdab"`, etc.  
4. **Classes de caractères** : 
   - `"[a-z]"` ; testez `"a"`, `"Z"`, `"m"`, etc.  
   - `"[a-zA-Z]"` ; testez `"m"`, `"M"`, `"9"`, etc.  
5. **Captures** : 
   - `"([0-9]+)ab"` ; vérifier le contenu de capture pour `"123ab"`.  
6. **Quantificateurs bornés `{m,n}`** : 
   - `"a{2,4}"` ; chaînes : `"a"`, `"aa"`, `"aaa"`, `"aaaa"`, `"aaaaa"`  
7. **Cas aux limites** : 
   - Expression vide, expression très longue, classe vide `[]`, etc.  
8. **Cas d’erreur** : 
   - Parenthèses non fermées `"((abc"`, crochet non fermé `"[abc"`, quantificateur illégal `"{,5}"`, etc.

---

## 9. Points d’attention et bonnes pratiques

1. **Gestion d’erreurs** :  
   - Dans le parseur, gérer les erreurs de syntaxe (caractère inattendu, parenthèse non fermée).  
   - Retourner des messages clairs (ou codes d’erreur).  

2. **Mémoire** :  
   - Allouer et libérer l’AST proprement.  
   - Être rigoureux sur les allocations d’états NFA et transitions.  

3. **Lisibilité** :  
   - Bien structurer le code en modules et fonctions courtes.  
   - Éviter au maximum de tout mettre dans une seule fonction.  

4. **Performance** (si nécessaire plus tard) :  
   - Éviter un backtracking trop naïf (certains patterns peuvent exploser combinatoirement).  
   - Si votre moteur doit gérer des expressions complexes, songez à utiliser des optimisations (ex. partial DFA).  

5. **Compatibilité** :  
   - Décider si vous voulez supporter des fonctionnalités plus complexes (lookaround, backreferences, etc.). Pour l’instant, c’est hors du scope si vous voulez rester “mini”.  

---

## Liens utiles

   - **AST**
      - https://medium.com/basecs/leveling-up-ones-parsing-game-with-asts-d7a6fc2400ff
      - https://www.cs.columbia.edu/~sedwards/classes/2003/w4115/ast.pdf
      - https://www-inf.telecom-sudparis.eu/COURS/CSC4251_4252/Diapos/c06-arbredesyntaxeabstraite-diapos.pdf
      - https://astexplorer.net/
      - https://keleshev.com/abstract-syntax-tree-an-example-in-c/

N'hésitez pas à rajouter d'autres liens pertinents.


## Conclusion

La création d’un mini moteur regex est un **projet ambitieux** et instructif. En suivant cette roadmap :

1. **Définissez la syntaxe (grammaire)**.  
2. **Implémentez un parseur** qui produit un **AST**.  
3. **Générez un NFA** (ou autre structure similaire) depuis l’AST.  
4. **Écrivez la logique de matching** pour parcourir la chaîne.  
5. **Gérez progressivement les fonctionnalités** : captures, quantificateurs complexes, classes de caractères, etc.  

En avançant étape par étape, vous garderez la maîtrise de la complexité. Vous pourrez ensuite itérer pour améliorer la performance ou ajouter des fonctionnalités avancées. Bon courage !