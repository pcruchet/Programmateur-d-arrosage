/**
 * @file    StockageEtatVannes.h
 * @brief   Persistance NVS de l'état physique (ouverte/fermée) de chacune
 *          des 4 vannes, et détection de dépassement de délai d'arrosage.
 *
 * @details Distinct de StockageProgrammationVannes (qui persiste la
 *          programmation horaire) : cette classe ne conserve que l'état
 *          courant réel de chaque vanne et l'horodatage de sa dernière
 *          ouverture, afin de pouvoir restaurer cet état après un
 *          redémarrage et détecter si une vanne est restée ouverte plus
 *          longtemps que prévu (sécurité anti-inondation).
 */

#ifndef STOCKAGE_ETAT_VANNES_H
#define STOCKAGE_ETAT_VANNES_H

#include <Arduino.h>
#include <Preferences.h>

#include "Debug.h"

// ── Structure de l'état runtime d'une vanne ───────────────────────────────────

/**
 * @struct EtatVanne
 * @brief  État persisté d'une vanne : ouverte/fermée et horodatage
 *         d'ouverture.
 */
struct EtatVanne
{
    bool   ouverte;          ///< État physique actuel de la vanne (true = ouverte).
    String heureOuverture;   ///< Heure ISO8601 à laquelle la vanne a été ouverte ; chaîne vide si la vanne est fermée.
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class StockageEtatVannes
 * @brief Persistance NVS (Preferences) de l'état physique des 4 vannes et
 *        détection de dépassement de délai.
 */
class StockageEtatVannes
{
public:
    static const uint8_t NB_VANNES_MAX     = 4;              ///< Nombre maximal de vannes gérées.
    static const int     MARGE_SECURITE_MS = 15 * 60 * 1000;  ///< Marge de sécurité ajoutée à la durée programmée avant de considérer le délai comme dépassé (15 minutes, en ms).

    /// Construit l'objet sans encore ouvrir l'espace de noms NVS.
    StockageEtatVannes();

    // ── Initialisation ────────────────────────────────────────

    /**
     * @brief Vérifie que l'espace de noms NVS dédié est accessible.
     * @details Ouvre puis referme immédiatement l'espace de noms en mode
     *          lecture/écriture, uniquement pour valider l'accès au
     *          stockage.
     * @return true si l'ouverture a réussi.
     */
    bool initialiser();

    // ── Écriture ──────────────────────────────────────────────

    /**
     * @brief Persiste l'état complet d'une vanne (ouverte/fermée +
     *        horodatage).
     * @param _idVanne Identifiant de la vanne (1 à 4) ; sans effet si hors
     *                  plage.
     * @param _etat    État à persister.
     */
    void sauvegarderEtat(uint8_t _idVanne, const EtatVanne &_etat);

    // Raccourcis ciblés

    /**
     * @brief Raccourci pour persister l'ouverture d'une vanne.
     * @param _idVanne  Identifiant de la vanne (1 à 4).
     * @param _heureISO Heure d'ouverture, au format ISO8601.
     */
    void sauvegarderOuverture(uint8_t _idVanne, const String &_heureISO);

    /**
     * @brief Raccourci pour persister la fermeture d'une vanne (état
     *        fermée, horodatage effacé).
     * @param _idVanne Identifiant de la vanne (1 à 4).
     */
    void sauvegarderFermeture(uint8_t _idVanne);

    // ── Lecture ───────────────────────────────────────────────

    /**
     * @brief Relit l'état persisté d'une vanne.
     * @param _idVanne Identifiant de la vanne (1 à 4).
     * @return État lu (ouverte=false et heureOuverture="" par défaut si
     *         l'identifiant est invalide ou si aucune valeur n'est
     *         encore persistée).
     */
    EtatVanne lireEtat(uint8_t _idVanne) const;

    /**
     * @brief Indique si une vanne donnée est actuellement ouverte, d'après
     *        l'état persisté.
     * @param _idVanne Identifiant de la vanne (1 à 4).
     * @return true si la vanne est marquée ouverte en NVS.
     */
    bool      vanneOuverte(uint8_t _idVanne) const;

    /**
     * @brief Indique si au moins une des 4 vannes est actuellement
     *        ouverte, d'après l'état persisté.
     * @return true dès qu'une vanne est marquée ouverte.
     */
    bool      uneVanneOuverte() const;

    // ── Timeout de sécurité ───────────────────────────────────

    /**
     * @brief Indique si une vanne ouverte a dépassé la durée d'arrosage
     *        programmée, marge de sécurité incluse.
     * @details Calcule l'écart en minutes entre l'heure d'ouverture
     *          persistée et l'heure actuelle fournie, et le compare à
     *          _dureeMinutes majorée de MARGE_SECURITE_MS. Retourne false
     *          si la vanne n'est pas ouverte ou si aucun horodatage
     *          d'ouverture n'est disponible.
     * @param _idVanne          Identifiant de la vanne (1 à 4).
     * @param _dureeMinutes     Durée d'arrosage programmée, en minutes.
     * @param _heureActuelleISO Heure actuelle de référence, au format
     *                          ISO8601.
     * @return true si la vanne est ouverte depuis plus de
     *         (_dureeMinutes + marge) minutes.
     */
    bool delaiDepasse(uint8_t _idVanne,
                      int _dureeMinutes,
                      const String &_heureActuelleISO) const;

    // ── Réinitialisation ──────────────────────────────────────

    /**
     * @brief Réinitialise l'état persisté d'une vanne à "fermée".
     * @param _idVanne Identifiant de la vanne (1 à 4).
     */
    void reinitialiserVanne(uint8_t _idVanne);

    /// Réinitialise l'état persisté des 4 vannes à "fermée".
    void reinitialiserTout();

private:
    mutable Preferences preferences; ///< Instance d'accès à l'espace de stockage NVS (mutable pour permettre l'ouverture en lecture depuis des méthodes const).

    /**
     * @brief Construit la clé NVS de l'état ouvert/fermé d'une vanne.
     * @param _idVanne Identifiant de la vanne.
     * @return Clé au format "v{idVanne}_ouv".
     */
    String cleOuverte(uint8_t _idVanne) const;

    /**
     * @brief Construit la clé NVS de l'horodatage d'ouverture d'une vanne.
     * @param _idVanne Identifiant de la vanne.
     * @return Clé au format "v{idVanne}_heure".
     */
    String cleHeure(uint8_t _idVanne) const;

    /**
     * @brief Valide qu'un identifiant de vanne est dans la plage gérée.
     * @param _idVanne Identifiant à valider.
     * @return true si _idVanne est compris entre 1 et NB_VANNES_MAX.
     */
    bool   idVanneValide(uint8_t _idVanne) const;

    /**
     * @brief Calcule la différence en minutes entre deux dates ISO8601.
     * @details Approximation calendaire simplifiée (365 jours/an,
     *          30 jours/mois) suffisante pour un calcul d'écart sur une
     *          courte période (durée d'arrosage de l'ordre de quelques
     *          dizaines de minutes) ; ne gère pas les années bissextiles
     *          ni les mois de longueur variable au-delà de cette
     *          approximation.
     * @param _heureDebutISO Date/heure de début, ISO8601 (doit faire
     *                       exactement 19 caractères, sinon retourne 0).
     * @param _heureFinISO   Date/heure de fin, ISO8601 (mêmes contraintes).
     * @return Écart en minutes (>= 0 ; ramené à 0 si le résultat brut
     *         serait négatif).
     */
    int diffMinutes(const String &_heureDebutISO,
                    const String &_heureFinISO) const;
};

#endif // STOCKAGE_ETAT_VANNES_H
