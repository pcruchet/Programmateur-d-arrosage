/**
 * @file    ProtocoleArrosageServeur.h
 * @brief   Encodage/décodage des trames JSON du protocole applicatif
 *          "Contrôleur d'Arrosage" (WebSocket, ESP32 <-> application Qt/QML).
 *
 * @details Toutes les trames partagent la structure générale
 *          `{"v":<version>,"t":"<type>","c":"<commande>", ...}`. Cette
 *          classe ne réalise aucune action métier : elle se contente de
 *          décoder les requêtes entrantes (decoder()) et de construire les
 *          trames JSON de réponse, d'accusé de réception, d'erreur et de
 *          notification, à charge pour BoitierPilotageArrosage d'orchestrer
 *          leur utilisation.
 */

// lib/ProtocoleArrosageServeur/ProtocoleArrosageServeur.h

#ifndef PROTOCOLE_ARROSAGE_SERVEUR_H
#define PROTOCOLE_ARROSAGE_SERVEUR_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Debug.h"

// ── Structure résultat du décodage ────────────────────────────────────────────

/**
 * @struct RequeteArrosage
 * @brief  Représentation décodée d'une trame JSON reçue du client.
 */
struct RequeteArrosage
{
    String type;     ///< Type de trame reçue : "q" (query/demande) ou "c" (commande).
    String commande; ///< Caractère de commande ("S", "T", "O", "F", "m", "p", ...), voir les constantes CMD_*.
    int idVanne;     ///< Identifiant de vanne visé (1 à 4), ou -1 si non applicable à cette commande.
    String mode;     ///< Mode demandé ("M", "P", "A"), ou chaîne vide si non applicable.
    String heure;    ///< Heure associée à la requête, au format ISO8601, ou chaîne vide si non applicable.
    int duree;       ///< Durée associée (minutes), ou 0 si non applicable.
    int frequence;   ///< Fréquence associée (heures), ou 0 si non applicable.
    bool valide;     ///< false si le parsing JSON a échoué ou si la version de protocole est incompatible ; les autres champs ne doivent alors pas être exploités.
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class ProtocoleArrosageServeur
 * @brief Couche protocole : décodage des requêtes et construction des
 *        trames JSON de réponse côté firmware ESP32.
 */
class ProtocoleArrosageServeur
{
public:
    // ── Version ──────────────────────────────────────────────────────────────
    static const int VERSION = 1; ///< Version du protocole applicatif supportée ; toute trame reçue avec une autre version est rejetée par decoder().

    // ── Types de trames ───────────────────────────────────────────────────────
    static const char TYPE_QUERY = 'q';    ///< Trame de demande (question) émise par le client.
    static const char TYPE_REPONSE = 'r';  ///< Trame de réponse à une demande.
    static const char TYPE_COMMANDE = 'c'; ///< Trame de commande (action à exécuter) émise par le client.
    static const char TYPE_ACK = 'a';      ///< Trame d'accusé de réception d'une commande exécutée avec succès.
    static const char TYPE_ERREUR = 'e';   ///< Trame d'erreur.
    static const char TYPE_NOTIF = 'n';    ///< Trame de notification spontanée émise par le serveur (non sollicitée par une requête).

    // ── Commandes ─────────────────────────────────────────────────────────────
    static const char CMD_GET_TIME = 'T';   ///< Lecture de l'heure système.
    static const char CMD_SET_TIME = 't';   ///< Écriture de l'heure système.
    static const char CMD_GET_ETAT = 'E';   ///< Lecture de l'état (ouverte/fermée) d'une vanne.
    static const char CMD_GET_MODE = 'M';   ///< Lecture du mode d'une vanne (également utilisé comme commande des notifications de changement de mode).
    static const char CMD_SET_MODE = 'm';   ///< Écriture du mode d'une vanne.
    static const char CMD_GET_PROG = 'P';   ///< Lecture de la programmation d'une vanne.
    static const char CMD_SET_PROG = 'p';   ///< Écriture de la programmation d'une vanne.
    static const char CMD_OUVRIR = 'O';     ///< Commande d'ouverture manuelle d'une vanne.
    static const char CMD_FERMER = 'F';     ///< Commande de fermeture manuelle d'une vanne.
    static const char CMD_PING = 'G';       ///< Commande de test de présence (ping/pong).
    static const char CMD_GET_SYSTEM = 'S'; ///< Lecture de l'état système complet (heure + état de toutes les vannes).
    static const char CMD_VEILLE = 'V';     ///< Commande/notification associée à la mise en veille du boîtier.

