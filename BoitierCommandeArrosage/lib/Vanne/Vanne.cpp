/**
 * @file   Vanne.cpp
 * @author Philippe CRUCHET
 * @date   25/06/2026
 * @brief  Implémentation du pilotage d'une électrovanne bistable.
 */

#include "Vanne.h"

/**
 * @brief Construit une vanne et initialise ses broches matérielles.
 * @details Configure les 3 broches (sélection, IN A, IN B) en sortie, puis
 *          sélectionne brièvement la vanne pour couper ses sorties
 *          (arreter()) avant de la désélectionner, garantissant un état de
 *          repos propre au démarrage.
 * @param _brocheSelection Broche GPIO de sélection sur le multiplexeur.
 * @param _inA              Broche IN A du pont en H.
 * @param _inB              Broche IN B du pont en H.
 * @param _dureeImpulsionMs Durée de l'impulsion de commande, en ms.
 */
Vanne::Vanne(uint8_t _brocheSelection, uint8_t _inA, uint8_t _inB, uint16_t _dureeImpulsionMs)
    : brocheSelection(_brocheSelection), inA(_inA), inB(_inB), dureeImpulsion(_dureeImpulsionMs), ouverte(false), enCours(false), etatCible(false)
{
    pinMode(brocheSelection, OUTPUT);
    pinMode(inA, OUTPUT);
    pinMode(inB, OUTPUT);
    selectionner();
    arreter();
    deselectionner();
}

/// Positionne la broche de sélection à l'état haut pour désigner cette
/// vanne comme destinataire des signaux IN A/IN B du pont en H.
void Vanne::selectionner()
{
    digitalWrite(brocheSelection, HIGH);
}

/// Positionne la broche de sélection à l'état bas : cette vanne n'est plus
/// affectée par les signaux IN A/IN B.
void Vanne::deselectionner()
{
    digitalWrite(brocheSelection, LOW);
}

/// Coupe les deux sorties du pont en H (IN A et IN B à LOW).
void Vanne::arreter()
{
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
}

/**
 * @brief Modifie la durée d'impulsion utilisée pour les prochaines
 *        manœuvres.
 * @param _dureeMs Nouvelle durée, en millisecondes.
 */
void Vanne::setDureeImpulsion(uint16_t _dureeMs)
{
    dureeImpulsion = _dureeMs;
}

/**
 * @brief Indique l'état logique courant de la vanne.
 * @return true si ouverte, false si fermée.
 */
bool Vanne::estOuverte() const
{
    return ouverte;
}

/**
 * @brief Indique si une impulsion est en cours.
 * @return true tant que la durée d'impulsion n'est pas écoulée.
 */
bool Vanne::estEnCours() const
{
    return enCours;
}

/**
 * @brief Force l'état logique en mémoire sans piloter le matériel.
 * @param _ouverte Nouvel état logique (true = ouverte).
 */
void Vanne::setEtatLogique(bool _ouverte)
{
    ouverte = _ouverte;
}

/**
 * @brief Démarre une impulsion de commande dans le sens demandé.
 * @details Mémorise la cible (etatCible) et l'horodatage de départ
 *          (debutImpulsion via millis()), sélectionne la vanne, puis
 *          positionne IN A/IN B selon le sens voulu.
 * @param _ouvrir true = impulsion d'ouverture (IN A=HIGH, IN B=LOW),
 *                false = impulsion de fermeture (IN A=LOW, IN B=HIGH).
 */
void Vanne::demarrerImpulsion(bool _ouvrir)
{
    etatCible = _ouvrir;
    enCours = true;
    debutImpulsion = millis();
    selectionner();
    if (_ouvrir)
    {
        digitalWrite(inA, HIGH);
        digitalWrite(inB, LOW);
    }
    else
    {
        digitalWrite(inA, LOW);
        digitalWrite(inB, HIGH);
    }
}

/**
 * @brief Déclenche l'ouverture de la vanne si elle est fermée et
 *        disponible.
 * @details Garde de sécurité : ne démarre une impulsion que si la vanne
 *          n'est ni déjà ouverte, ni déjà en cours de manœuvre. La lecture
 *          des drapeaux et l'appel à demarrerImpulsion() sont encadrés par
 *          le spinlock mux, afin de rester cohérents avec update() qui peut
 *          être invoquée de façon concurrente depuis loop().
 */
void Vanne::ouvrir()
{
    portENTER_CRITICAL(&mux);
    if (!ouverte && !enCours)
        demarrerImpulsion(true);
    portEXIT_CRITICAL(&mux);
}

/**
 * @brief Déclenche la fermeture de la vanne si elle est ouverte et
 *        disponible.
 * @details Garde symétrique de ouvrir(), également protégée par le
 *          spinlock mux.
 */
void Vanne::fermer()
{
    portENTER_CRITICAL(&mux);
    if (ouverte && !enCours)
        demarrerImpulsion(false);
    portEXIT_CRITICAL(&mux);
}

/**
 * @brief Fait progresser l'impulsion en cours ; à appeler régulièrement
 *        depuis la boucle principale.
 * @details Si une impulsion est en cours et que le temps écoulé depuis son
 *          démarrage atteint dureeImpulsion, coupe les sorties du pont en H
 *          (arreter()), désélectionne la vanne, applique l'état cible
 *          (etatCible) à l'état logique (ouverte) et marque l'impulsion
 *          comme terminée (enCours = false). Sans effet si aucune impulsion
 *          n'est en cours, ou si la durée n'est pas encore écoulée. Le bloc
 *          est encadré par le spinlock mux, symétriquement à ouvrir() et
 *          fermer().
 */
void Vanne::update()
{
    portENTER_CRITICAL(&mux);
    if (enCours)
    {
        if ((millis() - debutImpulsion) >= dureeImpulsion)
        {
            arreter();
            deselectionner();
            ouverte = etatCible;
            enCours = false;
        }
    }
    portEXIT_CRITICAL(&mux);
}
