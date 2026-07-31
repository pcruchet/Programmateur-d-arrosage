/**
 * @file    GestionnaireTemps.h
 * @brief   Interface avec le RTC DS3231 : heure système, calcul de
 *          planification et gestion de l'alarme de réveil.
 *
 * @details Encapsule la bibliothèque RTClib (RTC_DS3231) pour offrir au
 *          reste du firmware une API en chaînes de caractères ISO8601
 *          ("YYYY-MM-DDTHH:MM:SS"), un calcul de la prochaine occurrence
 *          d'une programmation périodique, et la gestion de l'alarme 1 du
 *          DS3231 utilisée comme source de réveil du deep sleep (broche
 *          SQW/INT en mode interruption).
 */

#ifndef GESTIONNAIRE_TEMPS_H
#define GESTIONNAIRE_TEMPS_H

#include <Arduino.h>
#include <RTClib.h>
#include "Debug.h"

/**
 * @class GestionnaireTemps
 * @brief Encapsule le RTC DS3231 (horloge temps réel + alarme matérielle).
 */
class GestionnaireTemps
{
public:
    /**
     * @brief Construit le gestionnaire sans initialiser le matériel.
     * @param _brocheIRQ Broche GPIO reliée à la sortie SQW/INT du DS3231
     *                    (doit être une broche RTC GPIO pour permettre le
     *                    réveil du deep sleep).
     */
    GestionnaireTemps(uint8_t _brocheIRQ);

    // ── Initialisation ────────────────────────────────────────

    /**
     * @brief Initialise la communication I2C avec le DS3231 et prépare
     *        l'alarme.
     * @details Configure la broche IRQ en entrée avec pull-up, tente
     *          d'ouvrir la communication avec le RTC (rtc.begin()). Si la
     *          perte d'alimentation est détectée (rtc.lostPower(), pile de
     *          secours absente/déchargée ou premier démarrage), réinitialise
     *          l'horloge à la date/heure de compilation du firmware (valeur
     *          de secours en attendant une resynchronisation applicative).
     *          Désactive la sortie 32 kHz, efface les deux alarmes, bascule
     *          la broche SQW/INT en mode interruption (DS3231_OFF,
     *          nécessaire pour que setAlarm1() fonctionne) et désactive
     *          l'alarme 2 (non utilisée).
     * @return true si la communication avec le DS3231 a réussi, false
     *         sinon.
     */
    bool initialiser();

    /**
     * @brief Indique si le RTC est disponible (initialisé avec succès).
     * @return true si les appels au RTC sont opérationnels.
     */
    bool rtcDisponible() const;

    // ── Lecture / écriture heure système ────────────────────────

    /**
     * @brief Lit l'heure courante du RTC.
     * @return Heure au format ISO8601 ("YYYY-MM-DDTHH:MM:SS"), ou chaîne
     *         vide si le RTC n'est pas disponible.
     */
    String lireHeureSysteme() const;                    // ISO8601

    /**
     * @brief Met à jour l'heure du RTC.
     * @param _heureISO Nouvelle heure au format ISO8601
     *                   ("YYYY-MM-DDTHH:MM:SS"). Sans effet si le RTC n'est
     *                   pas disponible.
     */
    void   ecrireHeureSysteme(const String &_heureISO);

    // ── Calcul de planification ──────────────────────────────────

    /**
     * @brief Calcule la prochaine occurrence d'ouverture d'une
     *        programmation périodique, strictement après l'heure courante.
     * @details Le mode doit être 'P' (Programme) et la fréquence strictement
     *          positive, sinon retourne une chaîne vide. Part de l'heure de
     *          début fournie et avance par pas de _frequenceHeures heures
     *          jusqu'à dépasser strictement l'heure courante du RTC (une
     *          échéance déjà passée est donc automatiquement reportée à
     *          l'occurrence future la plus proche).
     * @param _mode             Mode de la vanne ('P' pour Programme, tout
     *                          autre caractère produit un résultat vide).
     * @param _heureDebutISO    Heure d'ouverture programmée, au format
     *                          ISO8601.
     * @param _frequenceHeures  Fréquence de répétition, en heures (doit être
     *                          > 0).
     * @return Prochaine heure d'ouverture au format ISO8601, ou chaîne vide
     *         si le RTC est indisponible, le mode n'est pas 'P', ou la
     *         fréquence est nulle/négative.
     */
    String calculerProchaineAlarme(char _mode,
                                   const String &_heureDebutISO,
                                   int _frequenceHeures) const;