    // ── Modes ─────────────────────────────────────────────────────────────────
    static const char MODE_MANUEL = 'M';      ///< Mode Manuel : la vanne n'obéit qu'aux commandes explicites OUVRIR/FERMER, la programmation est suspendue.
    static const char MODE_PROGRAMME = 'P';   ///< Mode Programme : la vanne s'ouvre/se ferme automatiquement selon l'heure, la durée et la fréquence programmées.
    static const char MODE_AUTOMATIQUE = 'A'; ///< Mode Automatique (réservé pour une logique d'arrosage automatisée au-delà de la simple programmation horaire).

    // ── États ─────────────────────────────────────────────────────────────────
    static const char ETAT_FERMEE = 'F';  ///< Code d'état "vanne fermée".
    static const char ETAT_OUVERTE = 'O'; ///< Code d'état "vanne ouverte".

    // ── Codes erreur ──────────────────────────────────────────────────────────
    static const int ERREUR_AUCUNE = 0;                ///< Aucune erreur spécifique (utilisé par défaut, ex. commande inconnue).
    static const int ERREUR_VANNE_INEXISTANTE = 1;     ///< L'identifiant de vanne demandé ne correspond à aucune des 4 vannes.
    static const int ERREUR_MODE_INVALIDE = 2;         ///< Le mode demandé ne correspond à aucun des modes valides (M/P/A).
    static const int ERREUR_PROG_INVALIDE = 3;         ///< La programmation demandée est invalide (heure, durée ou fréquence manquante/nulle).
    static const int ERREUR_RTC_INDISPONIBLE = 4;      ///< Le RTC n'est pas disponible pour traiter la requête.
    static const int ERREUR_SQLITE_INDISPONIBLE = 5;   ///< Réservé pour signaler l'indisponibilité d'un stockage persistant (non utilisé actuellement côté firmware NVS).

    // ── Constructeur ──────────────────────────────────────────────────────────

    /// Construit l'encodeur/décodeur (ne détient aucun état).
    ProtocoleArrosageServeur();

    // ── Décodage ──────────────────────────────────────────────────────────────

    /**
     * @brief Décode une trame JSON reçue en structure RequeteArrosage.
     * @details Vérifie la présence des champs obligatoires ("v", "t", "c")
     *          et la compatibilité de version avant de considérer la
     *          requête comme valide. Les champs optionnels (idVanne, mode,
     *          heure, duree, frequence) ne sont renseignés que s'ils sont
     *          présents dans la trame reçue.
     * @param _message Trame JSON brute reçue du client.
     * @return Structure décodée ; RequeteArrosage::valide vaut false si le
     *         parsing JSON a échoué, si un champ obligatoire est absent, ou
     *         si la version ne correspond pas à VERSION.
     */
    RequeteArrosage decoder(const String &_message) const;

    // ── Construction des réponses ─────────────────────────────────────────────

    /**
     * @brief Construit la trame de réponse à une demande d'heure système.
     * @details Exemple : {"v":1,"t":"r","c":"T","h":"2026-07-08T18:30:00"}
     * @param _heure Heure système au format ISO8601.
     * @return Trame JSON sérialisée.
     */
    String creerReponseHeure(const String &_heure) const;

    /**
     * @brief Construit la trame de réponse à une demande d'état de vanne.
     * @details Exemple : {"v":1,"t":"r","c":"E","i":1,"e":"F"}
     * @param _idVanne Identifiant de la vanne concernée.
     * @param _etat    Code d'état (ETAT_OUVERTE ou ETAT_FERMEE).
     * @return Trame JSON sérialisée.
     */
    String creerReponseEtat(int _idVanne, char _etat) const;

    /**
     * @brief Construit la trame de réponse à une demande de mode de vanne.
     * @details Exemple : {"v":1,"t":"r","c":"M","i":1,"m":"P"}
     * @param _idVanne Identifiant de la vanne concernée.
     * @param _mode    Code de mode (MODE_MANUEL, MODE_PROGRAMME ou MODE_AUTOMATIQUE).
     * @return Trame JSON sérialisée.
     */
    String creerReponseMode(int _idVanne, char _mode) const;

    /**
     * @brief Construit la trame de réponse à une demande de programmation
     *        de vanne.
     * @details Exemple : {"v":1,"t":"r","c":"P","i":1,"h":"...","d":15,"f":24}
     * @param _idVanne   Identifiant de la vanne concernée.
     * @param _heure     Heure de début programmée, ISO8601.
     * @param _duree     Durée d'arrosage programmée, en minutes.
     * @param _frequence Fréquence de répétition, en heures.
     * @return Trame JSON sérialisée.
     */
    String creerReponseProgrammation(int _idVanne,
                                     const String &_heure,
                                     int _duree,
                                     int _frequence) const;

