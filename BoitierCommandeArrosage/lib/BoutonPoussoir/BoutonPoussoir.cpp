/**
 * @file    BoutonPoussoir.cpp
 * @brief   Implémentation de la lecture anti-rebond d'un bouton poussoir.
 */

#include "BoutonPoussoir.h"

BoutonPoussoir::BoutonPoussoir(uint8_t _broche, bool _avecTirageInterne,
                                uint16_t _delaiAntiRebondMs)
    : broche(_broche), delaiAntiRebond(_delaiAntiRebondMs),
      etatStable(HIGH), etatPrecedentLu(HIGH), evenementAppui(false),
      dernierChangementMs(0)
{
    pinMode(broche, _avecTirageInterne ? INPUT_PULLUP : INPUT);
}

void BoutonPoussoir::update()
{
    bool lecture = digitalRead(broche);

    if (lecture != etatPrecedentLu)
    {
        dernierChangementMs = millis();
        etatPrecedentLu = lecture;
    }

    if ((millis() - dernierChangementMs) > delaiAntiRebond)
    {
        if (lecture != etatStable)
        {
            etatStable = lecture;

            if (etatStable == LOW)
                evenementAppui = true;
        }
    }
}

bool BoutonPoussoir::frontAppui()
{
    bool resultat = evenementAppui;

    evenementAppui = false;

    return resultat;
}

bool BoutonPoussoir::estAppuye() const
{
    bool resultat = (etatStable == LOW);

    return resultat;
}
