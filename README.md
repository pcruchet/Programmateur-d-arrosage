# 🌿 TW2 — Programmateur d'arrosage autonome solaire

> Projet BTS CIEL IR — Épreuve E6 — Lycée Touchard-Washington, Le Mans  
> Année scolaire 2024-2025

---

## Présentation générale

TW2 est un système d'arrosage automatique **autonome en énergie**, piloté à distance via une application Android. Il est conçu pour fonctionner sans intervention humaine : le panneau solaire charge la batterie, l'ESP32 gère les vannes selon des plages horaires programmées, et le téléphone sert de télécommande en Wi-Fi.

```
┌─────────────────────────────────────────────────────────────────┐
│  Panneau solaire 100 W                                          │
│       ↓                                                         │
│  Batterie 3S 18650  →  LM2596 (12 V)  →  Vannes bistables x4    │
│                     →  LM2596 (3.3 V) →  ESP32 LOLIN32          │
│                                            ├── DS3231 (RTC)     │
│                                            ├── SSD1306 (OLED)   │
│                                            └── WebSocket Wi-Fi  │
│                                                     ↕           │
│                                         Application Qt/Android  │
└─────────────────────────────────────────────────────────────────┘
```

Le système fonctionne **sans cloud** : l'ESP32 crée son propre point d'accès Wi-Fi (mode `WIFI_MODE_APSTA`) et communique directement avec l'application via WebSocket.

---

## 🔗 Liens rapides

