// test/test_ServeurWebSocket/test_ServeurWebSocket.cpp

#include <Arduino.h>
#include <unity.h>
#include <WiFi.h>
#include <esp_task_wdt.h>         
#include "AfficheurOLED.h"
#include "ServeurWebSocket.h"
#include "secrets.h"

#define SSID_TEST             SSID_STA
#define PASSWORD_TEST         PASSWORD_STA
#define TIMEOUT_CONNEXION_MS  12000
#define TIMEOUT_CLIENT_MS     30000

AfficheurOLED    afficheur;
ServeurWebSocket serveur(afficheur);
static String    derniereTrameRecue  = "";  

// ── Flag : STA déjà démarrée ─────────────────────────────────────────────────
static bool staDejaDemare = false;

static bool attendreConnexionWifi(uint32_t _timeoutMs)
{
    uint32_t debut = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - debut > _timeoutMs)
            return false;
        esp_task_wdt_reset();      // nourrit le watchdog
        delay(200);
    }
    return true;
}

void setUp()    {}
void tearDown() {}

// ─────────────────────────────────────────────────────────────────────────────
void test_demarrerSTA_connexion_reussie()
{
    serveur.demarrerSTA(SSID_TEST, PASSWORD_TEST);
    bool connecte = attendreConnexionWifi(TIMEOUT_CONNEXION_MS);
    staDejaDemare = connecte;

    TEST_ASSERT_TRUE_MESSAGE(connecte, "L'ESP32 n'a pas pu rejoindre le reseau STA");

    IPAddress ip = WiFi.localIP();
    TEST_ASSERT_FALSE_MESSAGE(ip == IPAddress(0,0,0,0), "IP locale invalide");

    Serial.print("IP obtenue : ");
    Serial.println(ip);
}

void test_clientConnecte_faux_au_demarrage()
{
    TEST_ASSERT_FALSE(serveur.clientConnecte());
}

void test_setOnMessage_enregistrement()
{
    serveur.setOnMessage([](const String &_msg) {
        Serial.print("Message recu : ");
        Serial.println(_msg);
    });
    TEST_PASS();
}

void test_envoyerMessage_sans_client()
{
    TEST_ASSERT_FALSE(serveur.clientConnecte());
    serveur.envoyerMessage("{\"cmd\":\"test\"}");
    TEST_PASS();
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST INTERACTIF — connexion client
// Le serveur est DÉJÀ démarré au test 1, on ne le redémarre pas.
// ─────────────────────────────────────────────────────────────────────────────
void test_client_connexion_detectee()
{
    if (!staDejaDemare)
    {
        TEST_IGNORE_MESSAGE("STA non connectee, test ignore");
        return;
    }

    // ── Poser le callback AVANT d'attendre le client ──────────────────────
    
    derniereTrameRecue = "";

    serveur.setOnMessage([](const String &_msg)
    {
        derniereTrameRecue = _msg;
        Serial.print("Trame recue : ");
        Serial.println(_msg);
    });
    // ─────────────────────────────────────────────────────────────────────

    String urlWs = "ws://" + WiFi.localIP().toString() + ":5000/ws";
    Serial.println("\n>>> Connecter le client WebSocket maintenant (30s) <<<");
    Serial.println(">>> " + urlWs + " <<<");

    uint32_t debut   = millis();
    uint32_t dernier = millis();

    while (!serveur.clientConnecte() && millis() - debut < TIMEOUT_CLIENT_MS)
    {
        esp_task_wdt_reset();
        if (millis() - dernier >= 5000)
        {
            uint32_t restant = (TIMEOUT_CLIENT_MS - (millis() - debut)) / 1000;
            Serial.print("En attente... ");
            Serial.print(restant);
            Serial.println("s — " + urlWs);
            dernier = millis();
        }
        delay(200);
    }

    TEST_ASSERT_TRUE_MESSAGE(
        serveur.clientConnecte(),
        "Aucun client WebSocket detecte dans le delai imparti"
    );
}

void test_reception_message_client()
{
    if (!serveur.clientConnecte())
    {
        TEST_IGNORE_MESSAGE("Aucun client connecte");
        return;
    }

    uint32_t debut = millis();
    while (derniereTrameRecue.isEmpty() && millis() - debut < 5000)
    {
        esp_task_wdt_reset();
        delay(200);
    }

    TEST_ASSERT_FALSE_MESSAGE(
        derniereTrameRecue.isEmpty(),
        "Aucun message recu depuis le client"
    );
    Serial.print("Trame validee : ");
    Serial.println(derniereTrameRecue);
    // Vérifie que la trame contient bien une requête de type "S"
    TEST_ASSERT_TRUE_MESSAGE(
        derniereTrameRecue.indexOf("\"c\":\"S\"") >= 0,
        "La trame recue ne contient pas la commande S attendue"
    );
}

void test_envoyerMessage_avec_client()
{
    if (!serveur.clientConnecte())
    {
        TEST_IGNORE_MESSAGE("Aucun client connecte");
        return;
    }

    // On répond avec une trame JSON simulant l'état de deux vannes
    // Format conforme au protocole ArrosageProtocole
    serveur.envoyerMessage(
        "{\"v\":1,\"t\":\"r\",\"c\":\"S\","
        "\"h\":\"2026-01-01T00:00:00\","
        "\"s\":["
            "{\"i\":1,\"e\":\"F\",\"m\":\"P\",\"h\":\"2026-01-01T00:00:00\",\"d\":15,\"f\":24},"
            "{\"i\":2,\"e\":\"O\",\"m\":\"M\",\"h\":\"2026-01-01T00:00:00\",\"d\":30,\"f\":12}"
        "]}"
    );

    delay(500);
    Serial.println(">>> Verifier reception de la trame d'etat cote client Qt <<<");
    TEST_PASS();
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Désactiver le watchdog sur le cœur courant pour les longues attentes
    esp_task_wdt_init(60, false);   // timeout 60s, pas de panic

    afficheur.initialiser();

    UNITY_BEGIN();
    RUN_TEST(test_demarrerSTA_connexion_reussie);
    RUN_TEST(test_clientConnecte_faux_au_demarrage);
    RUN_TEST(test_setOnMessage_enregistrement);
    RUN_TEST(test_envoyerMessage_sans_client);
    RUN_TEST(test_client_connexion_detectee);
    RUN_TEST(test_reception_message_client);
    RUN_TEST(test_envoyerMessage_avec_client);
    UNITY_END();
}

void loop() {}