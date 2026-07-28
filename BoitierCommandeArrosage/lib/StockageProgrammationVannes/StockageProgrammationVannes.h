/**
 * @file    StockageProgrammationVannes.h
 * @brief   Persistance NVS de la programmation (mode, heure, durée,
 *          fréquence) de chacune des 4 vannes.
 *
 * @details Distinct de StockageEtatVannes (qui persiste l'état physique
 *          ouverte/fermée) : cette classe conserve la configuration
 *          d'arrosage définie par l'utilisateur via l'application, relue
 *          par l'automate pour déclencher les ouvertures/fermetures
 *          programmées (voir BoitierPilotageArrosage::verifierProgrammations()).
 */

// lib/StockageProgrammationVannes/StockageProgrammationVannes.h

#ifndef STOCKAGE_PROGRAMMATION_VANNES_H
#define STOCKAGE_PROGRAMMATION_VANNES_H

#include <Arduino.h>
#include <Preferences.h>
#include "Debug.h"

// ── Structure complète de la programmation d'une vanne ────────────────────────

/**
 * @struct ProgrammationVanne
 * @brief  Programmation complète persistée pour une vanne.
 */
struct ProgrammationVanne
{
    char   mode;        ///< Mode de la vanne : 'M' (Manuel), 'P' (Programme) ou 'A' (Automatique).
    String heure;        ///< Heure de début d'arrosage programmée, au format ISO8601.
    int    duree;        ///< Durée d'arrosage programmée, en minutes.
    int    frequence;    ///< Fréquence de répétition de la programmation, en heures.
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class StockageProgrammationVannes
 * @brief Persistance NVS (Preferences) de la programmation des 4 vannes.
 */
class StockageProgrammationVannes
{
public:
    static const uint8_t NB_VANNES_MAX = 4; ///< Nombre maximal de vannes gérées.

    static const char    MODE_DEFAUT      = 'M'; ///< Mode par défaut lorsqu'aucune valeur n'est encore persistée (Manuel).
    static const int     DUREE_DEFAUT     = 0;    ///< Durée par défaut lorsqu'aucune valeur n'est encore persistée.
    static const int     FREQUENCE_DEFAUT = 0;    ///< Fréquence par défaut lorsqu'aucune valeur n'est encore persistée.

    /// Construit l'objet sans encore ouvrir l'espace de noms NVS.
    StockageProgrammationVannes();

    // ── Initialisation ────────────────────────────────────────────────────────

    /**
     * @brief Vérifie que l'espace de noms NVS dédié est accessible.
     * @return true si l'ouverture/fermeture de l'espace de noms a réussi.
     */
    bool initialiser();

    // ── Programmation complète (utilisé pour la trame S et set_prog) ──────────

    /**
     * @brief Écrit la programmation complète d'une vanne en NVS.
     * @details Écrit les 4 champs (mode, heure, durée, fréquence) en une
     *          seule ouverture de l'espace de noms. Sans effet si
     *          _idVanne est invalide.
     * @param _idVanne       Identifiant de la vanne (1 à 4).
     * @param _programmation Programmation complète à persister.
     */
    void ecrireProgrammation(uint8_t _idVanne, const ProgrammationVanne &_programmation);

    /**
     * @brief Relit la programmation complète d'une vanne.
     * @param _idVanne Identifiant de la vanne (1 à 4).
     * @return Programmation lue ; valeurs par défaut (MODE_DEFAUT,
     *         heure vide, DUREE_DEFAUT, FREQUENCE_DEFAUT) si l'identifiant
     *         est invalide ou si aucune valeur n'est encore persistée.
     */
    ProgrammationVanne lireProgrammation(uint8_t _idVanne) const;

    // ── Champ ciblé : mode (utilisé pour set_mode) ─────────────────────────────

    /**
     * @brief Écrit uniquement le champ mode d'une vanne, sans toucher aux
     *        autres champs de sa programmation (heure/durée/fréquence
     *        conservées).
     * @param _idVanne Identifiant de la vanne (1 à 4).
     * @param _mode    Nouveau mode ('M', 'P' ou 'A').
     */
    void ecrireMode(uint8_t _idVanne, char _mode);

    /**
     * @brief Relit uniquement le champ mode d'une vanne.
     * @param _idVanne Identifiant de la vanne (1 à 4).
     * @return Mode persisté, ou MODE_DEFAUT si l'identifiant est invalide
     *         ou si aucune valeur n'est encore persistée.
     */
    char lireMode(uint8_t _idVanne) const;

    // ── Réinitialisation ──────────────────────────────────────────────────────

    /**
     * @brief Réinitialise la programmation d'une vanne à ses valeurs par
     *        défaut (mode Manuel, heure vide, durée et fréquence nulles).
     * @param _idVanne Identifiant de la vanne (1 à 4).
     */
    void reinitialiserVanne(uint8_t _idVanne);

    /// Réinitialise la programmation des 4 vannes à leurs valeurs par
    /// défaut.
    void reinitialiserTout();

private:
    mutable Preferences preferences; ///< Instance d'accès à l'espace de stockage NVS (mutable pour permettre des lectures depuis des méthodes const).

    // ── Construction des clés NVS ────────────────────────────────────────────

    /// Construit la clé NVS "v{idVanne}_mode".
    String cleMode(uint8_t _idVanne) const;
    /// Construit la clé NVS "v{idVanne}_heure".
    String cleHeure(uint8_t _idVanne) const;
    /// Construit la clé NVS "v{idVanne}_duree".
    String cleDuree(uint8_t _idVanne) const;
    /// Construit la clé NVS "v{idVanne}_freq".
    String cleFrequence(uint8_t _idVanne) const;

    /**
     * @brief Valide qu'un identifiant de vanne est dans la plage gérée.
     * @param _idVanne Identifiant à valider.
     * @return true si _idVanne est compris entre 1 et NB_VANNES_MAX.
     */
    bool idVanneValide(uint8_t _idVanne) const;
};

#endif // STOCKAGE_PROGRAMMATION_VANNES_H
