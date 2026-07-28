/**
 * @file    StockageEtatVannes.cpp
 * @brief   Implémentation de la persistance NVS de l'état physique des 4
 *          vannes.
 */

// lib/StockageEtatVannes/StockageEtatVannes.cpp

#include "StockageEtatVannes.h"

#define NAMESPACE_NVS "etat_vanne"

/// Constructeur par défaut, sans état à initialiser.
StockageEtatVannes::StockageEtatVannes()
{
}

// ============================================================
// Initialisation
// ============================================================

/**
 * @brief Vérifie l'accès à l'espace de noms NVS "etat_vanne".
 * @return true si l'ouverture/fermeture de l'espace de noms a réussi.
 */
bool StockageEtatVannes::initialiser()
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
// Écriture
// ============================================================

/**
 * @brief Écrit l'état complet (ouverte/fermée + horodatage) d'une vanne en
 *        NVS.
 * @details Sans effet si _idVanne est invalide ou si l'ouverture de
 *          l'espace de noms en écriture échoue.
 */
void StockageEtatVannes::sauvegarderEtat(uint8_t _idVanne, const EtatVanne &_etat)
{
    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, false))
        {
            preferences.putBool(cleOuverte(_idVanne).c_str(), _etat.ouverte);
            preferences.putString(cleHeure(_idVanne).c_str(), _etat.heureOuverture);
            preferences.end();
        }
    }
}

/**
 * @brief Persiste l'ouverture d'une vanne à l'heure donnée.
 * @details Construit un EtatVanne{ouverte=true, heureOuverture=_heureISO}
 *          et délègue à sauvegarderEtat().
 */
void StockageEtatVannes::sauvegarderOuverture(uint8_t _idVanne, const String &_heureISO)
{
    if (idVanneValide(_idVanne))
    {
        EtatVanne etat;
        etat.ouverte        = true;
        etat.heureOuverture = _heureISO;

        sauvegarderEtat(_idVanne, etat);
    }
}

/**
 * @brief Persiste la fermeture d'une vanne.
 * @details Construit un EtatVanne{ouverte=false, heureOuverture=""} et
 *          délègue à sauvegarderEtat().
 */
void StockageEtatVannes::sauvegarderFermeture(uint8_t _idVanne)
{
    if (idVanneValide(_idVanne))
    {
        EtatVanne etat;
        etat.ouverte        = false;
        etat.heureOuverture = "";

        sauvegarderEtat(_idVanne, etat);
    }
}

// ============================================================
// Lecture
// ============================================================

/**
 * @brief Relit l'état persisté d'une vanne.
 * @details Retourne l'état par défaut (fermée, heure vide) si l'identifiant
 *          est invalide ou si l'espace de noms ne peut pas être ouvert en
 *          lecture.
 */
EtatVanne StockageEtatVannes::lireEtat(uint8_t _idVanne) const
{
    EtatVanne etat;
    etat.ouverte        = false;
    etat.heureOuverture = "";

    if (idVanneValide(_idVanne))
    {
        if (preferences.begin(NAMESPACE_NVS, true))
        {
            etat.ouverte        = preferences.getBool(cleOuverte(_idVanne).c_str(), false);
            etat.heureOuverture = preferences.getString(cleHeure(_idVanne).c_str(), "");
            preferences.end();
        }
    }

    return etat;
}

/**
 * @brief Indique si une vanne est ouverte d'après l'état persisté.
 * @details Relit l'état complet via lireEtat() et retourne son champ
 *          ouverte.
 */
bool StockageEtatVannes::vanneOuverte(uint8_t _idVanne) const
{
    bool resultat = false;

    if (idVanneValide(_idVanne))
    {
        EtatVanne etat = lireEtat(_idVanne);
        resultat       = etat.ouverte;
    }

    return resultat;
}

/**
 * @brief Indique si au moins une des 4 vannes est ouverte.
 * @details Parcourt les identifiants 1 à NB_VANNES_MAX et s'arrête dès
 *          qu'une vanne ouverte est trouvée.
 */
bool StockageEtatVannes::uneVanneOuverte() const
{
    bool resultat = false;

    uint8_t idVanne = 1;
    while (idVanne <= NB_VANNES_MAX && !resultat)
    {
        resultat = vanneOuverte(idVanne);
        idVanne++;
    }

    return resultat;
}

// ============================================================
// Timeout de sécurité
// ============================================================

/**
 * @brief Détermine si la durée d'ouverture d'une vanne dépasse la durée
 *        programmée majorée de la marge de sécurité.
 * @details Ne s'applique que si la vanne est ouverte et qu'un horodatage
 *          d'ouverture est disponible ; calcule l'écart via diffMinutes()
 *          et le compare à _dureeMinutes + MARGE_SECURITE_MS converti en
 *          minutes.
 */
