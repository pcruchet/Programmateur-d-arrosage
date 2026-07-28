// test/test_MesureBatteries/test_MesureBatteries.cpp
//
// Test matériel interactif de la carte "Alimentation BTS SNIR" :
// vérifie que le capteur INA219 répond, que les mesures sont plausibles
// pour un pack 3S Li-ion 3000 mAh, puis lance une phase de surveillance
// en direct pendant laquelle chaque appui sur BP2 (GPIO39) affiche un
// relevé complet sur le port série — utile pour observer la charge en
// conditions réelles (panneau solaire branché ou non).
//
// Utilise les classes MesureBatteries et BoutonPoussoir de lib/, dans
// l'idée qu'elles pourront être reprises telles quelles dans
// BoitierPilotageArrosage une fois la carte batterie validée.

#include <Arduino.h>
#include <Wire.h>
#include <unity.h>
#include "MesureBatteries.h"
#include "BoutonPoussoir.h"

// ── Broches boutons (cf. schéma alimentation : BP1..BP4 sur GPIO34-39,
//    toutes en entrée seule avec résistance de tirage externe sur la carte) ──
#define BROCHE_BP2 39

// ── Durée de la phase de surveillance interactive ────────────────────────────
#define DUREE_SURVEILLANCE_MS  120000   // 2 minutes
#define DELAI_AFFICHAGE_AUTO_MS 15000   // affichage auto si aucun appui BP2

MesureBatteries mesureBatteries;
BoutonPoussoir bp2(BROCHE_BP2, false); // GPIO39 : pas de tirage interne, résistance externe sur la carte

void setUp()    {}
void tearDown() {}

// ============================================================
// Détection et plausibilité des mesures
// ============================================================

void test_initialiser_reussit()
{
    bool resultat = mesureBatteries.initialiser();

    TEST_ASSERT_TRUE_MESSAGE(resultat, "INA219 non detecte sur le bus I2C (verifier cablage SDA/SCL et adresse 0x40)");
}

void test_disponible_apres_init()
{
    TEST_ASSERT_TRUE(mesureBatteries.disponible());
}

void test_tension_plausible_pour_pack_3s()
{
    float tension = mesureBatteries.lireTension();

    Serial.print("Tension mesuree : ");
    Serial.print(tension, 3);
    Serial.println(" V");

    // Plage large pour couvrir un pack partiellement décharge jusqu'a
    // pleine charge, avec marge (cablage/shunt) : 6V (tres decharge, alerte)
    // a 13V (legerement au-dessus de la pleine charge nominale 12.6V).
    TEST_ASSERT_TRUE_MESSAGE(tension > 6.0f && tension < 13.0f,
                              "Tension hors plage plausible pour un pack 3S — verifier le cablage Vin+/Vin-");
}

void test_pourcentage_dans_les_bornes()
{
    uint8_t pourcentage = mesureBatteries.estimerPourcentage();

    Serial.print("Pourcentage estime : ");
    Serial.print(pourcentage);
    Serial.println(" %");

    TEST_ASSERT_TRUE(pourcentage <= 100);
}

void test_lecture_courant_et_puissance()
{
    float courant   = mesureBatteries.lireCourant();
    float puissance = mesureBatteries.lirePuissance();

    Serial.print("Courant mesure  : ");
    Serial.print(courant, 1);
    Serial.println(" mA");
    Serial.print("Puissance mesuree : ");
    Serial.print(puissance, 1);
    Serial.println(" mW");

    Serial.println(">>> Verifier le signe du courant : positif ou negatif selon");
    Serial.println(">>> le sens de cablage Vin+/Vin-, a noter pour ajuster");
    Serial.println(">>> estEnCharge() dans MesureBatteries.cpp si necessaire. <<<");

    TEST_PASS(); // valeurs affichees pour verification visuelle, pas d'assertion automatique sur le signe
}

// ============================================================
// Surveillance interactive (BP2)
// ============================================================

static void afficherMesure(const MesureBatterie &_mesure)
{
    Serial.println("──────────────────────────────────────────");
    Serial.print("Tension pack   : ");
    Serial.print(_mesure.tensionV, 3);
    Serial.println(" V");
    Serial.print("Courant        : ");
    Serial.print(_mesure.courantMa, 1);
    Serial.println(" mA");
    Serial.print("Puissance      : ");
    Serial.print(_mesure.puissanceMw, 1);
    Serial.println(" mW");
    Serial.print("Charge estimee : ");
    Serial.print(_mesure.pourcentage);
    Serial.println(" %");
    Serial.print("Etat           : ");
    Serial.println(_mesure.enCharge ? "EN CHARGE" : "au repos / decharge");
    Serial.println("──────────────────────────────────────────");
}

void test_surveillance_interactive_bp2()
{
    Serial.println("\n>>> Phase de surveillance : appuyer sur BP2 pour un releve <<<");
    Serial.println(">>> a tout moment (affichage automatique toutes les 15s). <<<");

    uint32_t debut = millis();
    uint32_t dernierAffichage = millis();

    afficherMesure(mesureBatteries.lireMesure());

    while ((millis() - debut) < DUREE_SURVEILLANCE_MS)
    {
        bp2.update();

        if (bp2.frontAppui())
        {
            Serial.println("\n>>> BP2 : releve demande <<<");
            afficherMesure(mesureBatteries.lireMesure());
            dernierAffichage = millis();
        }

        if ((millis() - dernierAffichage) > DELAI_AFFICHAGE_AUTO_MS)
        {
            afficherMesure(mesureBatteries.lireMesure());
            dernierAffichage = millis();
        }

        delay(10);
    }

    Serial.println("\n>>> Fin de la phase de surveillance <<<");

    TEST_PASS();
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║   TEST MATERIEL BATTERIE — INA219 / BP2  ║");
    Serial.println("╚══════════════════════════════════════════╝");

    Wire.begin();

    UNITY_BEGIN();

    RUN_TEST(test_initialiser_reussit);
    RUN_TEST(test_disponible_apres_init);
    RUN_TEST(test_tension_plausible_pour_pack_3s);
    RUN_TEST(test_pourcentage_dans_les_bornes);
    RUN_TEST(test_lecture_courant_et_puissance);
    RUN_TEST(test_surveillance_interactive_bp2);

    UNITY_END();

    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║   FIN DES TESTS BATTERIE                 ║");
    Serial.println("╚══════════════════════════════════════════╝");
}

void loop() {}
