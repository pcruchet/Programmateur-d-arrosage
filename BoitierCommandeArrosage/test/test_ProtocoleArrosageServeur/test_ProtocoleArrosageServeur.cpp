#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "ProtocoleArrosageServeur.h"

ProtocoleArrosageServeur protocole;

void setUp()    {}
void tearDown() {}

// ============================================================
// Helpers
// ============================================================

// Parse une trame JSON produite par la classe et retourne l'objet
static JsonDocument parserTrame(const String &_trame)
{
    JsonDocument document;
    deserializeJson(document, _trame);
    return document;
}

// ============================================================
// Tests decoder()
// ============================================================

void test_decoder_trame_valide_query_system()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":1,\"t\":\"q\",\"c\":\"S\"}");

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL_STRING("q", requete.type.c_str());
    TEST_ASSERT_EQUAL_STRING("S", requete.commande.c_str());
    TEST_ASSERT_EQUAL(-1, requete.idVanne);
}

void test_decoder_trame_valide_commande_ouvrir()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":1,\"t\":\"c\",\"c\":\"O\",\"i\":2}");

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL_STRING("c", requete.type.c_str());
    TEST_ASSERT_EQUAL_STRING("O", requete.commande.c_str());
    TEST_ASSERT_EQUAL(2, requete.idVanne);
}

void test_decoder_trame_valide_set_mode()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":1,\"t\":\"c\",\"c\":\"m\",\"i\":1,\"m\":\"A\"}");

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL_STRING("m", requete.commande.c_str());
    TEST_ASSERT_EQUAL(1, requete.idVanne);
    TEST_ASSERT_EQUAL_STRING("A", requete.mode.c_str());
}

void test_decoder_trame_valide_set_programmation()
{
    RequeteArrosage requete = protocole.decoder(
        "{\"v\":1,\"t\":\"c\",\"c\":\"p\",\"i\":1,"
        "\"h\":\"2026-07-08T18:30:00\",\"d\":15,\"f\":24}"
    );

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL_STRING("p", requete.commande.c_str());
    TEST_ASSERT_EQUAL(1, requete.idVanne);
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", requete.heure.c_str());
    TEST_ASSERT_EQUAL(15, requete.duree);
    TEST_ASSERT_EQUAL(24, requete.frequence);
}

void test_decoder_trame_valide_set_time()
{
    RequeteArrosage requete = protocole.decoder(
        "{\"v\":1,\"t\":\"c\",\"c\":\"t\",\"h\":\"2026-07-08T18:30:00\"}"
    );

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL_STRING("t", requete.commande.c_str());
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", requete.heure.c_str());
}

void test_decoder_trame_json_malformee()
{
    RequeteArrosage requete = protocole.decoder("ceci n'est pas du json");

    TEST_ASSERT_FALSE(requete.valide);
}

void test_decoder_trame_version_incorrecte()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":2,\"t\":\"q\",\"c\":\"S\"}");

    TEST_ASSERT_FALSE(requete.valide);
}

void test_decoder_trame_champ_c_absent()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":1,\"t\":\"q\"}");

    TEST_ASSERT_FALSE(requete.valide);
}

void test_decoder_trame_vide()
{
    RequeteArrosage requete = protocole.decoder("");

    TEST_ASSERT_FALSE(requete.valide);
}

void test_decoder_valeurs_par_defaut_si_champs_absents()
{
    RequeteArrosage requete = protocole.decoder("{\"v\":1,\"t\":\"q\",\"c\":\"G\"}");

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL(-1, requete.idVanne);
    TEST_ASSERT_EQUAL(0,  requete.duree);
    TEST_ASSERT_EQUAL(0,  requete.frequence);
    TEST_ASSERT_EQUAL_STRING("", requete.mode.c_str());
    TEST_ASSERT_EQUAL_STRING("", requete.heure.c_str());
}

// ============================================================
// Tests creerReponseHeure()
// ============================================================

