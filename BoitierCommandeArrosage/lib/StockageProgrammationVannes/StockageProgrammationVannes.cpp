/**
 * @file    StockageProgrammationVannes.cpp
 * @brief   Implémentation de la persistance NVS de la programmation des 4
 *          vannes.
 */


#include "StockageProgrammationVannes.h"

#define NAMESPACE_NVS "arrosage"

/// Constructeur par défaut, sans état à initialiser.
StockageProgrammationVannes::StockageProgrammationVannes()
{
}

// ============================================================
// Initialisation
// ============================================================

/**
 * @brief Vérifie l'accès à l'espace de noms NVS "arrosage".
 * @return true si l'ouverture/fermeture de l'espace de noms a réussi.
 */
bool StockageProgrammationVannes::initialiser()
{
    bool resultat = false;

    if (preferences.begin(NAMESPACE_NVS, false))
    {
        preferences.end();
        resultat = true;
    }

    return resultat;
}

// ============================================================
// Programmation complète
// ============================================================

/**
 * @brief Écrit les 4 champs de la programmation d'une vanne en une seule
 *        ouverture de l'espace de noms NVS.
 * @details Sans effet si _idVanne est invalide ou si l'ouverture de
 *          l'espace de noms en écriture échoue.
 */
void StockageProgrammationVannes::ecrireProgrammation(uint8_t _idVanne,
                                                       const ProgrammationVanne &_programmation)
{
    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, false))
        {
            preferences.putChar(cleMode(_idVanne).c_str(), _programmation.mode);
            preferences.putString(cleHeure(_idVanne).c_str(), _programmation.heure);
            preferences.putInt(cleDuree(_idVanne).c_str(), _programmation.duree);
            preferences.putInt(cleFrequence(_idVanne).c_str(), _programmation.frequence);
            preferences.end();
        }
    }
}

/**
 * @brief Relit les 4 champs de la programmation d'une vanne.
 * @details Initialise le résultat aux valeurs par défaut avant lecture ;
 *          celles-ci restent donc utilisées si l'identifiant est invalide,
 *          si l'ouverture de l'espace de noms échoue, ou si une clé n'a
 *          jamais été écrite.
 */
ProgrammationVanne StockageProgrammationVannes::lireProgrammation(uint8_t _idVanne) const
{
    ProgrammationVanne programmation;
    programmation.mode      = MODE_DEFAUT;
    programmation.heure     = "";
    programmation.duree     = DUREE_DEFAUT;
    programmation.frequence = FREQUENCE_DEFAUT;

    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, true))
        {
            programmation.mode      = preferences.getChar(cleMode(_idVanne).c_str(), MODE_DEFAUT);
            programmation.heure     = preferences.getString(cleHeure(_idVanne).c_str(), "");
            programmation.duree     = preferences.getInt(cleDuree(_idVanne).c_str(), DUREE_DEFAUT);
            programmation.frequence = preferences.getInt(cleFrequence(_idVanne).c_str(), FREQUENCE_DEFAUT);
            preferences.end();
        }
    }

    return programmation;
}

// ============================================================
// Champ ciblé : mode
// ============================================================

/**
 * @brief Écrit uniquement le champ mode d'une vanne (heure/durée/fréquence
 *        non modifiées).
 */
void StockageProgrammationVannes::ecrireMode(uint8_t _idVanne, char _mode)
{
    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, false))
        {
            preferences.putChar(cleMode(_idVanne).c_str(), _mode);
            preferences.end();
        }
    }
}

/**
 * @brief Relit uniquement le champ mode d'une vanne.
 * @return Mode persisté, ou MODE_DEFAUT par défaut.
 */
char StockageProgrammationVannes::lireMode(uint8_t _idVanne) const
{
    char mode = MODE_DEFAUT;

    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, true))
        {
            mode = preferences.getChar(cleMode(_idVanne).c_str(), MODE_DEFAUT);
            preferences.end();
        }
    }

    return mode;
}

// ============================================================
// Réinitialisation
// ============================================================

/**
 * @brief Réécrit la programmation d'une vanne avec les valeurs par défaut
 *        (mode Manuel, heure vide, durée et fréquence nulles).
 * @details Construit une ProgrammationVanne "vierge" et délègue à
 *          ecrireProgrammation().
 */
void StockageProgrammationVannes::reinitialiserVanne(uint8_t _idVanne)
{
    if (idVanneValide(_idVanne))
    {
        ProgrammationVanne programmationVierge;
        programmationVierge.mode      = MODE_DEFAUT;
        programmationVierge.heure     = "";
        programmationVierge.duree     = DUREE_DEFAUT;
        programmationVierge.frequence = FREQUENCE_DEFAUT;

        ecrireProgrammation(_idVanne, programmationVierge);
    }
}

/// Applique reinitialiserVanne() aux identifiants 1 à NB_VANNES_MAX.
void StockageProgrammationVannes::reinitialiserTout()
{
    uint8_t idVanne = 1;

    while (idVanne <= NB_VANNES_MAX)
    {
        reinitialiserVanne(idVanne);
        idVanne++;
    }
}

// ============================================================
// Privé : construction des clés NVS
// ============================================================

/// Construit la clé NVS "v{idVanne}_mode".
String StockageProgrammationVannes::cleMode(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_mode";

    return cle;
}

/// Construit la clé NVS "v{idVanne}_heure".
String StockageProgrammationVannes::cleHeure(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_heure";

    return cle;
}

/// Construit la clé NVS "v{idVanne}_duree".
String StockageProgrammationVannes::cleDuree(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_duree";

    return cle;
}

/// Construit la clé NVS "v{idVanne}_freq".
String StockageProgrammationVannes::cleFrequence(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_freq";

    return cle;
}

// ============================================================
// Privé : validation
// ============================================================

/// Valide que _idVanne est compris entre 1 et NB_VANNES_MAX.
bool StockageProgrammationVannes::idVanneValide(uint8_t _idVanne) const
{
    bool valide = (_idVanne >= 1 && _idVanne <= NB_VANNES_MAX);

    return valide;
}