| Ressource | Lien |
|---|---|
| 📁 Dépôt GitHub | [github.com/pcruchet/Programmateur-d-arrosage](https://github.com/pcruchet/Programmateur-d-arrosage) |
| 📖 Documentation Doxygen | [pcruchet.github.io/Programmateur-d-arrosage](https://pcruchet.github.io/Programmateur-d-arrosage/) |
| 🔧 Firmware ESP32 | [BoitierCommandeArrosage/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage) |
| 📱 Application Qt/Android | [ControleurArrosage/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/ControleurArrosage) |

---

## Architecture du dépôt

```
Programmateur-d-arrosage/
├── BoitierCommandeArrosage/        # Firmware ESP32 (PlatformIO / Arduino)
│   ├── include/                    # En-têtes globaux
│   ├── src/                        # Point d'entrée et automate principal
│   ├── lib/                        # Bibliothèques internes (une lib = une classe)
│   │   ├── Vanne/
│   │   ├── GestionnaireTemps/
│   │   ├── AfficheurOLED/
│   │   ├── ServeurWebSocket/
│   │   ├── ProtocoleArrosageServeur/
│   │   ├── StockageProgrammationVannes/
│   │   ├── StockageEtatVannes/
│   │   └── MesureBatteries/
│   ├── test/                       # Tests unitaires Unity (PlatformIO)
│   └── platformio.ini
│
└── ControleurArrosage/             # Application Qt/QML Android
    ├── *.h / *.cpp                 # Classes C++
    ├── *.qml                       # Pages et composants QML
    └── *.js                        # Modules JS (.pragma library)
```

---

## Matériel utilisé

| Composant | Référence | Rôle |
|---|---|---|
| Microcontrôleur | ESP32 LOLIN32 | Cerveau du système |
| Vannes | Antelco eZyvalve4 (bistables) | Ouverture/fermeture arrosage |
| Horloge temps réel | DS3231 | Gestion des plages horaires |
| Afficheur | SSD1306 128×64 OLED | Statut local du boîtier |
| Batterie | Pack 3S 18650 | Stockage énergie solaire |
| Panneau solaire | 100 W | Source d'énergie principale |
| Régulateurs | LM2596 (×2) | 12 V vannes / 3,3 V ESP32 |

---

## Périmètres des étudiants (épreuve E6)

Le projet est réparti entre **quatre étudiants**, chacun responsable d'un périmètre technique distinct.

---

### 👤 Étudiant 1 — Firmware ESP32 : automate et pilotage des vannes

**Périmètre :** logique de l'automate principal, pilotage des vannes bistables, gestion du temps RTC, stockage NVS, affichage OLED.

| Classe | Fichiers | Rôle |
|---|---|---|
| `BoitierPilotageArrosage` | [include/](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/BoitierCommandeArrosage/include/BoitierPilotageArrosage.h) · [src/](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/BoitierCommandeArrosage/src/BoitierPilotageArrosage.cpp) | Automate principal (états : DEMARRAGE → ARROSAGE → LIGHT_SLEEP → ENDORMISSEMENT) |
| `Vanne` | [lib/Vanne/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/Vanne) | Pilotage d'une vanne bistable (impulsions INA/INB) |
| `GestionnaireTemps` | [lib/GestionnaireTemps/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/GestionnaireTemps) | Interface DS3231 : lecture heure, programmation alarme, réveil deep sleep |
| `StockageProgrammationVannes` | [lib/StockageProgrammationVannes/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/StockageProgrammationVannes) | Persistance NVS des programmations (mode, heure, durée, fréquence) |
| `StockageEtatVannes` | [lib/StockageEtatVannes/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/StockageEtatVannes) | Persistance NVS de l'état ouvert/fermé (survit au deep sleep) |
| `AfficheurOLED` | [lib/AfficheurOLED/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/AfficheurOLED) | Affichage SSD1306 : IP, état vannes, heure, statut client |

**Points techniques notables :**
- Automate à états (`enum class EtatBoitier`) sans bibliothèque externe
- Vannes bistables : une impulsion courte suffit à commuter, sans maintien du courant
- Deep sleep entre les arrosages → réveil sur alarme RTC (DS3231 GPIO27) ou bouton poussoir (GPIO36)
- NVS (Non-Volatile Storage) : les états survivent aux coupures d'alimentation

---

### 👤 Étudiant 2 — Application Qt/Android : interface QML

**Périmètre :** application mobile Qt, modèle de données C++, pages QML, persistance locale.

| Classe / Fichier | Liens | Rôle |
|---|---|---|
| `ControleurArrosage` | [.h](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/controleurarrosage.h) · [.cpp](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/controleurarrosage.cpp) | Orchestrateur central : `QAbstractListModel`, actions QML, parsing des réponses ESP32 |
| `Vanne` (Qt) | [.h](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/vanne.h) · [.cpp](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/vanne.cpp) | Miroir Qt de l'état d'une vanne (état, mode, programmation) |
| `PagePrincipale.qml` | [lien](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/PagePrincipale.qml) | Liste des vannes avec badges de mode et état |
| `PageProgrammation.qml` | [lien](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/PageProgrammation.qml) | Saisie heure de début, durée, fréquence |
| `PageConnexion.qml` | [lien](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/PageConnexion.qml) | Configuration IP et port, connexion WebSocket |
| `Principale.js` | [lien](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/Principale.js) | Helpers UI (`.pragma library`) : couleurs, labels selon mode/état |
| `Constantes.qml` | [lien](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/Constantes.qml) | Singleton de design tokens (couleurs, tailles, rayons) |

**Points techniques notables :**
- `ControleurArrosage` hérite de `QAbstractListModel` → exposition directe au moteur QML sans `QML_ELEMENT` supplémentaire
- Mise à jour **optimiste** de l'UI : l'interface réagit immédiatement, l'ESP32 confirme ensuite
- `QSettings` pour la persistance de l'adresse IP et du port entre les sessions
- Modules JS avec `.pragma library` : partagés entre pages, mais sans accès aux globals QML → format de date explicite

---

### 👤 Étudiant 3 — Communication WebSocket et protocole applicatif

**Périmètre :** couche WebSocket des deux côtés, protocole JSON, format des trames.

| Classe | Côté | Liens | Rôle |
|---|---|---|---|
| `ServeurWebSocket` | ESP32 | [lib/ServeurWebSocket/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/ServeurWebSocket) | Serveur WebSocket asynchrone (ESPAsyncWebServer), gestion des clients |
| `ProtocoleArrosageServeur` | ESP32 | [lib/ProtocoleArrosageServeur/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/ProtocoleArrosageServeur) | Décodage des requêtes JSON entrantes, encodage des réponses/notifications |
| `CommunicationESP32` | Qt | [.h](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/communicationesp32.h) · [.cpp](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/communicationesp32.cpp) | Client WebSocket Qt (`QWebSocket`), signaux `connected` / `disconnected` / `messageReceived` |
| `ProtocoleArrosageClient` | Qt | [.h](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/protocolarrosage.h) · [.cpp](https://github.com/pcruchet/Programmateur-d-arrosage/blob/main/ControleurArrosage/protocolarrosage.cpp) | Encodage des requêtes JSON sortantes, décodage des réponses ESP32 |

**Format d'une trame JSON :**

```json
{ "v": 1, "t": "<type>", "c": "<commande>", ... }
```

| Champ | Valeurs possibles |
|---|---|
| `v` | Version du protocole (1) |
| `t` | `q` query · `r` réponse · `c` commande · `a` ack · `e` erreur · `n` notification |
| `c` | `S` système · `T`/`t` heure · `E` état · `M`/`m` mode · `P`/`p` programmation · `O` ouvrir · `F` fermer · `G` ping |

**Points techniques notables :**
- Constantes de protocole en `static const char` (pas `QString`) → compatibilité `switch/case` et symétrie ESP32/Qt
- Les notifications spontanées (`t="n"`) et les réponses (`t="r"`) ont le même format de données → traitement unifié côté Qt
- Tests unitaires Unity couvrant `ProtocoleArrosageServeur` : [test/test_ProtocoleArrosageServeur/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/test/test_ProtocoleArrosageServeur)

---

### 👤 Étudiant 4 — Gestion de l'énergie

**Périmètre :** mesure de batterie, modes de sommeil ESP32, alimentation de la carte puissance.

| Classe / Composant | Liens | Rôle |
|---|---|---|
| `MesureBatteries` | [lib/MesureBatteries/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/MesureBatteries) | Lecture tension batterie par ADC, calcul niveau de charge |
| `BoutonPoussoir` | [lib/BoutonPoussoir/](https://github.com/pcruchet/Programmateur-d-arrosage/tree/main/BoitierCommandeArrosage/lib/BoutonPoussoir) | Détection appui sur GPIO36, source de réveil ext1 |
| Carte puissance | — | LM2596 × 2 (12 V / 3,3 V), MOSFET Q1 (IRF9530) coupure vannes |

**Modes de sommeil implémentés :**

| Mode | Consommation | Conditions de réveil |
|---|---|---|
| **Light sleep** | ~1 mA | Timer 30 s (vérification vannes) ou BP1 |
| **Deep sleep** | ~10 µA | Alarme RTC DS3231 (GPIO27) ou BP1 (GPIO36) |

**Points techniques notables :**
- Le MOSFET Q1 (IRF9530) est câblé en dur pour couper l'alimentation 12 V des vannes lors du deep sleep (protection hors-gel)
- La protection hors-gel se traduit par un arrêt complet du système (pas d'état intermédiaire)
- DS3231 alimenté en permanence depuis le régulateur 3,3 V (pile de secours inutilisée en déploiement)
- GPIO19 ≠ GPIO RTC → le réveil `ext0` requiert GPIO27 (pont fil nécessaire sur le PCB)

---

## Outils et environnement

| Outil | Usage |
|---|---|
| [PlatformIO](https://platformio.org/) | Build, upload, tests Unity (firmware ESP32) |
| [Qt Creator](https://www.qt.io/product/development-tools) | IDE application Qt/QML/Android |
| [Modelio](https://www.modelio.org/) | Diagrammes UML (classes, séquences, activités) |
| [draw.io](https://app.diagrams.net/) | Schéma d'architecture OSI |
| [Doxygen](https://www.doxygen.nl/) + [doxyqml](https://github.com/agateau/doxyqml) | Génération de la documentation (commentaires dans `.h` uniquement) |
| Node.js `docx` | Génération des dossiers techniques `.docx` |

### Dépendances firmware (PlatformIO)

```ini
lib_deps =
    esp32async/ESPAsyncWebServer@^3.11.1
    adafruit/Adafruit SSD1306@^2.5.17
    adafruit/RTClib@^2.1.4
```

### Lancer les tests unitaires

```bash
# Depuis le répertoire BoitierCommandeArrosage/
pio test -e lolin32_test
```

---

## Conventions de code

| Règle | Exemple |
|---|---|
| Nom de classe | PascalCase : `GestionnaireTemps` |
| Attributs | sans préfixe : `disponible`, `socket` |
| Paramètres | préfixe `_` : `_idVanne`, `_message` |
| Un seul `return` par méthode | variable résultat intermédiaire |
| Doxygen | uniquement dans les `.h` |
| Commits | Conventional Commits : `feat:`, `fix:`, `docs:` |

---

## Documentation générée

La documentation Doxygen complète est publiée automatiquement sur GitHub Pages :

**👉 [pcruchet.github.io/Programmateur-d-arrosage](https://pcruchet.github.io/Programmateur-d-arrosage/)**

Elle couvre le firmware ESP32 et l'application Qt (via doxyqml pour les fichiers QML).

---

## Licence

Projet pédagogique — Lycée Touchard-Washington, Le Mans.  
Encadrant : Philippe Cruchet (`@pcruchet`).
