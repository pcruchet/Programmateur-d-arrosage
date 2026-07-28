// test/test_AfficheurOLED/test_AfficheurOLED.cpp

#include <Arduino.h>
#include <unity.h>
#include "AfficheurOLED.h"

AfficheurOLED afficheur;

void setUp() {}    // exécuté avant chaque test
void tearDown() {} // exécuté après chaque test

// ── Un test = une fonction void sans paramètre ────────────────────────────────

void test_initialiser_retourne_vrai()
{
    bool resultat = afficheur.initialiser();
    TEST_ASSERT_TRUE(resultat);
}

void test_setModeWifi_ne_crash_pas()
{
    afficheur.setModeWifi("AP");
    TEST_PASS();
}

void test_setAdresseIP_ne_crash_pas()
{
    afficheur.setAdresseIP("192.168.4.1");
    TEST_PASS();
}

void test_setClientConnecte_ne_crash_pas()
{
    afficheur.setClientConnecte(false);
    TEST_PASS();
}

void test_setEtatVannes_4_vannes()
{
    bool etats[4] = {true, false, false, true};
    afficheur.setEtatVannes(etats, 4);
    TEST_PASS();
}

void test_setEtatVannes_tronque_au_dela_de_4()
{
    bool etats[6] = {true, true, true, true, true, true};
    afficheur.setEtatVannes(etats, 6); // ne doit pas déborder
    TEST_PASS();
}

void test_setHeureSysteme_ne_crash_pas()
{
    afficheur.setHeureSysteme("11:30");
    TEST_PASS();
}

void test_rafraichir_apres_init()
{
    afficheur.initialiser();
    bool etats[4] = {true, false, false, true};
    afficheur.setModeWifi("AP");
    afficheur.setAdresseIP("192.168.4.1");
    afficheur.setClientConnecte(false);
    afficheur.setEtatVannes(etats, 4);
    afficheur.setHeureSysteme("11:30");
    afficheur.rafraichir(); // ne doit pas crasher/rebooter
    TEST_PASS();
}

void test_eteindre_ne_crash_pas()
{
    afficheur.eteindre();
    Serial.println(">>> Verifier visuellement que l'ecran OLED est ETEINT <<<");
    delay(5000);
    TEST_PASS();
}

void test_allumer_ne_crash_pas()
{
    afficheur.allumer();
    Serial.println(">>> Verifier visuellement que l'ecran OLED est ALLUME <<<");
    delay(5000);
    TEST_PASS();
}
// ── Point d'entrée ────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(2000); // temps pour la console

    UNITY_BEGIN();

    RUN_TEST(test_initialiser_retourne_vrai);
    RUN_TEST(test_setModeWifi_ne_crash_pas);
    RUN_TEST(test_setAdresseIP_ne_crash_pas);
    RUN_TEST(test_setClientConnecte_ne_crash_pas);
    RUN_TEST(test_setEtatVannes_4_vannes);
    RUN_TEST(test_setEtatVannes_tronque_au_dela_de_4);
    RUN_TEST(test_setHeureSysteme_ne_crash_pas);
    RUN_TEST(test_rafraichir_apres_init);
    RUN_TEST(test_eteindre_ne_crash_pas);
    RUN_TEST(test_allumer_ne_crash_pas);

    UNITY_END();
}

void loop() {}