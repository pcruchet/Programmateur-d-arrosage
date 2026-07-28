// test/test_Vanne_logique/test_Vanne_logique.cpp

#include <Arduino.h>
#include <unity.h>
#include "Vanne.h"

// ── Broches réelles selon schéma L298 ────────────────────────────────────────
#define PIN_SEL_V1  5
#define PIN_SEL_V2  17
#define PIN_SEL_V3  16
#define PIN_SEL_V4  4
#define PIN_INA     2
#define PIN_INB     15

// On teste sur la vanne 1, durée courte pour les tests temporels
Vanne vanne(PIN_SEL_V1, PIN_INA, PIN_INB, 100);

void setUp()    {}
void tearDown() {}

// ============================================================
// Tests état initial
// ============================================================

void test_etat_initial_fermee()
{
    TEST_ASSERT_FALSE(vanne.estOuverte());
}

void test_etat_initial_pas_en_cours()
{
    TEST_ASSERT_FALSE(vanne.estEnCours());
}

// ============================================================
// Tests logique ouvrir()
// ============================================================

void test_ouvrir_demarre_impulsion()
{
    vanne.ouvrir();
    TEST_ASSERT_TRUE(vanne.estEnCours());
}

void test_ouvrir_pendant_impulsion_sans_effet()
{
    // Impulsion déjà en cours depuis le test précédent
    vanne.ouvrir();
    TEST_ASSERT_TRUE(vanne.estEnCours());
}

void test_ouvrir_complete_apres_impulsion()
{
    delay(150);
    vanne.update();
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_ASSERT_TRUE(vanne.estOuverte());
}

void test_ouvrir_si_deja_ouverte_sans_effet()
{
    vanne.ouvrir();
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_ASSERT_TRUE(vanne.estOuverte());
}

// ============================================================
// Tests logique fermer()
// ============================================================

void test_fermer_demarre_impulsion()
{
    vanne.fermer();
    TEST_ASSERT_TRUE(vanne.estEnCours());
}

void test_fermer_pendant_impulsion_sans_effet()
{
    vanne.fermer();
    TEST_ASSERT_TRUE(vanne.estEnCours());
}

void test_fermer_complete_apres_impulsion()
{
    delay(150);
    vanne.update();
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_ASSERT_FALSE(vanne.estOuverte());
}

void test_fermer_si_deja_fermee_sans_effet()
{
    vanne.fermer();
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_ASSERT_FALSE(vanne.estOuverte());
}

// ============================================================
// Tests setDureeImpulsion()
// ============================================================

void test_setDureeImpulsion_ne_crash_pas()
{
    vanne.setDureeImpulsion(500);
    vanne.setDureeImpulsion(100);   // remet à 100ms pour la suite
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_PASS();
}

void test_setDureeImpulsion_respectee()
{
    vanne.setDureeImpulsion(100);

    vanne.ouvrir();
    TEST_ASSERT_TRUE(vanne.estEnCours());

    // À 50ms : impulsion encore en cours
    delay(50);
    vanne.update();
    TEST_ASSERT_TRUE(vanne.estEnCours());

    // À 150ms : impulsion terminée
    delay(100);
    vanne.update();
    TEST_ASSERT_FALSE(vanne.estEnCours());
    TEST_ASSERT_TRUE(vanne.estOuverte());
}

// ============================================================
// Tests update() sans impulsion en cours
// ============================================================

void test_update_sans_impulsion_sans_effet()
{
    // Fermer pour repartir d'un état connu
    vanne.fermer();
    delay(150);
    vanne.update();

    bool etatAvant = vanne.estOuverte();
    vanne.update();
    vanne.update();

    TEST_ASSERT_EQUAL(etatAvant, vanne.estOuverte());
    TEST_ASSERT_FALSE(vanne.estEnCours());
}

// ============================================================
// Test indépendance des vannes
// Vérifie que deux instances sur broches de sélection différentes
// ont des états logiques indépendants
// ============================================================

void test_deux_vannes_etats_independants()
{
    Vanne vanne2(PIN_SEL_V2, PIN_INA, PIN_INB, 100);

    // vanne1 est fermée (état laissé par les tests précédents)
    // on ouvre vanne2
    vanne2.ouvrir();
    delay(150);
    vanne2.update();

    TEST_ASSERT_FALSE(vanne.estOuverte());   // vanne1 toujours fermée
    TEST_ASSERT_TRUE(vanne2.estOuverte());   // vanne2 ouverte
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    // ── État initial ───────────────────────────────────────
    RUN_TEST(test_etat_initial_fermee);
    RUN_TEST(test_etat_initial_pas_en_cours);

    // ── Ouverture ──────────────────────────────────────────
    RUN_TEST(test_ouvrir_demarre_impulsion);
    RUN_TEST(test_ouvrir_pendant_impulsion_sans_effet);
    RUN_TEST(test_ouvrir_complete_apres_impulsion);
    RUN_TEST(test_ouvrir_si_deja_ouverte_sans_effet);

    // ── Fermeture ──────────────────────────────────────────
    RUN_TEST(test_fermer_demarre_impulsion);
    RUN_TEST(test_fermer_pendant_impulsion_sans_effet);
    RUN_TEST(test_fermer_complete_apres_impulsion);
    RUN_TEST(test_fermer_si_deja_fermee_sans_effet);

    // ── Configuration ──────────────────────────────────────
    RUN_TEST(test_setDureeImpulsion_ne_crash_pas);
    RUN_TEST(test_setDureeImpulsion_respectee);

    // ── update() ───────────────────────────────────────────
    RUN_TEST(test_update_sans_impulsion_sans_effet);

    // ── Indépendance des vannes ────────────────────────────
    RUN_TEST(test_deux_vannes_etats_independants);

    UNITY_END();
}

void loop() {}