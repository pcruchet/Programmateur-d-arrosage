/**
 * @file    BoutonPoussoir.h
 * @brief   Lecture anti-rebond d'un bouton poussoir actif à l'état bas, par
 *          scrutation (à appeler régulièrement depuis loop() via update()).
 *
 * @details Ne gère pas les broches en tant que source de réveil deep sleep
 *          (voir BROCHE_BP1/configurerSourcesReveil() dans
 *          BoitierPilotageArrosage pour cet usage) : cette classe couvre
 *          l'usage complémentaire de détection d'appui pendant que le
 *          boîtier est déjà éveillé (ex. BP2/GPIO39 pour déclencher un
 *          affichage ponctuel).
 *
 *          Important : les broches GPIO34 à GPIO39 de l'ESP32 sont en
 *          entrée seule et ne disposent d'aucune résistance de tirage
 *          interne. Pour ces broches (dont BP2/GPIO39), une résistance de
 *          tirage au +3,3V doit être présente sur la carte, et le
 *          constructeur doit être appelé avec _avecTirageInterne=false.
 */

#ifndef BOUTON_POUSSOIR_H
#define BOUTON_POUSSOIR_H

#include <Arduino.h>

/**
 * @class BoutonPoussoir
 * @brief Bouton poussoir actif à l'état bas, avec anti-rebond logiciel et
 *        détection de front d'appui.
 */
class BoutonPoussoir
{
public:
    /**
     * @brief Construit le bouton et configure sa broche en entrée.
     * @param _broche             Broche GPIO reliée au bouton.
     * @param _avecTirageInterne  true pour activer le tirage interne
     *                            (INPUT_PULLUP) ; à mettre à false pour les
     *                            broches GPIO34-39, qui n'en disposent pas
     *                            (nécessitent une résistance de tirage
     *                            externe sur la carte).
     * @param _delaiAntiRebondMs  Durée de stabilisation requise avant de
     *                            valider un changement d'état, en ms.
     */
    BoutonPoussoir(uint8_t _broche, bool _avecTirageInterne = true,
                   uint16_t _delaiAntiRebondMs = 50);

    /**
     * @brief Met à jour l'état anti-rebond du bouton. À appeler
     *        régulièrement (à chaque tour de boucle).
     */
    void update();

    /**
     * @brief Indique si un appui a été détecté depuis le dernier appel.
     * @details Consomme l'événement : un appel retournant true remet le
     *          drapeau interne à false (l'appui n'est signalé qu'une fois).
     * @return true si un front d'appui a été détecté depuis le dernier
     *         appel à frontAppui().
     */
    bool frontAppui();

    /**
     * @brief Indique l'état stable courant du bouton.
     * @return true si le bouton est actuellement considéré appuyé.
     */
    bool estAppuye() const;

private:
    uint8_t broche;
    uint16_t delaiAntiRebond;
    bool etatStable;
    bool etatPrecedentLu;
    bool evenementAppui;
    uint32_t dernierChangementMs;
};

#endif // BOUTON_POUSSOIR_H
