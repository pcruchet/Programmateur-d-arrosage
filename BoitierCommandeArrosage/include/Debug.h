/**
 * @file    Debug.h
 * @brief   Macros de trace série partagées par l'ensemble du firmware.
 *
 * @details Centralise l'activation/désactivation des traces de debug pour
 *          tous les fichiers du projet (src/ et lib/), qui incluent ce
 *          fichier plutôt que de redéfinir localement leurs propres macros.
 *          Commenter #DEBUG_SERIAL désactive globalement toutes les traces
 *          (les macros deviennent des no-op), ce qui réduit la taille du
 *          binaire et le temps d'exécution en production.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

/// Active les macros DEBUG/DEBUG_VAL. À commenter pour les désactiver
/// globalement (macros vides) en version de production.
#define DEBUG_SERIAL   // ← commenter pour desactiver en production

#ifdef DEBUG_SERIAL
    /**
     * @def DEBUG(_msg)
     * @brief Affiche un message de trace sur le port série, suivi d'un
     *        retour à la ligne.
     * @param _msg Message ou expression affichable via Serial.println().
     */
    #define DEBUG(_msg)            Serial.println(_msg)

    /**
     * @def DEBUG_VAL(_msg, _val)
     * @brief Affiche un message de trace suivi d'une valeur, sur la même
     *        ligne du port série (utile pour tracer "libellé : valeur").
     * @param _msg Libellé affiché en premier (sans retour à la ligne).
     * @param _val Valeur affichée à la suite, suivie d'un retour à la ligne.
     */
    #define DEBUG_VAL(_msg, _val)  do { Serial.print(_msg); Serial.println(_val); } while (0)
#else
    /// Version désactivée de DEBUG() : ne produit aucun code (macro vide).
    #define DEBUG(_msg)
    /// Version désactivée de DEBUG_VAL() : ne produit aucun code (macro vide).
    #define DEBUG_VAL(_msg, _val)
#endif

#endif // DEBUG_H