    /**
     * @brief Construit la trame de réponse à un ping.
     * @details Exemple : {"v":1,"t":"r","c":"G"}
     * @return Trame JSON sérialisée.
     */
    String creerReponsePing() const;

    /**
     * @brief Construit la trame de réponse à une demande d'état système
     *        complet.
     * @details Exemple : {"v":1,"t":"r","c":"S","h":"...","s":[...]}
     * @param _heureSysteme Heure système courante, ISO8601.
     * @param _vannes       Tableau JSON décrivant l'état de chaque vanne,
     *                      déjà construit par l'appelant
     *                      (BoitierPilotageArrosage).
     * @return Trame JSON sérialisée.
     */
    String creerReponseSysteme(const String &_heureSysteme,
                               const JsonArray &_vannes) const;

    // ── Construction des acks ─────────────────────────────────────────────────

    /**
     * @brief Construit un accusé de réception générique (sans vanne
     *        associée).
     * @details Exemple : {"v":1,"t":"a","c":"t"}
     * @param _commande Caractère de la commande acquittée.
     * @return Trame JSON sérialisée.
     */
    String creerAck(char _commande) const;

    /**
     * @brief Construit un accusé de réception associé à une vanne.
     * @details Exemple : {"v":1,"t":"a","c":"m","i":1}
     * @param _commande Caractère de la commande acquittée.
     * @param _idVanne  Identifiant de la vanne concernée.
     * @return Trame JSON sérialisée.
     */
    String creerAckVanne(char _commande, int _idVanne) const;

    // ── Construction des erreurs ──────────────────────────────────────────────

    /**
     * @brief Construit une trame d'erreur.
     * @details Exemple : {"v":1,"t":"e","c":"<commande>","x":<code>}
     * @param _commande   Caractère de la commande à l'origine de l'erreur.
     * @param _codeErreur Code d'erreur (voir les constantes ERREUR_*).
     * @return Trame JSON sérialisée.
     */
    String creerErreur(char _commande, int _codeErreur) const;

    // ── Construction des notifications ────────────────────────────────────────

    /**
     * @brief Construit une notification de changement d'état d'une vanne
     *        (ouverture ou fermeture spontanée, ex. déclenchée par la
     *        programmation).
     * @details Exemple : {"v":1,"t":"n","c":"O","i":1} ou
     *          {"v":1,"t":"n","c":"F","i":1}
     * @param _commande Commande décrivant l'événement (CMD_OUVRIR ou
     *                  CMD_FERMER).
     * @param _idVanne  Identifiant de la vanne concernée.
     * @return Trame JSON sérialisée.
     */
    String creerNotifEtatVanne(char _commande, int _idVanne) const;

    /**
     * @brief Construit une notification de changement de mode d'une vanne.
     * @details Exemple : {"v":1,"t":"n","c":"M","i":1,"m":"A"}
     * @param _idVanne Identifiant de la vanne concernée.
     * @param _mode    Nouveau mode (MODE_MANUEL, MODE_PROGRAMME ou MODE_AUTOMATIQUE).
     * @return Trame JSON sérialisée.
     */
    String creerNotifMode(int _idVanne, char _mode) const;

    /**
     * @brief Construit une notification de changement de programmation
     *        d'une vanne.
     * @details Exemple : {"v":1,"t":"n","c":"P","i":1,"h":"...","d":15,"f":24}
     * @param _idVanne   Identifiant de la vanne concernée.
     * @param _heure     Nouvelle heure de début programmée, ISO8601.
     * @param _duree     Nouvelle durée programmée, en minutes.
     * @param _frequence Nouvelle fréquence programmée, en heures.
     * @return Trame JSON sérialisée.
     */
    String creerNotifProgrammation(int _idVanne,
                                   const String &_heure,
                                   int _duree,
                                   int _frequence) const;

    /**
     * @brief Construit la notification de mise en veille envoyée au client
     *        avant l'entrée en deep sleep pour cause d'inactivité.
     * @details Exemple : {"v":1,"t":"n","c":"V"}
     * @return Trame JSON sérialisée.
     */
    String creerNotifVeille() const;

private:
    // ── Utilitaire interne : trame de base ────────────────────────────────────

    /**
     * @brief Construit une trame minimale ne contenant que la version, le
     *        type et la commande (aucun autre champ).
     * @param _type     Type de trame (voir les constantes TYPE_*).
     * @param _commande Caractère de commande associé.
     * @return Trame JSON sérialisée.
     */
    String creerTrame(char _type, char _commande) const;
};

#endif // PROTOCOLE_ARROSAGE_SERVEUR_H
