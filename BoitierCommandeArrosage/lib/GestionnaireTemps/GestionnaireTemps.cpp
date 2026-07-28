/**
 * @file    GestionnaireTemps.cpp
 * @brief   Implémentation de l'interface avec le RTC DS3231.
 */

// lib/GestionnaireTemps/GestionnaireTemps.cpp

#include "GestionnaireTemps.h"

/**
 * @brief Construit le gestionnaire en mémorisant la broche IRQ ; le RTC est
 *        considéré indisponible tant que initialiser() n'a pas réussi.
 * @param _brocheIRQ Broche GPIO reliée à SQW/INT du DS3231.
 */
GestionnaireTemps::GestionnaireTemps(uint8_t _brocheIRQ)
    : brocheIRQ(_brocheIRQ)
    , disponible(false)
{
}

// ============================================================
// Initialisation
// ============================================================

/**
 * @brief Initialise la communication avec le DS3231 et prépare l'alarme.
 * @details Voir la documentation complète dans GestionnaireTemps.h. Points
 *          clés de l'implémentation : la broche IRQ est configurée en
 *          INPUT_PULLUP (le DS3231 tire la ligne à l'état bas en sortie
 *          open-drain lors du déclenchement) ; rtc.lostPower() indique si
 *          l'oscillateur s'est arrêté (perte d'alimentation principale ET
 *          de la pile de secours) et déclenche un réglage de repli sur la
 *          date de compilation ; rtc.writeSqwPinMode(DS3231_OFF) doit être
 *          appelé avant toute programmation d'alarme, sans quoi
 *          rtc.setAlarm1() peut échouer silencieusement.
 * @return true si rtc.begin() a réussi, false sinon.
 */
bool GestionnaireTemps::initialiser()
{
    pinMode(brocheIRQ, INPUT_PULLUP);

    if (rtc.begin())
    {
        disponible = true;
        DEBUG_VAL("lostPower : ", rtc.lostPower());

        if (rtc.lostPower())
        {
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }

        rtc.disable32K();
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
        rtc.writeSqwPinMode(DS3231_OFF);   // ← AJOUT : active le mode interruption sur SQW/INT
        rtc.disableAlarm(2);
    }
    else
    {
        disponible = false;
    }

    return disponible;
}

/**
 * @brief Indique si le RTC est disponible.
 * @return Valeur du drapeau interne "disponible".
 */
bool GestionnaireTemps::rtcDisponible() const
{
    return disponible;
}

// ============================================================
// Lecture / écriture heure système
// ============================================================

/**
 * @brief Lit l'heure courante du RTC et la formate en ISO8601.
 * @return Heure ISO8601, ou chaîne vide si le RTC est indisponible.
 */
String GestionnaireTemps::lireHeureSysteme() const
{
    String heureISO = "";

    if (disponible)
    {
        DateTime maintenant = rtc.now();
        heureISO = versISO8601(maintenant);
    }

    return heureISO;
}

/**
 * @brief Met à jour l'heure du RTC à partir d'une chaîne ISO8601.
 * @param _heureISO Nouvelle heure, au format ISO8601.
 */
void GestionnaireTemps::ecrireHeureSysteme(const String &_heureISO)
{
    if (disponible)
    {
        DateTime nouvelleDate = versDateTime(_heureISO);
        rtc.adjust(nouvelleDate);
    }
}

// ============================================================
// Calcul de planification
// ============================================================

/**
 * @brief Calcule la prochaine occurrence future d'une programmation
 *        périodique.
 * @details Avance l'heure de début par pas de _frequenceHeures heures
 *          (TimeSpan) tant qu'elle reste antérieure ou égale à l'heure
 *          courante du RTC, garantissant un résultat strictement futur.
 * @param _mode            Mode de la vanne ('P' requis).
 * @param _heureDebutISO   Heure de début de la programmation, ISO8601.
 * @param _frequenceHeures Période de répétition, en heures (doit être > 0).
 * @return Prochaine occurrence au format ISO8601, ou chaîne vide si les
 *         conditions (RTC disponible, mode 'P', fréquence > 0) ne sont pas
 *         réunies.
 */
String GestionnaireTemps::calculerProchaineAlarme(char _mode,
                                                  const String &_heureDebutISO,
                                                  int _frequenceHeures) const
{
    String prochaineISO = "";

    if (disponible)
    {
        if (_mode == 'P')
        {
            if (_frequenceHeures > 0)
            {
                DateTime maintenant   = rtc.now();
                DateTime occurrence   = versDateTime(_heureDebutISO);
                TimeSpan pasFrequence = TimeSpan(0, _frequenceHeures, 0, 0);

                while (occurrence <= maintenant)
                    occurrence = occurrence + pasFrequence;

                prochaineISO = versISO8601(occurrence);
            }
        }
    }

    return prochaineISO;
}

