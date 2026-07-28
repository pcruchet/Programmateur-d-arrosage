// test/test_StockageProgrammationVannes/test_StockageProgrammationVannes.cpp

#include <Arduino.h>
#include <unity.h>
#include "StockageProgrammationVannes.h"

StockageProgrammationVannes stockage;

void setUp()
{
    // Repartir d'un état propre avant chaque test
    stockage.reinitialiserTout();
}

void tearDown()
{
    // Nettoyer après chaque test pour ne pas polluer le suivant
    stockage.reinitialiserTout();
}

// ============================================================
// Tests initialiser()
// ============================================================

void test_initialiser_reussit()
{
    bool resultat = stockage.initialiser();

    TEST_ASSERT_TRUE(resultat);
}

// ============================================================
// Tests valeurs par défaut (NVS vierge)
// ============================================================

void test_lireProgrammation_valeurs_defaut()
{
    ProgrammationVanne programmation = stockage.lireProgrammation(1);

    TEST_ASSERT_EQUAL('M', programmation.mode);
    TEST_ASSERT_EQUAL_STRING("", programmation.heure.c_str());
    TEST_ASSERT_EQUAL(0, programmation.duree);
    TEST_ASSERT_EQUAL(0, programmation.frequence);
}

void test_lireMode_valeur_defaut()
{
    char mode = stockage.lireMode(2);

    TEST_ASSERT_EQUAL('M', mode);
}

// ============================================================
// Tests écriture/lecture programmation complète
// ============================================================

void test_ecrireProgrammation_puis_lire_vanne1()
{
    ProgrammationVanne programmation;
    programmation.mode      = 'P';
    programmation.heure     = "2026-07-08T18:30:00";
    programmation.duree     = 15;
    programmation.frequence = 24;

    stockage.ecrireProgrammation(1, programmation);

    ProgrammationVanne lue = stockage.lireProgrammation(1);

    TEST_ASSERT_EQUAL('P', lue.mode);
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", lue.heure.c_str());
    TEST_ASSERT_EQUAL(15, lue.duree);
    TEST_ASSERT_EQUAL(24, lue.frequence);
}

void test_ecrireProgrammation_vanne_differente_isolee()
{
    ProgrammationVanne prog1;
    prog1.mode      = 'A';
    prog1.heure     = "2026-01-01T00:00:00";
    prog1.duree     = 10;
    prog1.frequence = 6;
    stockage.ecrireProgrammation(1, prog1);

    ProgrammationVanne prog2;
    prog2.mode      = 'M';
    prog2.heure     = "2026-02-02T12:00:00";
    prog2.duree     = 20;
    prog2.frequence = 12;
    stockage.ecrireProgrammation(2, prog2);

    ProgrammationVanne lue1 = stockage.lireProgrammation(1);
    ProgrammationVanne lue2 = stockage.lireProgrammation(2);

    TEST_ASSERT_EQUAL('A', lue1.mode);
    TEST_ASSERT_EQUAL(10, lue1.duree);

    TEST_ASSERT_EQUAL('M', lue2.mode);
    TEST_ASSERT_EQUAL(20, lue2.duree);
}

void test_ecrireProgrammation_les_4_vannes()
{
    uint8_t idVanne = 1;

    while (idVanne <= 4)
    {
        ProgrammationVanne programmation;
        programmation.mode      = 'P';
        programmation.heure     = "2026-03-03T08:00:00";
        programmation.duree     = idVanne * 5;
        programmation.frequence = idVanne * 2;

        stockage.ecrireProgrammation(idVanne, programmation);

        idVanne++;
    }

    idVanne = 1;
    while (idVanne <= 4)
    {
        ProgrammationVanne lue = stockage.lireProgrammation(idVanne);

        TEST_ASSERT_EQUAL(idVanne * 5, lue.duree);
        TEST_ASSERT_EQUAL(idVanne * 2, lue.frequence);

        idVanne++;
    }
}

// ============================================================
// Tests champ ciblé : mode
// ============================================================

void test_ecrireMode_puis_lire()
{
    stockage.ecrireMode(3, 'A');

    char mode = stockage.lireMode(3);

    TEST_ASSERT_EQUAL('A', mode);
}

void test_ecrireMode_ne_modifie_pas_autres_champs()
{
    ProgrammationVanne programmation;
    programmation.mode      = 'M';
    programmation.heure     = "2026-04-04T10:00:00";
    programmation.duree     = 30;
    programmation.frequence = 48;
    stockage.ecrireProgrammation(4, programmation);

    // Modifie uniquement le mode
    stockage.ecrireMode(4, 'P');

    ProgrammationVanne lue = stockage.lireProgrammation(4);

    TEST_ASSERT_EQUAL('P', lue.mode);                              // changé
    TEST_ASSERT_EQUAL_STRING("2026-04-04T10:00:00", lue.heure.c_str());  // inchangé
    TEST_ASSERT_EQUAL(30, lue.duree);                               // inchangé
    TEST_ASSERT_EQUAL(48, lue.frequence);                           // inchangé
}

