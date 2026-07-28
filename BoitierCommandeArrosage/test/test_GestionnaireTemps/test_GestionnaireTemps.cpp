// test/test_GestionnaireTemps/test_GestionnaireTemps.cpp

#include <Arduino.h>
#include <Wire.h>
#include <unity.h>
#include "GestionnaireTemps.h"

#define BROCHE_IRQ  19

GestionnaireTemps gestionnaireTemps(BROCHE_IRQ);

void setUp()    {}
void tearDown() {}

// ============================================================
// Tests initialisation
// ============================================================

void test_initialiser_reussit()
{
    bool resultat = gestionnaireTemps.initialiser();

    TEST_ASSERT_TRUE_MESSAGE(resultat, "DS3231 non detecte sur le bus I2C");
}

void test_rtcDisponible_apres_init()
{
    TEST_ASSERT_TRUE(gestionnaireTemps.rtcDisponible());
}

// ============================================================
// Tests lecture / écriture heure système
// ============================================================

void test_ecrireHeureSysteme_puis_lire()
{
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T18:30:00");

    String heureLue = gestionnaireTemps.lireHeureSysteme();

    // On compare jusqu'à la minute : l'écriture/lecture peut
    // traverser une frontière de seconde entre les deux appels
    String heureSansSecondes = heureLue.substring(0, 16);

    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30", heureSansSecondes.c_str());
}

void test_lireHeureSysteme_format_iso8601()
{
    String heure = gestionnaireTemps.lireHeureSysteme();

    // Format attendu : YYYY-MM-DDTHH:MM:SS (19 caractères)
    TEST_ASSERT_EQUAL(19, heure.length());
    TEST_ASSERT_EQUAL('-', heure.charAt(4));
    TEST_ASSERT_EQUAL('-', heure.charAt(7));
    TEST_ASSERT_EQUAL('T', heure.charAt(10));
    TEST_ASSERT_EQUAL(':', heure.charAt(13));
    TEST_ASSERT_EQUAL(':', heure.charAt(16));
}

// ============================================================
// Tests ajouterMinutes() — logique pure
// ============================================================

void test_ajouterMinutes_meme_heure()
{
    String resultat = gestionnaireTemps.ajouterMinutes("2026-07-08T18:30:00", 15);

    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:45:00", resultat.c_str());
}

void test_ajouterMinutes_passage_heure_suivante()
{
    String resultat = gestionnaireTemps.ajouterMinutes("2026-07-08T18:50:00", 15);

    TEST_ASSERT_EQUAL_STRING("2026-07-08T19:05:00", resultat.c_str());
}

void test_ajouterMinutes_passage_jour_suivant()
{
    String resultat = gestionnaireTemps.ajouterMinutes("2026-07-08T23:50:00", 30);

    TEST_ASSERT_EQUAL_STRING("2026-07-09T00:20:00", resultat.c_str());
}

void test_ajouterMinutes_chaine_vide_retourne_vide()
{
    String resultat = gestionnaireTemps.ajouterMinutes("", 15);

    TEST_ASSERT_EQUAL_STRING("", resultat.c_str());
}

// ============================================================
// Tests calculerProchaineAlarme() — logique pure
// ============================================================

void test_calculerProchaineAlarme_mode_manuel_retourne_vide()
{
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T12:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('M', "2026-07-08T18:30:00", 24);

    TEST_ASSERT_EQUAL_STRING("", resultat.c_str());
}

void test_calculerProchaineAlarme_frequence_nulle_retourne_vide()
{
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T12:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('P', "2026-07-08T18:30:00", 0);

    TEST_ASSERT_EQUAL_STRING("", resultat.c_str());
}

void test_calculerProchaineAlarme_premiere_occurrence_future()
{
    // Heure système avant l'heure de début programmée
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T10:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('P', "2026-07-08T18:30:00", 24);

    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", resultat.c_str());
}

void test_calculerProchaineAlarme_avance_dun_jour()
{
    // Heure système après l'heure de début programmée du même jour
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T20:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('P', "2026-07-08T18:30:00", 24);

    TEST_ASSERT_EQUAL_STRING("2026-07-09T18:30:00", resultat.c_str());
}