/**
 * @brief Ajoute une durée en minutes à une date ISO8601.
 * @param _heureISO Date/heure de départ, ISO8601 (vide → résultat vide).
 * @param _minutes  Minutes à ajouter (peut être négatif).
 * @return Date/heure résultat, au format ISO8601.
 */
String GestionnaireTemps::ajouterMinutes(const String &_heureISO, int _minutes) const
{
    String resultatISO = "";

    if (_heureISO.length() > 0)
    {
        DateTime depart    = versDateTime(_heureISO);
        TimeSpan duree     = TimeSpan(0, 0, _minutes, 0);
        DateTime resultat  = depart + duree;

        resultatISO = versISO8601(resultat);
    }

    return resultatISO;
}

// ============================================================
// Alarme / réveil
// ============================================================

/**
 * @brief Programme l'alarme 1 du DS3231 à la date/heure donnée.
 * @details Efface l'alarme 1 existante puis la reprogramme en mode
 *          DS3231_A1_Date (correspondance exacte date/heure/minute/seconde,
 *          adaptée à un déclenchement ponctuel unique par occurrence).
 *          Journalise une erreur sur le port série si l'écriture échoue.
 * @param _heureISO Date/heure de déclenchement, au format ISO8601.
 */
void GestionnaireTemps::programmerAlarme(const String &_heureISO)
{
    if (disponible)
    {
        if (_heureISO.length() > 0)
        {
            DateTime dateAlarme = versDateTime(_heureISO);

            rtc.clearAlarm(1);

            if (!rtc.setAlarm1(dateAlarme, DS3231_A1_Date))
            {
                Serial.println("ERREUR : setAlarm1() a echoue");
            }
        }
    }
}

/**
 * @brief Lit le drapeau logiciel de déclenchement de l'alarme 1
 *        (rtc.alarmFired()), indépendamment de l'état physique de la
 *        broche SQW/INT.
 * @return true si l'alarme 1 est signalée comme déclenchée.
 */
bool GestionnaireTemps::alarmeDeclenchee() const
{
    bool declenchee = false;

    if (disponible)
        declenchee = rtc.alarmFired(1);

    return declenchee;
}

/**
 * @brief Efface le drapeau de déclenchement de l'alarme 1
 *        (rtc.clearAlarm(1)).
 */
void GestionnaireTemps::effacerAlarme()
{
    if (disponible)
        rtc.clearAlarm(1);
}

// ============================================================
// Mise en veille
// ============================================================

/**
 * @brief Active le réveil sur alarme RTC (EXT0) et entre en deep sleep.
 * @details N'active que la source EXT0 (contrairement à
 *          BoitierPilotageArrosage::configurerSourcesReveil(), qui combine
 *          EXT0 et EXT1) ; esp_deep_sleep_start() ne retourne jamais.
 */
void GestionnaireTemps::endormirJusquaAlarme()
{
    esp_sleep_enable_ext0_wakeup((gpio_num_t)brocheIRQ, LOW);
    esp_deep_sleep_start();
    // ne retourne jamais : l'ESP32 redémarre depuis setup() au réveil
}

// ============================================================
// Privé : conversions
// ============================================================

/**
 * @brief Parse une chaîne ISO8601 en DateTime RTClib.
 * @details Découpe la chaîne par position fixe (format attendu strict
 *          "YYYY-MM-DDTHH:MM:SS") et convertit chaque champ via
 *          String::toInt().
 * @param _heureISO Chaîne à parser.
 * @return DateTime correspondant.
 */
DateTime GestionnaireTemps::versDateTime(const String &_heureISO) const
{
    // Format attendu : "YYYY-MM-DDTHH:MM:SS"
    int annee   = _heureISO.substring(0, 4).toInt();
    int mois    = _heureISO.substring(5, 7).toInt();
    int jour    = _heureISO.substring(8, 10).toInt();
    int heure   = _heureISO.substring(11, 13).toInt();
    int minute  = _heureISO.substring(14, 16).toInt();
    int seconde = _heureISO.substring(17, 19).toInt();

    DateTime resultat(annee, mois, jour, heure, minute, seconde);

    return resultat;
}

/**
 * @brief Formate un DateTime RTClib en chaîne ISO8601.
 * @param _dateHeure DateTime à formater.
 * @return Chaîne "YYYY-MM-DDTHH:MM:SS".
 */
String GestionnaireTemps::versISO8601(const DateTime &_dateHeure) const
{
    char tampon[20];

    sprintf(tampon, "%04d-%02d-%02dT%02d:%02d:%02d",
            _dateHeure.year(), _dateHeure.month(), _dateHeure.day(),
            _dateHeure.hour(), _dateHeure.minute(), _dateHeure.second());

    String resultat = String(tampon);

    return resultat;
}