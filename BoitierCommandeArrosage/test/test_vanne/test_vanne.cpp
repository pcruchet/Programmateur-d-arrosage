// test/test_Vanne_materiel/test_Vanne_materiel.cpp

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

// Durée réelle pour électrovannes bistables
#define DUREE_IMPULSION_MS  1000
// Délai d'observation visuelle entre chaque test
#define DELAI_OBSERVATION_MS  3000

Vanne vanne1(PIN_SEL_V1, PIN_INA, PIN_INB, DUREE_IMPULSION_MS);
Vanne vanne2(PIN_SEL_V2, PIN_INA, PIN_INB, DUREE_IMPULSION_MS);
Vanne vanne3(PIN_SEL_V3, PIN_INA, PIN_INB, DUREE_IMPULSION_MS);
Vanne vanne4(PIN_SEL_V4, PIN_INA, PIN_INB, DUREE_IMPULSION_MS);

// ── Helper : attendre la fin d'une impulsion avec appels update() ─────────────
static void attendreFinImpulsion(Vanne &_vanne)
{
    uint32_t debut = millis();
    while (_vanne.estEnCours() && millis() - debut < DUREE_IMPULSION_MS + 500)
    {
        _vanne.update();
        delay(10);
    }
}

void setUp()    {}
void tearDown() {}

// ============================================================
// VANNE 1
// ============================================================