// ============================================================
// Tests réinitialisation
// ============================================================

void test_reinitialiserVanne_retour_aux_defauts()
{
    ProgrammationVanne programmation;
    programmation.mode      = 'A';
    programmation.heure     = "2026-05-05T05:00:00";
    programmation.duree     = 99;
    programmation.frequence = 99;
    stockage.ecrireProgrammation(1, programmation);

    stockage.reinitialiserVanne(1);

    ProgrammationVanne lue = stockage.lireProgrammation(1);

    TEST_ASSERT_EQUAL('M', lue.mode);
    TEST_ASSERT_EQUAL_STRING("", lue.heure.c_str());
    TEST_ASSERT_EQUAL(0, lue.duree);
    TEST_ASSERT_EQUAL(0, lue.frequence);
}

void test_reinitialiserTout_remet_les_4_vannes()
{
    uint8_t idVanne = 1;

    while (idVanne <= 4)
    {
        ProgrammationVanne programmation;
        programmation.mode      = 'A';
        programmation.heure     = "2026-06-06T06:00:00";
        programmation.duree     = 50;
        programmation.frequence = 50;
        stockage.ecrireProgrammation(idVanne, programmation);

        idVanne++;
    }

    stockage.reinitialiserTout();

    idVanne = 1;
    while (idVanne <= 4)
    {
        ProgrammationVanne lue = stockage.lireProgrammation(idVanne);

        TEST_ASSERT_EQUAL('M', lue.mode);
        TEST_ASSERT_EQUAL(0, lue.duree);

        idVanne++;
    }
}

// ============================================================
// Test persistance réelle (simule un redémarrage)
// ============================================================

void test_persistance_apres_nouvelle_instance()
{
    ProgrammationVanne programmation;
    programmation.mode      = 'P';
    programmation.heure     = "2026-07-07T07:00:00";
    programmation.duree     = 25;
    programmation.frequence = 8;

    stockage.ecrireProgrammation(2, programmation);

    // Nouvelle instance = simule un redémarrage ESP32
    // (preferences.begin/end à chaque appel garantit la vraie persistance flash,
    //  pas juste une valeur en RAM)
    StockageProgrammationVannes nouvelleInstance;

    ProgrammationVanne lue = nouvelleInstance.lireProgrammation(2);

    TEST_ASSERT_EQUAL('P', lue.mode);
    TEST_ASSERT_EQUAL_STRING("2026-07-07T07:00:00", lue.heure.c_str());
    TEST_ASSERT_EQUAL(25, lue.duree);
    TEST_ASSERT_EQUAL(8, lue.frequence);
}

// ============================================================
// Tests id vanne invalide
// ============================================================

void test_ecrireProgrammation_id_invalide_sans_effet()
{
    ProgrammationVanne programmation;
    programmation.mode      = 'A';
    programmation.heure     = "2026-08-08T08:00:00";
    programmation.duree     = 40;
    programmation.frequence = 40;

    // id 0 et id 5 sont hors plage (1 à 4)
    stockage.ecrireProgrammation(0, programmation);
    stockage.ecrireProgrammation(5, programmation);

    TEST_PASS();   // ne doit pas crasher
}

void test_lireProgrammation_id_invalide_retourne_defaut()
{
    ProgrammationVanne lue = stockage.lireProgrammation(0);

    TEST_ASSERT_EQUAL('M', lue.mode);
    TEST_ASSERT_EQUAL(0, lue.duree);
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    stockage.initialiser();

    UNITY_BEGIN();

    // ── Initialisation ──────────────────────────────────────
    RUN_TEST(test_initialiser_reussit);

    // ── Valeurs par défaut ───────────────────────────────────
    RUN_TEST(test_lireProgrammation_valeurs_defaut);
    RUN_TEST(test_lireMode_valeur_defaut);

    // ── Écriture/lecture complète ─────────────────────────────
    RUN_TEST(test_ecrireProgrammation_puis_lire_vanne1);
    RUN_TEST(test_ecrireProgrammation_vanne_differente_isolee);
    RUN_TEST(test_ecrireProgrammation_les_4_vannes);

    // ── Champ ciblé mode ──────────────────────────────────────
    RUN_TEST(test_ecrireMode_puis_lire);
    RUN_TEST(test_ecrireMode_ne_modifie_pas_autres_champs);

    // ── Réinitialisation ──────────────────────────────────────
    RUN_TEST(test_reinitialiserVanne_retour_aux_defauts);
    RUN_TEST(test_reinitialiserTout_remet_les_4_vannes);

    // ── Persistance réelle ────────────────────────────────────
    RUN_TEST(test_persistance_apres_nouvelle_instance);

    // ── Robustesse id invalide ────────────────────────────────
    RUN_TEST(test_ecrireProgrammation_id_invalide_sans_effet);
    RUN_TEST(test_lireProgrammation_id_invalide_retourne_defaut);

    // Nettoyage final pour ne pas laisser de données de test en NVS
    stockage.reinitialiserTout();

    UNITY_END();
}

void loop() {}
