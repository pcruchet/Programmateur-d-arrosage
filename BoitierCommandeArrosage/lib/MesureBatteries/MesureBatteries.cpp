/**
 * @file    MesureBatteries.cpp
 * @brief   Implémentation du pilote INA219 pour la surveillance du pack
 *          batterie.
 */

#include "MesureBatteries.h"

MesureBatteries::MesureBatteries(uint8_t _adresseI2C)
    : adresseI2C(_adresseI2C), disponibleFlag(false), ina219(_adresseI2C)
{
}

bool MesureBatteries::initialiser()
{
    disponibleFlag = ina219.begin(&Wire);

    if (disponibleFlag)
    {
        // Plage 32V/2A : marge confortable au-dessus des 12,6 V et des
        // quelques centaines de mA attendus sur ce pack 3S 3000 mAh.
        ina219.setCalibration_32V_2A();
        DEBUG("MesureBatteries : INA219 detecte et calibre");
    }
    else
    {
        DEBUG("MesureBatteries : INA219 non detecte sur le bus I2C");
    }

    return disponibleFlag;
}

bool MesureBatteries::disponible() const
{
    return disponibleFlag;
}

float MesureBatteries::lireTension() const
{
    float resultat = 0.0f;

    if (disponibleFlag)
    {
        // Tension côté charge (bus) + chute de tension aux bornes du shunt,
        // pour obtenir la tension réelle du pack en amont du shunt
        // (convention standard des exemples Adafruit_INA219).
        float tensionBus = ina219.getBusVoltage_V();
        float chuteShunt = ina219.getShuntVoltage_mV() / 1000.0f;
        resultat = tensionBus + chuteShunt;
    }

    return resultat;
}

float MesureBatteries::lireCourant() const
{
    float resultat = 0.0f;

    if (disponibleFlag)
        resultat = ina219.getCurrent_mA();

    return resultat;
}

float MesureBatteries::lirePuissance() const
{
    float resultat = 0.0f;

    if (disponibleFlag)
        resultat = ina219.getPower_mW();

    return resultat;
}

uint8_t MesureBatteries::estimerPourcentage() const
{
    uint8_t resultat = 0;

    if (disponibleFlag)
    {
        float tension = lireTension();
        float pourcentageBrut = (tension - TENSION_PACK_VIDE_V) /
                                 (TENSION_PACK_PLEINE_V - TENSION_PACK_VIDE_V) * 100.0f;
        float pourcentageBorne = constrain(pourcentageBrut, 0.0f, 100.0f);
        resultat = (uint8_t)(pourcentageBorne + 0.5f);
    }

    return resultat;
}

bool MesureBatteries::estEnCharge() const
{
    bool resultat = false;

    if (disponibleFlag)
        resultat = (lireCourant() > SEUIL_COURANT_CHARGE_MA);

    return resultat;
}

MesureBatterie MesureBatteries::lireMesure() const
{
    MesureBatterie mesure;

    mesure.tensionV    = lireTension();
    mesure.courantMa   = lireCourant();
    mesure.puissanceMw = lirePuissance();
    mesure.pourcentage = estimerPourcentage();
    mesure.enCharge     = estEnCharge();

    return mesure;
}
