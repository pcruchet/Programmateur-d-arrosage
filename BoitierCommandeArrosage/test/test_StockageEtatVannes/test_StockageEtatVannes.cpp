// test/test_StockageEtatVannes/test_StockageEtatVannes.cpp

#include <Arduino.h>
#include <unity.h>
#include "StockageEtatVannes.h"

StockageEtatVannes stockageEtat;

void setUp()    { stockageEtat.reinitialiserTout(); }
void tearDown() { stockageEtat.reinitialiserTout(); }

// ============================================================
// Tests initialisation
// ============================================================

void test_initialiser_reussit()
{
    bool resultat = stockageEtat.initialiser();

    TEST_ASSERT_TRUE(resultat);
}

// ============================================================
// Tests valeurs par défaut
// ============================================================

void test_lireEtat_valeurs_defaut()
{
    EtatVanne etat = stockageEtat.lireEtat(1);

    TEST_ASSERT_FALSE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("", etat.heureOuverture.c_str());
}

void test_vanneOuverte_faux_par_defaut()
{
    TEST_ASSERT_FALSE(stockageEtat.vanneOuverte(1));
}

void test_uneVanneOuverte_faux_par_defaut()
{
    TEST_ASSERT_FALSE(stockageEtat.uneVanneOuverte());
}

// ============================================================
// Tests sauvegarderOuverture()
// ============================================================

void test_sauvegarderOuverture_puis_lire()
{
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T18:30:00");

    EtatVanne etat = stockageEtat.lireEtat(1);

    TEST_ASSERT_TRUE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", etat.heureOuverture.c_str());
}

void test_sauvegarderOuverture_vanneOuverte_retourne_vrai()
{
    stockageEtat.sauvegarderOuverture(2, "2026-07-08T10:00:00");

    TEST_ASSERT_TRUE(stockageEtat.vanneOuverte(2));
}

void test_sauvegarderOuverture_uneVanneOuverte_retourne_vrai()
{
    stockageEtat.sauvegarderOuverture(3, "2026-07-08T06:00:00");

    TEST_ASSERT_TRUE(stockageEtat.uneVanneOuverte());
}

// ============================================================
// Tests sauvegarderFermeture()
// ============================================================

void test_sauvegarderFermeture_apres_ouverture()
{
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T18:30:00");
    stockageEtat.sauvegarderFermeture(1);

    EtatVanne etat = stockageEtat.lireEtat(1);

    TEST_ASSERT_FALSE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("", etat.heureOuverture.c_str());
}

void test_sauvegarderFermeture_vanneOuverte_retourne_faux()
{
    stockageEtat.sauvegarderOuverture(2, "2026-07-08T10:00:00");
    stockageEtat.sauvegarderFermeture(2);

    TEST_ASSERT_FALSE(stockageEtat.vanneOuverte(2));
}

// ============================================================
// Tests isolation entre vannes
// ============================================================

void test_ouverture_vanne1_naffecte_pas_vanne2()
{
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T08:00:00");

    TEST_ASSERT_TRUE(stockageEtat.vanneOuverte(1));
    TEST_ASSERT_FALSE(stockageEtat.vanneOuverte(2));
}

void test_uneVanneOuverte_avec_plusieurs_vannes()
{
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T08:00:00");
    stockageEtat.sauvegarderOuverture(3, "2026-07-08T09:00:00");

    TEST_ASSERT_TRUE(stockageEtat.uneVanneOuverte());

    stockageEtat.sauvegarderFermeture(1);
    TEST_ASSERT_TRUE(stockageEtat.uneVanneOuverte());   // vanne 3 encore ouverte

    stockageEtat.sauvegarderFermeture(3);
    TEST_ASSERT_FALSE(stockageEtat.uneVanneOuverte());  // toutes fermées
}

// ============================================================
// Tests delaiDepasse()
// ============================================================

void test_delaiDepasse_faux_dans_le_delai()
{
    // Ouverture à 18h00, durée 30 min, marge 15 min → limite à 18h45
    // Heure actuelle 18h30 → pas encore dépassé
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T18:00:00");

    bool depasse = stockageEtat.delaiDepasse(1, 30, "2026-07-08T18:30:00");

    TEST_ASSERT_FALSE(depasse);
}

void test_delaiDepasse_vrai_au_dela_de_la_marge()
{
    // Ouverture à 18h00, durée 30 min, marge 15 min → limite à 18h45
    // Heure actuelle 19h00 → dépassé
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T18:00:00");

    bool depasse = stockageEtat.delaiDepasse(1, 30, "2026-07-08T19:00:00");

    TEST_ASSERT_TRUE(depasse);
}

void test_delaiDepasse_faux_si_vanne_fermee()
{
    stockageEtat.sauvegarderFermeture(1);

    bool depasse = stockageEtat.delaiDepasse(1, 30, "2026-07-08T20:00:00");

    TEST_ASSERT_FALSE(depasse);
}

