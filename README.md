# Dette_Prog_Syst
Projet de rattrapage de la matière programmation système S7

Deadline : 10/11/2024

Modalité de rendu :
1. Un dépôt GitHub
2. Un rapport factuel, max 2 pages de texte (ça peut être sous forme d'un email ou dans le README du dépôt), l'objectif, c'est que j'ai une idée de ce qui a été implémenté et des choix qui ont été faits.
3. Une "soutenance" qui aura lieu sous la forme d'un entretien dans mon bureau la semaine du 11/11/2024. Tu prépareras une petite démo pour présenter les fonctionnalités implémentées.
L'objectif de la soutenance, c'est de "vendre" au mieux ce que tu as fait.

Sujet général : remote shell.
Étapes clefs conseillées
1. Écrire un shell simple avec historique des commandes équivalent à ce qui est fait dans la fiche d'exercice sur moodle, "Thème 6 - Système : Gestion des processus" (ne pas oublier >, >>, <<, < et &).
2. Permettre l'utilisation du shell de manière distante (système client serveur). Précision : c'est le client qui exécutera localement les commandes envoyées par le serveur.
3. Améliorer le shell pour le rendre réellement "utilisable" (e.g. gestion des pipes, gestion des variables d'environnement, gestion des signaux).
4. Gestions de plusieurs clients à la fois (multiprocessing et multithreading).

Le code doit avoir été écrit en intégralité par toi, être commenté et être documenté.
Le sujet est volontairement ouvert et laisse la possibilité de me montrer ce que tu sais faire, mais aussi de laisser de côté un élément si jamais il te semble bloquant.

# Comment utiliser le shell :

## Compilation :
make 

## Lancement du shell :
./main shell -> lance le shell en local
./main server <port> -> lance le serveur sur le port spécifié
./main client <port> -> lance le client et se connecte au serveur sur le port spécifié
./main multi_server <port> -> lance le serveur multi-clients sur le port spécifié

# Rapport de Projet

## Étape 1 :

Initialement, il a fallu écrire un shell simple avec un historique des commandes. J'ai commencé par créer un shell de base capable d'exécuter des commandes. J'ai rapidement constaté que la commande `cd` ne fonctionnait pas, j'ai donc implémenté ma propre fonction `cd`. Ensuite, j'ai ajouté une fonctionnalité d'historique des commandes. J'ai utilisé une liste pour stocker les commandes et j'ai créé des fonctions pour ajouter des commandes à l'historique et l'afficher. Puis, j'ai implémenté la gestion des redirections (`>`, `>>`, `<` et &&) et des processus en arrière-plan (`&`). Pour gérer ces fonctionnalités, j'ai utilisé la fonction `dup2` pour rediriger les flux d'entrée et de sortie. J'ai laissé de côté l'implémentation des pipes et des variables d'environnement à ce stade, ainsi que la redirection `<<` (qui semblait trop complexe à l'époque). Spoiler : je l'ai finalement oublié faute de temps (<<). Les fichiers pertinents pour cette étape sont `shell.c`, `shell.h` et `main.c` (utilisé pour lancer le shell).

## Étape 2 :

Une fois que le shell fonctionnait bien, j'ai ajouté la possibilité de l'utiliser à distance. J'ai créé un serveur qui attend qu'un client se connecte et envoie les commandes à exécuter. J'ai utilisé la bibliothèque socket pour la communication entre le client et le serveur. J'ai été confronté à la limitation du nombre de connexions clients, car je souhaitais que cette première version du serveur accepte une seule connexion. Après avoir essayé plusieurs approches, je n'ai réellement pas réussi à limiter le nombre de connexion... au moins le serveur n'envoie la commande qu'au premier client connecté bien que la fermeture de celui-ci ferme bien tous les autres clients. Je n'arrivais pas à limiter le nmobre de clients sans casser le fonctionnement du serveur.
J'ai donc laissé de coté la limitation du nombre de clients pour l'étape suivante.

Pour le client, rien de particulier à signaler ; il se connecte au serveur et attend les commandes à exécuter. Durant cette étape, j'ai également décidé d'implémenter la gestion des signaux (`SIGINT`, `SIGTERM`, et `SIGTSTP`). De plus, j'ai divisé le code en plusieurs fichiers d'en-tête pour améliorer la lisibilité. J'ai créé `server.c`, `server.h`, `client.c`, et `client.h`, puis j'ai écrit un Makefile pour compiler l'ensemble.

## Étape 3 :

Dans cette étape, j'ai décidé d'implémenter un serveur multi-clients pour gérer plusieurs clients simultanément. J'ai créé `multi_server.c` et rencontré des difficultés à gérer les connexions clients de manière efficace. Le problème était que chaque nouveau client devait attendre que le serveur envoie des commandes aux autres clients avant de pouvoir recevoir des commandes lui-même. Ce problème persistait pour chaque nouveau client.

J'ai finalement résolu le problème en utilisant un tableau de sockets et `select()` pour gérer les sockets prêts à lire ou écrire. Pour compléter cette étape, j'ai centralisé le lancement de tous les exécutables via une fonction main unique, permettant des arguments comme `./main shell`, `./main server`, `./main client` et `./main multi_server`.

Neamnoins, j'ai tenté d'implementer un nombre maximum de clients connectés en meme temps mais comme pour le serveur simple, je n'arrvie qu'à limiter le nombre de clients recevant une commande et non pas le nombre de clients connectés. J'ai donc laissé de coté cette fonctionnalité.

## Étape 4 :

Ensuite, j'ai implémenté la gestion des pipes et l'évaluation des variables d'environnement, ce qui s'est avéré moins compliqué que prévu. Après cela, j'ai ajouté un système de port dynamique pour permettre à plusieurs serveurs de fonctionner simultanément (au début, chaque serveur utilisait un port par défaut, le #2580). J'ai ajouté le port comme argument aux commandes comme `./main server <port>` (pareil pour le client et le multi-serveur).

Ensuite, j'ai amélioré la gestion locale des commandes. Initialement, `server` et `multi_server` pouvaient exécuter des commandes localement tant qu'aucun client n'était connecté. J'ai ajouté un argument `-local` pour indiquer qu'une commande ne devait pas être envoyée aux clients. Pendant ce processus, j'ai réalisé qu'avec `multi_server`, la même commande était envoyée à tous les clients, et je ne pouvais pas cibler un client spécifique pour le déconnecter. J'ai donc décidé d'inverser ce comportement uniquement pour `multi_server`. Maintenant, les commandes sont exécutées localement par défaut, même en présence de clients, et il faut ajouter `-all` ou `-id <x>` à la fin d'une commande pour spécifier les clients qui doivent la recevoir. Enfin, j'ai aussi permis au client d'entrer ses porpres commandes en local (non-envoyées au serveur).

## Étape 5 :

Enfin, j'ai documenté et commenté le code. J'ai ajouté des commentaires à chaque fonction et partie du code pour rendre le tout aussi clair que possible, puis j'ai écrit ce rapport.

## Remarques générales :

J'ai eu beaucoup d'idées que je n'ai pas pu mettre en oeuvre faute de temps ou de compétences, comme l'auto-complétion (vraiment complexe), un historique des commandes navigable via les flèches du clavier ou encore la possiblité de naviguer entre les caractères d'une ligne etc etc. De plus, de nouvelles idées surgissaient lors de chaque étape, ce qui rendait difficile de se concentrer sur l'essentiel. Je pense que j'ai perdu pas mal de temps à cause de cela, même si j'ai fait de mon mieux pour m'organiser.

J'ai tenté d'utiliser le code de M. Lienhardt au tout début, mais je l'ai trouvé difficile à comprendre et illogique, probablement (assurément même) en raison de mon manque de compétences. J'ai donc décidé de tout reprendre à zéro en utilisant les connaissances que j'avais acquises lors de ses TD, parcourant votre cours et le sien, ainsi que de la documentation (YouTube, Reddit, StackOverflow, le manuel Unix, et divers blogs).

J'ai aussi passé beaucoup de temps à déboguer et chercher des solutions complexes à des problèmes simples. J'ai parfois eu du mal à prendre du recul sur mon code et à voir des erreurs évidentes. Bien sûr, j'ai perdu du temps à tester et à réparer les problèmes causés par les nouvelles fonctionnalités.

En parlant de débogage, j'ai bien sûr utilisé l'IA pour m'aider lorsque je n'arrivais pas à avancer. Cependant, l'IA reste assez aléatoire dans ses réponses et est parfois complètement à côté de la plaque. Elle donne de bonnes idées, mais pas toujours la bonne façon de les mettre en oeuvre. Je n'ai pas utilisé ce que je ne comprenais pas et j'ai même donné mes solutions qui fonctionnaient à ChatGPT (j'imagine qu'il apprend de ce qu'on lui dit). Il faut savoir poser les bonnes questions pour obtenir les bonnes réponses.
Je lui ai également fait ecrire les commentaires et la documentation en repassant dessus si je constatais un manque de pertinence.

GitHub m'a également été très utile pour retrouver mon code fonctionnel après l'avoir cassé avec des modifications.
