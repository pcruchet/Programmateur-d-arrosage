/**
 * @file    Vanne.h
 * @author  Philippe CRUCHET
 * @date    25/06/2026
 * @brief   Pilotage physique d'une électrovanne bistable via un pont en H
 *          partagé (L298), multiplexé par une broche de sélection.
 *
 * @details Une électrovanne bistable ne nécessite qu'une brève impulsion de
 *          courant (quelques centaines de ms à 1 s) pour basculer d'un état
 *          à l'autre, puis reste dans cet état sans consommation
 *          (mécanisme à aimant permanent). Cette classe gère : la sélection
 *          de la vanne sur le multiplexeur du pont en H, le déclenchement
 *          de l'impulsion dans le bon sens (ouverture/fermeture), et la
 *          détection non bloquante de la fin de l'impulsion via update().
 */

#ifndef VANNE_H
#define VANNE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "Debug.h"

/**
 * @class Vanne
 * @brief Pilote une électrovanne bistable individuelle.
 *
 * @details L'appelant doit invoquer update() régulièrement (à chaque tour de
 *          boucle) tant qu'une impulsion peut être en cours
 *          (estEnCours() == true), afin que la coupure de l'impulsion et la
 *          mise à jour de l'état logique s'effectuent au bon moment. Aucune
 *          temporisation bloquante n'est utilisée en interne (hormis dans le
 *          code appelant qui souhaiterait attendre activement la fin de
 *          l'impulsion).
 */
class Vanne
{
public:
    /**
     * @brief Construit une vanne et initialise ses broches matérielles.
     * @details Configure les broches de sélection et de commande du pont en
     *          H en sortie, puis laisse la vanne au repos (sélectionnée
     *          brièvement pour l'arrêt des sorties, puis désélectionnée).
     *          L'état logique initial est "fermée".
     * @param _brocheSelection Broche GPIO de sélection de cette vanne sur le
     *                          multiplexeur du pont en H.
     * @param _inA              Broche IN A du pont en H, partagée entre les
     *                          vannes.
     * @param _inB              Broche IN B du pont en H, partagée entre les
     *                          vannes.
     * @param _dureeImpulsionMs Durée de l'impulsion de commande, en
     *                          millisecondes (1000 ms par défaut).
     */
    Vanne(uint8_t _brocheSelection, uint8_t _inA, uint8_t _inB,
          uint16_t _dureeImpulsionMs = 1000);

    /**
     * @brief Déclenche l'ouverture de la vanne.
     * @details N'a d'effet que si la vanne est actuellement fermée et
     *          qu'aucune impulsion n'est déjà en cours ; sinon, ne fait
     *          rien (appel sans effet, sûr à répéter). Démarre une
     *          impulsion non bloquante : l'état logique "ouverte" ne sera
     *          effectif qu'après l'appel de update() constatant la fin de
     *          l'impulsion.
     */
    void ouvrir();

    /**
     * @brief Déclenche la fermeture de la vanne.
     * @details Symétrique de ouvrir() : n'a d'effet que si la vanne est
     *          actuellement ouverte et qu'aucune impulsion n'est en cours.
     */
    void fermer();

    /**
     * @brief Fait progresser l'impulsion en cours, à appeler régulièrement.
     * @details Si une impulsion est en cours et que sa durée
     *          (dureeImpulsion) est écoulée depuis son démarrage, coupe les
     *          sorties du pont en H, désélectionne la vanne, met à jour
     *          l'état logique (ouverte/fermée) selon la cible visée, et
     *          marque l'impulsion comme terminée. Sans effet si aucune
     *          impulsion n'est en cours.
     */
    void update();

    /**
     * @brief Modifie la durée d'impulsion utilisée pour les prochaines
     *        manœuvres.
     * @param _dureeMs Nouvelle durée d'impulsion, en millisecondes.
     */
    void setDureeImpulsion(uint16_t _dureeMs);

    /**
     * @brief Indique l'état logique courant de la vanne.
     * @return true si la vanne est considérée ouverte (dernière impulsion
     *         d'ouverture terminée), false sinon.
     */
    bool estOuverte() const;

    /**
     * @brief Indique si une impulsion est actuellement en cours.
     * @return true tant que le délai dureeImpulsion depuis le démarrage de
     *         la dernière impulsion n'est pas écoulé.
     */
    bool estEnCours() const;

    /**
     * @brief Force l'état logique de la vanne sans déclencher d'impulsion
     *        physique.
     * @details Utilisée pour resynchroniser l'état logique en mémoire avec
     *          un état persisté (NVS) relu au démarrage, sans réactionner le
     *          matériel.
     * @param _ouverte Nouvel état logique à mémoriser (true = ouverte).
     */
    void setEtatLogique(bool _ouverte);

private:
    uint8_t brocheSelection;   ///< Broche GPIO de sélection de cette vanne sur le multiplexeur du pont en H.
    uint8_t inA;                ///< Broche IN A du pont en H (partagée entre les 4 vannes).
    uint8_t inB;                ///< Broche IN B du pont en H (partagée entre les 4 vannes).
    uint16_t dureeImpulsion;   ///< Durée de l'impulsion de commande, en millisecondes.
    bool ouverte;               ///< État logique courant de la vanne (true = ouverte).
    bool enCours;               ///< Indique si une impulsion est en cours d'exécution.
    bool etatCible;             ///< État visé par l'impulsion en cours (true=ouvrir, false=fermer).
    uint32_t debutImpulsion = 0; ///< Horodatage (millis()) du démarrage de l'impulsion en cours.
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock FreeRTOS protégeant l'accès concurrent aux données d'état de la vanne (ouvrir()/fermer() peuvent être appelées depuis le contexte applicatif pendant qu'update() est appelée depuis loop(), et les deux manipulent enCours/etatCible/debutImpulsion).

    /// Sélectionne cette vanne sur le multiplexeur (broche de sélection à HIGH).
    void selectionner();

    /// Désélectionne cette vanne sur le multiplexeur (broche de sélection à LOW).
    void deselectionner();

    /// Coupe les deux sorties du pont en H (IN A et IN B à LOW), arrêtant
    /// toute impulsion en cours sur cette vanne.
    void arreter();

    /**
     * @brief Démarre une impulsion de commande dans le sens demandé.
     * @details Mémorise la cible et l'horodatage de départ, sélectionne la
     *          vanne, puis positionne IN A/IN B selon le sens voulu
     *          (IN A=HIGH, IN B=LOW pour ouvrir ; inversement pour fermer).
     * @param _ouvrir true pour démarrer une impulsion d'ouverture, false
     *                pour une impulsion de fermeture.
     */
    void demarrerImpulsion(bool _ouvrir);
};

#endif // VANNE_H