void test_creerReponseHeure_structure()
{
    String trame = protocole.creerReponseHeure("2026-07-08T18:30:00");
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL(1,    doc["v"].as<int>());
    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("T", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", doc["h"].as<const char*>());
}

// ============================================================
// Tests creerReponseEtat()
// ============================================================

void test_creerReponseEtat_vanne_fermee()
{
    String trame = protocole.creerReponseEtat(1, ProtocoleArrosageServeur::ETAT_FERMEE);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("E", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL_STRING("F", doc["e"].as<const char*>());
}

void test_creerReponseEtat_vanne_ouverte()
{
    String trame = protocole.creerReponseEtat(2, ProtocoleArrosageServeur::ETAT_OUVERTE);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL(2,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL_STRING("O", doc["e"].as<const char*>());
}

// ============================================================
// Tests creerReponseMode()
// ============================================================

void test_creerReponseMode_programme()
{
    String trame = protocole.creerReponseMode(1, ProtocoleArrosageServeur::MODE_PROGRAMME);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("M", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL_STRING("P", doc["m"].as<const char*>());
}

// ============================================================
// Tests creerReponseProgrammation()
// ============================================================

void test_creerReponseProgrammation_structure()
{
    String trame = protocole.creerReponseProgrammation(1, "2026-07-08T18:30:00", 15, 24);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("P", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", doc["h"].as<const char*>());
    TEST_ASSERT_EQUAL(15,         doc["d"].as<int>());
    TEST_ASSERT_EQUAL(24,         doc["f"].as<int>());
}

// ============================================================
// Tests creerReponsePing()
// ============================================================

void test_creerReponsePing_structure()
{
    String trame = protocole.creerReponsePing();
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL(1,          doc["v"].as<int>());
    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("G", doc["c"].as<const char*>());
}

// ============================================================
// Tests creerReponseSysteme()
// ============================================================

void test_creerReponseSysteme_structure()
{
    JsonDocument vannesDoc;
    JsonArray vannes = vannesDoc.to<JsonArray>();

    JsonObject v1 = vannes.add<JsonObject>();
    v1["i"] = 1;
    v1["e"] = "F";
    v1["m"] = "P";
    v1["h"] = "2026-07-08T18:30:00";
    v1["d"] = 15;
    v1["f"] = 24;

    JsonObject v2 = vannes.add<JsonObject>();
    v2["i"] = 2;
    v2["e"] = "O";
    v2["m"] = "M";
    v2["h"] = "2026-07-08T06:00:00";
    v2["d"] = 30;
    v2["f"] = 12;

    String trame = protocole.creerReponseSysteme("2026-07-08T18:30:00", vannes);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("r", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("S", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("2026-07-08T18:30:00", doc["h"].as<const char*>());
    TEST_ASSERT_EQUAL(2, doc["s"].as<JsonArray>().size());
    TEST_ASSERT_EQUAL(1, doc["s"][0]["i"].as<int>());
    TEST_ASSERT_EQUAL(2, doc["s"][1]["i"].as<int>());
}

// ============================================================
// Tests creerAck()
// ============================================================

void test_creerAck_set_time()
{
    String trame = protocole.creerAck(ProtocoleArrosageServeur::CMD_SET_TIME);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("a", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("t", doc["c"].as<const char*>());
}

void test_creerAckVanne_ouvrir()
{
    String trame = protocole.creerAckVanne(ProtocoleArrosageServeur::CMD_OUVRIR, 1);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("a", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("O", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
}

// ============================================================
// Tests creerErreur()
// ============================================================

void test_creerErreur_vanne_inexistante()
{
    String trame = protocole.creerErreur(
        ProtocoleArrosageServeur::CMD_GET_ETAT,
        ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE
    );
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("e", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("E", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["x"].as<int>());
}

void test_creerErreur_mode_invalide()
{
    String trame = protocole.creerErreur(
        ProtocoleArrosageServeur::CMD_SET_MODE,
        ProtocoleArrosageServeur::ERREUR_MODE_INVALIDE
    );
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL(2, doc["x"].as<int>());
}

// ============================================================
// Tests notifications
// ============================================================

void test_creerNotifEtatVanne_ouverture()
{
    String trame = protocole.creerNotifEtatVanne(ProtocoleArrosageServeur::CMD_OUVRIR, 1);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("n", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("O", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
}

void test_creerNotifMode_automatique()
{
    String trame = protocole.creerNotifMode(1, ProtocoleArrosageServeur::MODE_AUTOMATIQUE);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("n", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("M", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL_STRING("A", doc["m"].as<const char*>());
}

void test_creerNotifProgrammation_structure()
{
    String trame = protocole.creerNotifProgrammation(2, "2026-07-08T06:00:00", 30, 12);
    JsonDocument doc = parserTrame(trame);

    TEST_ASSERT_EQUAL_STRING("n", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("P", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(2,          doc["i"].as<int>());
    TEST_ASSERT_EQUAL(30,         doc["d"].as<int>());
    TEST_ASSERT_EQUAL(12,         doc["f"].as<int>());
}

// ============================================================
// Test aller-retour : decoder() ↔ creerXxx()
// Vérifie la cohérence encodage/décodage de bout en bout
// ============================================================

void test_aller_retour_set_programmation()
{
    // Simule une trame reçue du client Qt
    String trameRecue =
        "{\"v\":1,\"t\":\"c\",\"c\":\"p\",\"i\":1,"
        "\"h\":\"2026-07-08T18:30:00\",\"d\":15,\"f\":24}";

    RequeteArrosage requete = protocole.decoder(trameRecue);

    // Simule la réponse construite par l'ESP32
    String reponse = protocole.creerAckVanne(
        requete.commande[0],
        requete.idVanne
    );

    JsonDocument doc = parserTrame(reponse);

    TEST_ASSERT_EQUAL_STRING("a", doc["t"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("p", doc["c"].as<const char*>());
    TEST_ASSERT_EQUAL(1,          doc["i"].as<int>());
}

void test_aller_retour_get_system()
{
    String trameRecue = "{\"v\":1,\"t\":\"q\",\"c\":\"S\"}";
    RequeteArrosage requete = protocole.decoder(trameRecue);

    TEST_ASSERT_TRUE(requete.valide);
    TEST_ASSERT_EQUAL('S', requete.commande[0]);
}

// ============================================================
// Point d'entrée Unity
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    // ── decoder() ─────────────────────────────────────────
    RUN_TEST(test_decoder_trame_valide_query_system);
    RUN_TEST(test_decoder_trame_valide_commande_ouvrir);
    RUN_TEST(test_decoder_trame_valide_set_mode);
    RUN_TEST(test_decoder_trame_valide_set_programmation);
    RUN_TEST(test_decoder_trame_valide_set_time);
    RUN_TEST(test_decoder_trame_json_malformee);
    RUN_TEST(test_decoder_trame_version_incorrecte);
    RUN_TEST(test_decoder_trame_champ_c_absent);
    RUN_TEST(test_decoder_trame_vide);
    RUN_TEST(test_decoder_valeurs_par_defaut_si_champs_absents);

    // ── creerReponseXxx() ─────────────────────────────────
    RUN_TEST(test_creerReponseHeure_structure);
    RUN_TEST(test_creerReponseEtat_vanne_fermee);
    RUN_TEST(test_creerReponseEtat_vanne_ouverte);
    RUN_TEST(test_creerReponseMode_programme);
    RUN_TEST(test_creerReponseProgrammation_structure);
    RUN_TEST(test_creerReponsePing_structure);
    RUN_TEST(test_creerReponseSysteme_structure);

    // ── creerAck() ────────────────────────────────────────
    RUN_TEST(test_creerAck_set_time);
    RUN_TEST(test_creerAckVanne_ouvrir);

    // ── creerErreur() ─────────────────────────────────────
    RUN_TEST(test_creerErreur_vanne_inexistante);
    RUN_TEST(test_creerErreur_mode_invalide);

    // ── Notifications ─────────────────────────────────────
    RUN_TEST(test_creerNotifEtatVanne_ouverture);
    RUN_TEST(test_creerNotifMode_automatique);
    RUN_TEST(test_creerNotifProgrammation_structure);

    // ── Aller-retour ──────────────────────────────────────
    RUN_TEST(test_aller_retour_set_programmation);
    RUN_TEST(test_aller_retour_get_system);

    UNITY_END();
}

void loop() {}