bool StockageEtatVannes::delaiDepasse(uint8_t _idVanne,
                                      int _dureeMinutes,
                                      const String &_heureActuelleISO) const
{
    bool resultat = false;

    if (idVanneValide(_idVanne))
    {
        EtatVanne etat = lireEtat(_idVanne);

        if (etat.ouverte && etat.heureOuverture.length() > 0)
        {
            int ecartMinutes = diffMinutes(etat.heureOuverture, _heureActuelleISO);
            int limiteMinutes = _dureeMinutes + (MARGE_SECURITE_MS / 60000);

            if (ecartMinutes >= limiteMinutes)
                resultat = true;
        }
    }

    return resultat;
}

// ============================================================
// Réinitialisation
// ============================================================

/**
 * @brief Réinitialise une vanne à l'état "fermée" en NVS.
 * @details Délègue à sauvegarderFermeture() après validation de
 *          l'identifiant.
 */
void StockageEtatVannes::reinitialiserVanne(uint8_t _idVanne)
{
    if (idVanneValide(_idVanne))
        sauvegarderFermeture(_idVanne);
}

/// Applique reinitialiserVanne() aux identifiants 1 à NB_VANNES_MAX.
void StockageEtatVannes::reinitialiserTout()
{
    uint8_t idVanne = 1;

    while (idVanne <= NB_VANNES_MAX)
    {
        reinitialiserVanne(idVanne);
        idVanne++;
    }
}

// ============================================================
// Privé : clés NVS
// ============================================================

/// Construit la clé NVS "v{idVanne}_ouv".
String StockageEtatVannes::cleOuverte(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_ouv";

    return cle;
}

/// Construit la clé NVS "v{idVanne}_heure".
String StockageEtatVannes::cleHeure(uint8_t _idVanne) const
{
    String cle = "v" + String(_idVanne) + "_heure";

    return cle;
}

// ============================================================
// Privé : validation
// ============================================================

/// Valide que _idVanne est compris entre 1 et NB_VANNES_MAX.
bool StockageEtatVannes::idVanneValide(uint8_t _idVanne) const
{
    bool valide = (_idVanne >= 1 && _idVanne <= NB_VANNES_MAX);

    return valide;
}

// ============================================================
// Privé : calcul écart en minutes entre deux ISO8601
// ============================================================

/**
 * @brief Calcule l'écart en minutes entre deux dates ISO8601.
 * @details Approximation calendaire simplifiée (365 jours/an, 30 jours/
 *          mois, cf. commentaires du code) suffisante pour des écarts de
 *          l'ordre de la durée d'un arrosage. Retourne 0 si l'une des deux
 *          chaînes n'a pas exactement 19 caractères, ou si le résultat brut
 *          serait négatif.
 */
int StockageEtatVannes::diffMinutes(const String &_heureDebutISO,
                                    const String &_heureFinISO) const
{
    int resultat = 0;

    if (_heureDebutISO.length() == 19 && _heureFinISO.length() == 19)
    {
        // Extraction des composantes de la date de début
        int anneeDebut   = _heureDebutISO.substring(0, 4).toInt();
        int moisDebut    = _heureDebutISO.substring(5, 7).toInt();
        int jourDebut    = _heureDebutISO.substring(8, 10).toInt();
        int heureDebut   = _heureDebutISO.substring(11, 13).toInt();
        int minuteDebut  = _heureDebutISO.substring(14, 16).toInt();

        // Extraction des composantes de la date de fin
        int anneeFin     = _heureFinISO.substring(0, 4).toInt();
        int moisFin      = _heureFinISO.substring(5, 7).toInt();
        int jourFin      = _heureFinISO.substring(8, 10).toInt();
        int heureFin     = _heureFinISO.substring(11, 13).toInt();
        int minuteFin    = _heureFinISO.substring(14, 16).toInt();

        // Conversion en minutes depuis une époque simplifiée
        // On suppose que les dates sont dans la même année pour simplifier,
        // sinon on ajoute 365 jours par an d'écart
        int minutesDebut = ((anneeDebut * 525600) +
                            (moisDebut  *  43800) +
                            (jourDebut  *   1440) +
                            (heureDebut *     60) +
                             minuteDebut);

        int minutesFin   = ((anneeFin   * 525600) +
                            (moisFin    *  43800) +
                            (jourFin    *   1440) +
                            (heureFin   *     60) +
                             minuteFin);

        resultat = minutesFin - minutesDebut;

        if (resultat < 0)
            resultat = 0;
    }

    return resultat;
}