    /**
     * @brief Calcule l'occurrence courante (ou la plus récemment passée)
     *        d'une programmation périodique, servant à déterminer si l'heure
     *        actuelle se trouve dans une fenêtre d'arrosage.
     * @details Le mode doit être 'P' (Programme) et la fréquence strictement
     *          positive, sinon retourne une chaîne vide. Part de l'heure de
     *          début fournie et avance par pas de _frequenceHeures heures
     *          tant que l'occurrence suivante reste antérieure ou égale à
     *          l'heure courante du RTC, afin d'obtenir la dernière échéance
     *          survenue à ce jour (contrairement à calculerProchaineAlarme(),
     *          qui retourne la prochaine échéance future). Si l'heure de
     *          début elle-même est encore future, retourne une chaîne vide
     *          (aucune occurrence n'a encore eu lieu).
     * @param _mode             Mode de la vanne ('P' pour Programme, tout
     *                          autre caractère produit un résultat vide).
     * @param _heureDebutISO    Heure d'ouverture programmée, au format
     *                          ISO8601.
     * @param _frequenceHeures  Fréquence de répétition, en heures (doit être
     *                          > 0).
     * @return Occurrence courante au format ISO8601, ou chaîne vide si le
     *         RTC est indisponible, le mode n'est pas 'P', la fréquence est
     *         nulle/négative, ou aucune occurrence n'a encore eu lieu.
     */
    String calculerOccurrenceCourante(char _mode,
                                      const String &_heureDebutISO,
                                      int _frequenceHeures) const;

    /**
     * @brief Ajoute une durée en minutes à une date ISO8601.
     * @details Utilisée notamment pour déduire l'heure de fermeture d'une
     *          vanne à partir de son heure d'ouverture et de sa durée
     *          d'arrosage programmée.
     * @param _heureISO Date/heure de départ, au format ISO8601. Si vide,
     *                  retourne une chaîne vide.
     * @param _minutes  Nombre de minutes à ajouter (peut être négatif).
     * @return Date/heure résultat au format ISO8601, ou chaîne vide si
     *         _heureISO est vide.
     */
    String ajouterMinutes(const String &_heureISO, int _minutes) const;

    // ── Alarme / réveil ───────────────────────────────────────

    /**
     * @brief Programme l'alarme 1 du DS3231 à une date/heure donnée.
     * @details Efface d'abord l'alarme 1, puis la programme en mode
     *          DS3231_A1_Date (correspondance sur date/heure/minute/seconde
     *          exacte). Trace une erreur sur le port série si l'écriture de
     *          l'alarme échoue (setAlarm1() retourne false). Sans effet si
     *          le RTC est indisponible ou si _heureISO est vide.
     * @param _heureISO Date/heure de déclenchement de l'alarme, au format
     *                  ISO8601.
     */
    void programmerAlarme(const String &_heureISO);

    /**
     * @brief Indique si l'alarme 1 s'est déclenchée (lecture logicielle du
     *        flag du DS3231, indépendante de l'état de la broche SQW/INT).
     * @return true si l'alarme 1 est signalée comme déclenchée.
     */
    bool alarmeDeclenchee() const;

    /**
     * @brief Efface le flag de déclenchement de l'alarme 1.
     * @details À appeler après un réveil par alarme RTC, pour permettre à
     *          la prochaine alarme programmée de se déclencher normalement.
     */
    void effacerAlarme();

    // ── Mise en veille ───────────────────────────────────────

    /**
     * @brief Configure le réveil sur alarme RTC et entre immédiatement en
     *        deep sleep.
     * @details Active la source de réveil EXT0 sur la broche IRQ (niveau
     *          bas) puis appelle esp_deep_sleep_start(), qui ne retourne
     *          jamais : l'exécution reprendra depuis setup() après
     *          redémarrage. Méthode utilitaire autonome, distincte du
     *          pilotage des sources de réveil réalisé par
     *          BoitierPilotageArrosage::configurerSourcesReveil() (qui
     *          combine EXT0 et EXT1 avant l'appel à esp_deep_sleep_start()
     *          dans l'automate principal).
     */
    void endormirJusquaAlarme();

private:
    mutable RTC_DS3231 rtc;   ///< Instance RTClib de communication avec le DS3231 (mutable pour permettre des lectures dans des méthodes const).
    uint8_t    brocheIRQ;      ///< Broche GPIO reliée à la sortie SQW/INT du DS3231.
    bool       disponible;     ///< Indique si le RTC a été initialisé avec succès (résultat de initialiser()).

    // ── Conversions internes ──────────────────────────────────

    /**
     * @brief Convertit une chaîne ISO8601 en DateTime RTClib.
     * @param _heureISO Chaîne au format "YYYY-MM-DDTHH:MM:SS".
     * @return DateTime correspondant (aucune validation stricte du format :
     *         les champs non numériques sont interprétés comme 0 par
     *         String::toInt()).
     */
    DateTime  versDateTime(const String &_heureISO) const;

    /**
     * @brief Convertit un DateTime RTClib en chaîne ISO8601.
     * @param _dateHeure DateTime à formater.
     * @return Chaîne au format "YYYY-MM-DDTHH:MM:SS".
     */
    String    versISO8601(const DateTime &_dateHeure) const;
};

#endif // GESTIONNAIRE_TEMPS_H