void test_calculerProchaineAlarme_rattrape_plusieurs_cycles_manques()
{
    // Simule une coupure secteur de 3 jours : heure système très en avance
    // sur la dernière date connue, fréquence 24h
    gestionnaireTemps.ecrireHeureSysteme("2026-07-11T09:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('P', "2026-07-08T18:30:00", 24);

    // Doit sauter les occurrences passées (8, 9, 10 juillet) et proposer le 11
    TEST_ASSERT_EQUAL_STRING("2026-07-11T18:30:00", resultat.c_str());
}

void test_calculerProchaineAlarme_frequence_courte()
{
    // Fréquence de 6h, plusieurs occurrences possibles dans la journée
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T07:00:00");

    String resultat = gestionnaireTemps.calculerProchaineAlarme('P', "2026-07-08T06:00:00", 6);

    TEST_ASSERT_EQUAL_STRING("2026-07-08T12:00:00", resultat.c_str());
}

// ============================================================
// Tests alarme matérielle (DS3231)
// ============================================================

void test_programmerAlarme_puis_effacer_sans_crash()
{
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T12:00:00");

    gestionnaireTemps.programmerAlarme("2026-07-08T12:05:00");
    gestionnaireTemps.effacerAlarme();

    TEST_PASS();
}

void test_alarmeDeclenchee_faux_juste_apres_programmation()
{
    gestionnaireTemps.ecrireHeureSysteme("2026-07-08T12:00:00");
    gestionnaireTemps.programmerAlarme("2026-07-08T12:05:00");

    TEST_ASSERT_FALSE(gestionnaireTemps.alarmeDeclenchee());

    gestionnaireTemps.effacerAlarme();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test interactif : déclenchement réel de l'alarme
// On programme une alarme 10 secondes dans le futur et on attend.
// ─────────────────────────────────────────────────────────────────────────────
void test_alarmeDeclenchee_apres_delai_reel()
{
    DateTime maintenant_inutilise;  // non utilisé, juste pour lisibilité du test

    // Récupère l'heure système réelle de la RTC (pas une heure simulée)
    gestionnaireTemps.ecrireHeureSysteme(gestionnaireTemps.lireHeureSysteme());
    String heureActuelle = gestionnaireTemps.lireHeureSysteme();

    // Programme une alarme 10 secondes plus tard
    String heureAlarme = gestionnaireTemps.ajouterMinutes(heureActuelle, 0);
    // ajout de 10 secondes via reconstruction manuelle (pas de méthode ajouterSecondes)
    int secondes = heureAlarme.substring(17, 19).toInt();
    secondes += 10;
    String basePrefixe = heureAlarme.substring(0, 17);
    char secTampon[3];
    sprintf(secTampon, "%02d", secondes % 60);
    heureAlarme = basePrefixe + String(secTampon);

    Serial.print(">>> Heure actuelle  : ");
    Serial.println(heureActuelle);
    Serial.print(">>> Alarme programmee a : ");
    Serial.println(heureAlarme);
    Serial.println(">>> Attente du declenchement (15s max) <<<");

    gestionnaireTemps.programmerAlarme(heureAlarme);

    uint32_t debut = millis();
    bool declenchee = false;

    while (!declenchee && millis() - debut < 15000)
    {
        declenchee = gestionnaireTemps.alarmeDeclenchee();
        delay(200);
    }

    TEST_ASSERT_TRUE_MESSAGE(declenchee, "L'alarme DS3231 ne s'est pas declenchee dans le delai imparti");

    gestionnaireTemps.effacerAlarme();
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin();

    UNITY_BEGIN();

    // ── Initialisation ──────────────────────────────────────
    RUN_TEST(test_initialiser_reussit);
    RUN_TEST(test_rtcDisponible_apres_init);

    // ── Lecture / écriture heure ──────────────────────────────
    RUN_TEST(test_ecrireHeureSysteme_puis_lire);
    RUN_TEST(test_lireHeureSysteme_format_iso8601);

    // ── ajouterMinutes() ──────────────────────────────────────
    RUN_TEST(test_ajouterMinutes_meme_heure);
    RUN_TEST(test_ajouterMinutes_passage_heure_suivante);
    RUN_TEST(test_ajouterMinutes_passage_jour_suivant);
    RUN_TEST(test_ajouterMinutes_chaine_vide_retourne_vide);

    // ── calculerProchaineAlarme() ────────────────────────────
    RUN_TEST(test_calculerProchaineAlarme_mode_manuel_retourne_vide);
    RUN_TEST(test_calculerProchaineAlarme_frequence_nulle_retourne_vide);
    RUN_TEST(test_calculerProchaineAlarme_premiere_occurrence_future);
    RUN_TEST(test_calculerProchaineAlarme_avance_dun_jour);
    RUN_TEST(test_calculerProchaineAlarme_rattrape_plusieurs_cycles_manques);
    RUN_TEST(test_calculerProchaineAlarme_frequence_courte);

    // ── Alarme matérielle ─────────────────────────────────────
    RUN_TEST(test_programmerAlarme_puis_effacer_sans_crash);
    RUN_TEST(test_alarmeDeclenchee_faux_juste_apres_programmation);
    RUN_TEST(test_alarmeDeclenchee_apres_delai_reel);   // interactif, ~15s

    // Remettre l'heure système correcte avant de quitter les tests
    // (les tests précédents ont modifié la RTC avec des dates arbitraires)
    // À ajuster à la date/heure réelle si besoin :
    // gestionnaireTemps.ecrireHeureSysteme("2026-06-30T12:00:00");

    UNITY_END();
}

void loop() {}