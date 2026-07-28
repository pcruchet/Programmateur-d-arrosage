/**
 * @file    main.cpp
 * @brief   Point d'entrée Arduino du firmware (setup()/loop()).
 *
 * @details Instancie l'automate BoitierPilotageArrosage, l'initialise, puis
 *          délègue tout le fonctionnement à son automate d'états à chaque
 *          tour de loop(). Ce fichier est exclu de la compilation lors des
 *          tests unitaires PlatformIO (macro UNIT_TEST définie par
 *          l'environnement de test), qui fournissent leur propre point
 *          d'entrée.
 */

// src/main.cpp

#include <Arduino.h>
#include "BoitierPilotageArrosage.h"

#ifndef UNIT_TEST
#include "Debug.h"

/// Instance unique de l'automate principal, allouée dans setup() et pilotée
/// à chaque tour de loop().
BoitierPilotageArrosage *boitier = nullptr;

/**
 * @brief Point d'entrée Arduino exécuté une seule fois au démarrage.
 *
 * @details Initialise le port série (avec un court délai en mode debug pour
 *          laisser le temps de rouvrir le moniteur série après un
 *          rebranchement USB), instancie et initialise le boîtier de
 *          pilotage. En cas d'échec d'initialisation, bloque définitivement
 *          l'exécution (boucle infinie) après avoir signalé l'erreur sur le
 *          port série, plutôt que de laisser tourner un automate dans un
 *          état matériel non garanti.
 */
void setup()
{
    Serial.begin(115200);

    #ifdef DEBUG_SERIAL
        delay(3000); // laisse le temps de rouvrir le moniteur serie apres rebranchement USB
    #endif
    
    boitier = new BoitierPilotageArrosage();

    bool initialisationReussie = boitier->initialiser();

    if (!initialisationReussie)
    {
        Serial.println("ERREUR CRITIQUE : initialisation du Boitier echouee");

        while (true)
            delay(1000);
    }
}

/**
 * @brief Boucle principale Arduino, exécutée en continu.
 *
 * @details Délègue chaque itération à
 *          BoitierPilotageArrosage::controler(), qui exécute un pas de
 *          l'automate d'états correspondant à l'état courant du boîtier.
 */
void loop()
{
    boitier->controler();
}

#endif