void test_delaiDepasse_exactement_a_la_limite()
{
    // Ouverture à 18h00, durée 30 min, marge 15 min → limite exacte à 18h45
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T18:00:00");

    bool depasse = stockageEtat.delaiDepasse(1, 30, "2026-07-08T18:45:00");

    TEST_ASSERT_TRUE(depasse);
}

// ============================================================
// Tests réinitialisation
// ============================================================

void test_reinitialiserVanne_retour_defaut()
{
    stockageEtat.sauvegarderOuverture(2, "2026-07-08T12:00:00");
    stockageEtat.reinitialiserVanne(2);

    EtatVanne etat = stockageEtat.lireEtat(2);

    TEST_ASSERT_FALSE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("", etat.heureOuverture.c_str());
}

void test_reinitialiserTout_ferme_toutes_vannes()
{
    stockageEtat.sauvegarderOuverture(1, "2026-07-08T08:00:00");
    stockageEtat.sauvegarderOuverture(2, "2026-07-08T09:00:00");
    stockageEtat.sauvegarderOuverture(3, "2026-07-08T10:00:00");
    stockageEtat.sauvegarderOuverture(4, "2026-07-08T11:00:00");

    stockageEtat.reinitialiserTout();

    TEST_ASSERT_FALSE(stockageEtat.uneVanneOuverte());
}

// ============================================================
// Tests id invalide
// ============================================================

void test_lireEtat_id_invalide_retourne_defaut()
{
    EtatVanne etat = stockageEtat.lireEtat(0);

    TEST_ASSERT_FALSE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("", etat.heureOuverture.c_str());
}

void test_sauvegarderOuverture_id_invalide_sans_crash()
{
    stockageEtat.sauvegarderOuverture(0, "2026-07-08T08:00:00");
    stockageEtat.sauvegarderOuverture(5, "2026-07-08T08:00:00");

    TEST_PASS();
}

// ============================================================
// Test persistance après nouvelle instance
// ============================================================

void test_persistance_apres_nouvelle_instance()
{
    stockageEtat.sauvegarderOuverture(4, "2026-07-08T14:00:00");

    StockageEtatVannes nouvelleInstance;
    EtatVanne etat = nouvelleInstance.lireEtat(4);

    TEST_ASSERT_TRUE(etat.ouverte);
    TEST_ASSERT_EQUAL_STRING("2026-07-08T14:00:00", etat.heureOuverture.c_str());
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    stockageEtat.initialiser();

    UNITY_BEGIN();

    // ── Initialisation ──────────────────────────────────────
    RUN_TEST(test_initialiser_reussit);

    // ── Valeurs par défaut ───────────────────────────────────
    RUN_TEST(test_lireEtat_valeurs_defaut);
    RUN_TEST(test_vanneOuverte_faux_par_defaut);
    RUN_TEST(test_uneVanneOuverte_faux_par_defaut);

    // ── Ouverture ────────────────────────────────────────────
    RUN_TEST(test_sauvegarderOuverture_puis_lire);
    RUN_TEST(test_sauvegarderOuverture_vanneOuverte_retourne_vrai);
    RUN_TEST(test_sauvegarderOuverture_uneVanneOuverte_retourne_vrai);

    // ── Fermeture ────────────────────────────────────────────
    RUN_TEST(test_sauvegarderFermeture_apres_ouverture);
    RUN_TEST(test_sauvegarderFermeture_vanneOuverte_retourne_faux);

    // ── Isolation ────────────────────────────────────────────
    RUN_TEST(test_ouverture_vanne1_naffecte_pas_vanne2);
    RUN_TEST(test_uneVanneOuverte_avec_plusieurs_vannes);

    // ── Timeout de sécurité ──────────────────────────────────
    RUN_TEST(test_delaiDepasse_faux_dans_le_delai);
    RUN_TEST(test_delaiDepasse_vrai_au_dela_de_la_marge);
    RUN_TEST(test_delaiDepasse_faux_si_vanne_fermee);
    RUN_TEST(test_delaiDepasse_exactement_a_la_limite);

    // ── Réinitialisation ─────────────────────────────────────
    RUN_TEST(test_reinitialiserVanne_retour_defaut);
    RUN_TEST(test_reinitialiserTout_ferme_toutes_vannes);

    // ── Id invalide ──────────────────────────────────────────
    RUN_TEST(test_lireEtat_id_invalide_retourne_defaut);
    RUN_TEST(test_sauvegarderOuverture_id_invalide_sans_crash);

    // ── Persistance ──────────────────────────────────────────
    RUN_TEST(test_persistance_apres_nouvelle_instance);

    stockageEtat.reinitialiserTout();

    UNITY_END();
}

void loop() {}