/**
 * @file    test_batterie_oled_main.cpp
 * @brief   Programme autonome : affiche l'état du pack batterie (INA219)
 *          sur l'écran OLED pendant 15 secondes à chaque démarrage, puis
 *          entre en deep sleep jusqu'au prochain appui sur BP2 (GPIO39).
 *
 * @details Pensé pour l'observation en extérieur (test au soleil, sans
 *          moniteur série) : chaque appui sur BP2 réveille le boîtier, qui
 *          reprend une mesure fraîche, l'affiche 15 secondes, puis se
 *          rendort. Réutilise directement AfficheurOLED (primitives de
 *          dessin héritées d'Adafruit_SSD1306 + initialiser()/eteindre()
 *          déjà écrits pour le firmware principal) et MesureBatteries, afin
 *          de faciliter leur intégration ultérieure dans
 *          BoitierPilotageArrosage.
 *
 *          Compilé uniquement dans l'environnement PlatformIO
 *          [env:test_batterie_oled] (voir platformio.ini). Flasher avec :
 *          pio run -e test_batterie_oled -t upload
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include "AfficheurOLED.h"
#include "MesureBatteries.h"

// ── Broche BP2 (cf. schéma alimentation : entrée seule, résistance de
//    tirage externe sur la carte, pas de tirage interne disponible) ─────────
#define BROCHE_BP2 39

// ── Durée d'affichage avant réendormissement ─────────────────────────────────
#define DUREE_AFFICHAGE_MS 15000

AfficheurOLED ecran;
MesureBatteries mesureBatteries;

static void dessinerMesure(const MesureBatterie &_mesure, bool _capteurDisponible)
{
    ecran.clearDisplay();
    ecran.setTextSize(1);
    ecran.setTextColor(SSD1306_WHITE);

    ecran.setCursor(0, 0);
    ecran.print("Batterie (INA219)");
    ecran.drawLine(0, 10, LARGEUR_ECRAN - 1, 10, SSD1306_WHITE);

    if (_capteurDisponible)
    {
        ecran.setCursor(0, 16);
        ecran.print("U : ");
        ecran.print(_mesure.tensionV, 2);
        ecran.print(" V");

        ecran.setCursor(0, 28);
        ecran.print("I : ");
        ecran.print(_mesure.courantMa, 0);
        ecran.print(" mA");

        ecran.setCursor(0, 40);
        ecran.print("Charge : ");
        ecran.print(_mesure.pourcentage);
        ecran.print(" %");

        ecran.setCursor(0, 52);
        ecran.print(_mesure.enCharge ? "EN CHARGE" : "repos / decharge");
    }
    else
    {
        ecran.setCursor(0, 24);
        ecran.print("INA219 non detecte");
        ecran.setCursor(0, 36);
        ecran.print("Verifier le bus I2C");
    }

    ecran.display();
}

static void configurerReveilSurBP2()
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); // purge un éventuel timer résiduel
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BROCHE_BP2, LOW);
}

void setup()
{
    Wire.begin();

    bool capteurDisponible = mesureBatteries.initialiser();
    bool ecranDisponible = ecran.initialiser();

    if (ecranDisponible)
    {
        MesureBatterie mesure = mesureBatteries.lireMesure();
        dessinerMesure(mesure, capteurDisponible);
    }

    delay(DUREE_AFFICHAGE_MS);

    if (ecranDisponible)
        ecran.eteindre();

    configurerReveilSurBP2();
    esp_deep_sleep_start();
    // ne retourne jamais : l'ESP32 redémarre depuis setup() au réveil
}

void loop() {}
