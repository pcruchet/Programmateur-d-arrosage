/**
 * @file    MesureBatteries.h
 * @brief   Mesure de tension/courant/puissance du pack batterie via le
 *          capteur INA219, et estimation du niveau de charge.
 *
 * @details Encapsule la bibliothèque Adafruit_INA219. Sur la carte
 *          "Alimentation BTS SNIR", le capteur est placé en série entre le
 *          rail 12,6 V (panneau solaire / sortie MOSFET Q1) et le chargeur
 *          3S (CH1) qui alimente le pack B1 (3 x Li-ion 3000 mAh en série,
 *          12,6 V pleine charge). Le capteur voit donc aussi bien le
 *          courant de charge (panneau → batterie) que le courant de
 *          décharge (batterie → boîtier), selon le sens du courant mesuré.
 *
 *          Le signe du courant retourné par estEnCharge() dépend du sens de
 *          câblage réel de Vin+/Vin- sur la carte : à vérifier une fois sur
 *          le matériel (multimètre en série, ou observation du signe
 *          pendant un ensoleillement franc) et à ajuster si besoin via
 *          SEUIL_COURANT_CHARGE_MA / l'inversion du signe dans le .cpp.
 */

#ifndef MESURE_BATTERIES_H
#define MESURE_BATTERIES_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "Debug.h"

// ── Caractéristiques du pack batterie (3S Li-ion, 3000 mAh, cf. CH1/B1) ───────
#define TENSION_PACK_PLEINE_V   12.6f  ///< 3 x 4,2 V, pack en fin de charge.
#define TENSION_PACK_VIDE_V     9.9f   ///< 3 x 3,3 V, marge de sécurité avant le seuil critique de 3,0 V/cellule.
#define CAPACITE_PACK_MAH       3000   ///< Capacité nominale d'une cellule du pack (cellules en série : capacité du pack inchangée).

// ── Seuil de détection de charge ──────────────────────────────────────────────
#define SEUIL_COURANT_CHARGE_MA 20.0f  ///< Au-delà de ce courant (mA), le pack est considéré en charge ; en-deçà, en décharge ou au repos (marge de bruit autour de 0).

/**
 * @struct MesureBatterie
 * @brief  Instantané complet d'une mesure du pack batterie.
 */
struct MesureBatterie
{
    float   tensionV;      ///< Tension du pack, en volts.
    float   courantMa;     ///< Courant mesuré, en mA (signe selon le sens de câblage, voir MesureBatteries.h).
    float   puissanceMw;   ///< Puissance instantanée, en mW.
    uint8_t pourcentage;   ///< Niveau de charge estimé (0 à 100 %), à partir de la seule tension.
    bool    enCharge;      ///< true si le courant mesuré dépasse SEUIL_COURANT_CHARGE_MA.
};

/**
 * @class MesureBatteries
 * @brief Pilote du capteur INA219 pour la surveillance du pack batterie.
 */
class MesureBatteries
{
public:
    /**
     * @brief Construit l'objet et prépare le pilote INA219 à l'adresse
     *        I2C donnée, sans encore communiquer avec le capteur.
     * @param _adresseI2C Adresse I2C du capteur (0x40 par défaut sur les
     *                     modules INA219 avec les ponts d'adressage non
     *                     soudés).
     */
    MesureBatteries(uint8_t _adresseI2C = 0x40);

    /**
     * @brief Initialise la communication I2C avec le capteur et applique
     *        la calibration par défaut (plage 32 V / 2 A), largement
     *        suffisante pour un pack 3S chargé/déchargé à quelques
     *        centaines de mA.
     * @return true si le capteur a été détecté et initialisé.
     */
    bool initialiser();

    /**
     * @brief Indique si le capteur a été initialisé avec succès.
     * @return Valeur du drapeau interne, positionné par initialiser().
     */
    bool disponible() const;

    /**
     * @brief Réalise une mesure complète du pack (tension, courant,
     *        puissance, pourcentage estimé, état de charge).
     * @return Structure MesureBatterie renseignée, ou valeurs nulles si
     *         le capteur n'est pas disponible.
     */
    MesureBatterie lireMesure() const;

    /**
     * @brief Lit la tension du pack.
     * @return Tension en volts, ou 0.0f si le capteur n'est pas disponible.
     */
    float lireTension() const;

    /**
     * @brief Lit le courant traversant le shunt du capteur.
     * @return Courant en mA, ou 0.0f si le capteur n'est pas disponible.
     */
    float lireCourant() const;

    /**
     * @brief Lit la puissance instantanée mesurée par le capteur.
     * @return Puissance en mW, ou 0.0f si le capteur n'est pas disponible.
     */
    float lirePuissance() const;

    /**
     * @brief Estime le niveau de charge du pack à partir de sa seule
     *        tension à vide (approximation : ne tient pas compte de la
     *        chute de tension en charge/décharge).
     * @details Interpole linéairement entre TENSION_PACK_VIDE_V (0 %) et
     *          TENSION_PACK_PLEINE_V (100 %), puis borne le résultat.
     * @return Pourcentage estimé (0 à 100).
     */
    uint8_t estimerPourcentage() const;

    /**
     * @brief Indique si le pack est actuellement en charge.
     * @return true si le courant mesuré dépasse SEUIL_COURANT_CHARGE_MA.
     */
    bool estEnCharge() const;

private:
    uint8_t adresseI2C;
    bool disponibleFlag;
    mutable Adafruit_INA219 ina219;
};

#endif // MESURE_BATTERIES_H