void test_vanne1_ouverture_physique()
{
    Serial.println("\n>>> VANNE 1 : envoi impulsion OUVERTURE <<<");

    vanne1.ouvrir();
    attendreFinImpulsion(vanne1);

    Serial.println(">>> Verifier visuellement que la VANNE 1 est OUVERTE (eau qui coule) <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_TRUE_MESSAGE(vanne1.estOuverte(), "Vanne 1 : etat logique non OUVERTE apres impulsion");
    TEST_ASSERT_FALSE(vanne1.estEnCours());
}

void test_vanne1_fermeture_physique()
{
    Serial.println("\n>>> VANNE 1 : envoi impulsion FERMETURE <<<");

    vanne1.fermer();
    attendreFinImpulsion(vanne1);

    Serial.println(">>> Verifier visuellement que la VANNE 1 est FERMEE (eau arretee) <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_FALSE_MESSAGE(vanne1.estOuverte(), "Vanne 1 : etat logique non FERMEE apres impulsion");
    TEST_ASSERT_FALSE(vanne1.estEnCours());
}

// ============================================================
// VANNE 2
// ============================================================

void test_vanne2_ouverture_physique()
{
    Serial.println("\n>>> VANNE 2 : envoi impulsion OUVERTURE <<<");

    vanne2.ouvrir();
    attendreFinImpulsion(vanne2);

    Serial.println(">>> Verifier visuellement que la VANNE 2 est OUVERTE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_TRUE_MESSAGE(vanne2.estOuverte(), "Vanne 2 : etat logique non OUVERTE apres impulsion");
    TEST_ASSERT_FALSE(vanne2.estEnCours());
}

void test_vanne2_fermeture_physique()
{
    Serial.println("\n>>> VANNE 2 : envoi impulsion FERMETURE <<<");

    vanne2.fermer();
    attendreFinImpulsion(vanne2);

    Serial.println(">>> Verifier visuellement que la VANNE 2 est FERMEE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_FALSE_MESSAGE(vanne2.estOuverte(), "Vanne 2 : etat logique non FERMEE apres impulsion");
    TEST_ASSERT_FALSE(vanne2.estEnCours());
}

// ============================================================
// VANNE 3
// ============================================================

void test_vanne3_ouverture_physique()
{
    Serial.println("\n>>> VANNE 3 : envoi impulsion OUVERTURE <<<");

    vanne3.ouvrir();
    attendreFinImpulsion(vanne3);

    Serial.println(">>> Verifier visuellement que la VANNE 3 est OUVERTE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_TRUE_MESSAGE(vanne3.estOuverte(), "Vanne 3 : etat logique non OUVERTE apres impulsion");
    TEST_ASSERT_FALSE(vanne3.estEnCours());
}

void test_vanne3_fermeture_physique()
{
    Serial.println("\n>>> VANNE 3 : envoi impulsion FERMETURE <<<");

    vanne3.fermer();
    attendreFinImpulsion(vanne3);

    Serial.println(">>> Verifier visuellement que la VANNE 3 est FERMEE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_FALSE_MESSAGE(vanne3.estOuverte(), "Vanne 3 : etat logique non FERMEE apres impulsion");
    TEST_ASSERT_FALSE(vanne3.estEnCours());
}

// ============================================================
// VANNE 4
// ============================================================

void test_vanne4_ouverture_physique()
{
    Serial.println("\n>>> VANNE 4 : envoi impulsion OUVERTURE <<<");

    vanne4.ouvrir();
    attendreFinImpulsion(vanne4);

    Serial.println(">>> Verifier visuellement que la VANNE 4 est OUVERTE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_TRUE_MESSAGE(vanne4.estOuverte(), "Vanne 4 : etat logique non OUVERTE apres impulsion");
    TEST_ASSERT_FALSE(vanne4.estEnCours());
}

void test_vanne4_fermeture_physique()
{
    Serial.println("\n>>> VANNE 4 : envoi impulsion FERMETURE <<<");

    vanne4.fermer();
    attendreFinImpulsion(vanne4);

    Serial.println(">>> Verifier visuellement que la VANNE 4 est FERMEE <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_FALSE_MESSAGE(vanne4.estOuverte(), "Vanne 4 : etat logique non FERMEE apres impulsion");
    TEST_ASSERT_FALSE(vanne4.estEnCours());
}

// ============================================================
// Test isolation : une seule vanne s'active à la fois
// Ouvre V1 et vérifie que V2 reste fermée pendant l'impulsion
// ============================================================

void test_isolation_vannes_simultanees()
{
    Serial.println("\n>>> TEST ISOLATION : ouverture V1 uniquement <<<");
    Serial.println(">>> V2, V3, V4 doivent rester INACTIVES <<<");

    vanne1.ouvrir();
    attendreFinImpulsion(vanne1);

    Serial.println(">>> Verifier que SEULE la vanne 1 s'est ouverte <<<");
    delay(DELAI_OBSERVATION_MS);

    TEST_ASSERT_TRUE_MESSAGE(vanne1.estOuverte(),  "V1 aurait du s'ouvrir");
    TEST_ASSERT_FALSE_MESSAGE(vanne2.estOuverte(), "V2 ne devrait pas etre ouverte");
    TEST_ASSERT_FALSE_MESSAGE(vanne3.estOuverte(), "V3 ne devrait pas etre ouverte");
    TEST_ASSERT_FALSE_MESSAGE(vanne4.estOuverte(), "V4 ne devrait pas etre ouverte");

    // Refermer V1 pour laisser le système dans un état propre
    Serial.println("\n>>> Refermeture V1 <<<");
    vanne1.fermer();
    attendreFinImpulsion(vanne1);
    delay(DELAI_OBSERVATION_MS);
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║   TEST MATERIEL VANNES - AVEC PUISSANCE  ║");
    Serial.println("║   Duree totale estimee : ~3 minutes      ║");
    Serial.println("╚══════════════════════════════════════════╝");
    delay(3000);   // temps pour se positionner visuellement

    UNITY_BEGIN();

    // ── Vanne 1 ───────────────────────────────────────────
    RUN_TEST(test_vanne1_ouverture_physique);
    RUN_TEST(test_vanne1_fermeture_physique);

    // ── Vanne 2 ───────────────────────────────────────────
    RUN_TEST(test_vanne2_ouverture_physique);
    RUN_TEST(test_vanne2_fermeture_physique);

    // ── Vanne 3 ───────────────────────────────────────────
    RUN_TEST(test_vanne3_ouverture_physique);
    RUN_TEST(test_vanne3_fermeture_physique);

    // ── Vanne 4 ───────────────────────────────────────────
    RUN_TEST(test_vanne4_ouverture_physique);
    RUN_TEST(test_vanne4_fermeture_physique);

    // ── Isolation ─────────────────────────────────────────
    RUN_TEST(test_isolation_vannes_simultanees);

    UNITY_END();

    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║   FIN DES TESTS - Toutes vannes fermees  ║");
    Serial.println("╚══════════════════════════════════════════╝");
}

void loop() {}