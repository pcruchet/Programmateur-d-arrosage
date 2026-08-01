/**
 * @file    BoitierPilotageArrosage.h
 * @brief   Automate d'états principal du boîtier de commande d'arrosage.
 *
 * @details Déclare la classe BoitierPilotageArrosage, qui orchestre l'ensemble
 *          du firmware ESP32 : gestion des cycles veille/réveil (deep sleep /
 *          light sleep), pilotage des 4 électrovannes, communication WebSocket
 *          avec l'application Qt/QML, persistance NVS de la programmation et
 *          de l'état des vannes, et synchronisation avec le RTC DS3231.
 *
 *          Ce fichier centralise également :
 *          - le brochage matériel (broches de réveil, sélection des vannes,
 *            pont en H),
 *          - les identifiants réseau WiFi,
 *          - les constantes temporelles de l'automate (timeouts, durées de
 *            veille),
 *          - les énumérations d'état de l'automate (::EtatBoitier) et de
 *            cause de réveil (::CauseReveil).
 */

#ifndef BOITIER_PILOTAGE_ARROSAGE_H
#define BOITIER_PILOTAGE_ARROSAGE_H

/// Active les traces de debug sur le port série (macros DEBUG/DEBUG_VAL
/// définies dans Debug.h). À commenter pour désactiver les traces en
/// production (réduit la taille du binaire et le temps de boot).
#define DEBUG_SERIAL // ← commenter pour désactiver en production
#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include "AfficheurOLED.h"
#include "ServeurWebSocket.h"
#include "ProtocoleArrosageServeur.h"
#include "Vanne.h"
#include "StockageProgrammationVannes.h"
#include "StockageEtatVannes.h"
#include "GestionnaireTemps.h"
#include "MesureBatteries.h"
#include "BoutonPoussoir.h"

#include "Debug.h"

// ── Broches réveil ────────────────────────────────────────────────────────────

/// Broche GPIO reliée à la sortie SQW/INT du DS3231, utilisée comme source de
/// réveil EXT0 pour sortir l'ESP32 du deep sleep sur alarme RTC. Doit être une
/// broche RTC GPIO (0, 2, 4, 12-15, 25-27, 32-39) : GPIO27 est utilisée ici
/// (GPIO19 initialement câblée ne convient pas, hors domaine RTC).
#define BROCHE_IRQ_RTC 27

/// Broche GPIO reliée au bouton poussoir BP1, utilisée comme source de réveil
/// EXT1 (deep sleep) et comme sortie de veille anticipée pendant le light
/// sleep. Actif à l'état bas.
#define BROCHE_BP1 36

/// Broche GPIO reliée au bouton poussoir BP2, utilisée pendant les phases
/// éveillées (ATTENTE_CONNEXION, DIALOGUE) pour déclencher un affichage
/// ponctuel de l'état de la batterie sur l'OLED. Contrairement à BP1, ne
/// sert pas de source de réveil deep sleep. Entrée seule (GPIO34-39, sans
/// tirage interne disponible) : résistance de tirage externe sur la carte.
#define BROCHE_BP2 39

// ── Broches vannes ────────────────────────────────────────────────────────────

/// Broche de sélection de l'électrovanne 1 sur le multiplexeur du pont en H.
#define PIN_SEL_V1 5
/// Broche de sélection de l'électrovanne 2 sur le multiplexeur du pont en H.
#define PIN_SEL_V2 17
/// Broche de sélection de l'électrovanne 3 sur le multiplexeur du pont en H.
#define PIN_SEL_V3 16
/// Broche de sélection de l'électrovanne 4 sur le multiplexeur du pont en H.
#define PIN_SEL_V4 4
/// Broche IN A du pont en H (L298), partagée par les 4 vannes ; pilote le
/// sens d'impulsion "ouverture".
#define PIN_INA 2
/// Broche IN B du pont en H (L298), partagée par les 4 vannes ; pilote le
/// sens d'impulsion "fermeture".
#define PIN_INB 15

// ── Credentials WiFi ──────────────────────────────────────────────────────────

/// SSID_STA / PASSWORD_STA définis dans secrets.h (non versionné, voir
/// include/secrets.h.example).
#include "secrets.h"

// ── Délais d'inactivité ───────────────────────────────────────────────────────

/// Délai maximal (ms) passé en ATTENTE_CONNEXION sans qu'aucun client
/// WebSocket ne se connecte, avant de repartir en deep sleep.
#define DELAI_ATTENTE_CONNEXION_MS      60000    // sans client → deep sleep

/// Délai maximal (ms) d'inactivité d'un client WebSocket resté connecté
/// (aucun message reçu) avant l'envoi d'une notification de veille et le
/// passage en deep sleep — protection contre les connexions fantômes.
#define DELAI_DECONNEXION_MS            300000    // connexion fantôme → deep sleep (5 min)

/// Durée (ms) d'un cycle de light sleep pendant l'arrosage, entre deux
/// vérifications de l'état des vannes (fermeture programmée à échéance).
#define DUREE_LIGHT_SLEEP_MS            30000    // light sleep entre updates vannes

/// Période (ms) de rafraîchissement de l'heure affichée sur l'OLED pendant
/// l'état DIALOGUE.
#define DELAI_RAFRAICHISSEMENT_OLED_MS  10000

/// Durée (ms) d'affichage de l'écran batterie déclenché par BP2, avant
/// retour automatique à l'écran habituel (statut réseau/vannes/heure).
#define DUREE_AFFICHAGE_BATTERIE_MS      15000

/// Période minimale (ms) entre deux vérifications de la programmation des
/// vannes pendant l'état DIALOGUE (throttle de verifierProgrammations()).
#define DELAI_VERIFICATION_PROGRAMMATION_MS  1000    // vérification programmation en DIALOGUE

// ── Durée du light sleep ──────────────────────────────────────────────────────
// (redéfinition identique à DUREE_LIGHT_SLEEP_MS ci-dessus, conservée telle
// quelle dans le code source)
#define DUREE_LIGHT_SLEEP_MS 30000

/**
 * @enum  EtatBoitier
 * @brief États de l'automate principal piloté par
 *        BoitierPilotageArrosage::controler().
 */
enum class EtatBoitier
{
    DEMARRAGE,             ///< Initialisation de l'affichage et des composants au boot, avant détermination de la cause de réveil.
    DETERMINER_REVEIL,     ///< Identifie la cause du réveil (premier démarrage, alarme RTC, bouton poussoir, timer light sleep).
    RESTAURER_ETAT_VANNES, ///< Relit l'état des vannes en NVS et referme par sécurité celles dont le délai est dépassé ou en cas de coupure secteur.
    ARROSAGE,               ///< Vérifie et applique la programmation horaire des vannes (ouverture/fermeture programmées).
    ATTENTE_CONNEXION,     ///< Démarre le WiFi et attend la connexion d'un client WebSocket (avec timeout).
    DIALOGUE,              ///< Traite les requêtes du client connecté et vérifie périodiquement la programmation.
    LIGHT_SLEEP,           ///< Veille légère (RAM conservée) pendant qu'une vanne reste ouverte, entre deux vérifications.
    ENDORMISSEMENT          ///< Calcule la prochaine alarme RTC, configure les sources de réveil et entre en deep sleep.
};

/**
 * @enum  CauseReveil
 * @brief Cause identifiée du réveil de l'ESP32, déterminée à partir de
 *        esp_sleep_get_wakeup_cause().
 */
enum class CauseReveil
{
    PREMIER_DEMARRAGE, ///< Mise sous tension initiale ou reset (cause de réveil non déterminée par le contrôleur de sommeil) ; traité comme une coupure secteur potentielle.
    ALARME_RTC,        ///< Réveil déclenché par l'interruption du DS3231 (source EXT0) : une programmation d'arrosage arrive à échéance.
    BOUTON_POUSSOIR,   ///< Réveil déclenché par un appui sur BP1 (source EXT1).
    LIGHT_SLEEP_TIMER, ///< Réveil déclenché par le timer périodique du light sleep (source TIMER).
    INCONNUE            ///< Cause de réveil non reconnue parmi les cas précédents.
};

/**
 * @class BoitierPilotageArrosage
 * @brief Automate d'états central du firmware : orchestre l'ensemble du
 *        cycle de vie du boîtier d'arrosage.
 *
 * @details Cette classe agrège tous les composants matériels et logiciels du
 *          boîtier (afficheur OLED, serveur WebSocket, protocole applicatif,
 *          gestion du temps RTC, stockage NVS de la programmation et de
 *          l'état des vannes, pilotage des 4 électrovannes) et implémente un
 *          automate d'états fini (::EtatBoitier) exécuté à chaque appel de
 *          controler() depuis la boucle principale Arduino (loop()).
 *
 *          Le cycle de vie typique alterne des phases de veille profonde
 *          (deep sleep, réveillée par le RTC ou un bouton poussoir) et des
 *          phases actives de dialogue avec l'application mobile ou de
 *          surveillance de la programmation d'arrosage.
 */
class BoitierPilotageArrosage
{
public:
    /**
     * @brief Construit l'automate. N'initialise pas le matériel : voir
     *        initialiser().
     */
    BoitierPilotageArrosage();

    /**
     * @brief Initialise l'ensemble des composants matériels et logiciels
     *        (afficheur, serveur WebSocket, protocole, RTC, stockages NVS,
     *        vannes) et positionne l'automate dans son état de départ.
     * @return true si tous les composants ont été initialisés avec succès,
     *         false sinon (le boîtier ne doit alors pas être piloté).
     */
    bool initialiser();

    /**
     * @brief Exécute un pas de l'automate d'états : appelle le traitement
     *        correspondant à l'état courant. À appeler en boucle depuis
     *        loop().
     */
    void controler();

private:
    // ── Composants ────────────────────────────────────────────────────────────
    AfficheurOLED *afficheur;                       ///< Pilote de l'écran OLED (état des vannes, heure, statut de connexion).
    ServeurWebSocket *serveurWebSocket;              ///< Serveur WebSocket exposant l'API de pilotage à l'application Qt/QML.
    ProtocoleArrosageServeur *protocole;             ///< Encodeur/décodeur des trames JSON du protocole applicatif.
    GestionnaireTemps *gestionnaireTemps;            ///< Interface avec le RTC DS3231 (lecture/écriture de l'heure, gestion de l'alarme).
    StockageProgrammationVannes *stockage;           ///< Persistance NVS de la programmation (mode, heure, durée, fréquence) de chaque vanne.
    StockageEtatVannes *stockageEtat;                ///< Persistance NVS de l'état physique (ouverte/fermée, heure d'ouverture) de chaque vanne.
    Vanne *vannes[4];                                 ///< Pilotage physique des 4 électrovannes bistables via le pont en H.
    MesureBatteries *mesureBatteries;                 ///< Lecture du capteur INA219 (tension/courant/puissance/pourcentage du pack batterie).
    BoutonPoussoir *bp2;                              ///< Lecture anti-rebond de BP2, déclenche l'affichage ponctuel de l'état batterie.

    // ── État courant ──────────────────────────────────────────────────────────
    EtatBoitier etatCourant;   ///< État courant de l'automate, déterminant le traitement exécuté par controler().
    CauseReveil causeReveil;   ///< Cause du dernier réveil, déterminée une fois au démarrage et utilisée pour orienter la restauration d'état.

    // ── Suivi d'activité ──────────────────────────────────────────────────────
    volatile uint32_t dernierMessageRecuMs;    ///< Horodatage (millis()) du dernier message WebSocket reçu, mis à jour depuis le contexte AsyncTCP (onMessageRecu()) et lu depuis loop() : marqué volatile pour éviter toute optimisation incohérente entre les deux tâches.
    uint32_t dernierRafraichissementOledMs;    ///< Horodatage du dernier rafraîchissement périodique de l'heure affichée sur l'OLED.
    uint32_t dernierVerificationProgMs;        ///< Horodatage de la dernière vérification de la programmation des vannes en état DIALOGUE (throttle).

    bool wifiDemarre;   ///< Indique si la connexion WiFi a déjà été initiée pour le cycle d'attente courant, afin d'éviter de la relancer à chaque appel de controler().

    bool affichageBatterieActif;       ///< true tant que l'écran batterie (déclenché par BP2) est affiché, le temps de DUREE_AFFICHAGE_BATTERIE_MS.
    uint32_t finAffichageBatterieMs;   ///< Horodatage (millis()) auquel l'écran batterie doit être remplacé par l'affichage habituel.

    // ── Traitements de chaque état ────────────────────────────────────────────

    /// Traitement de l'état DEMARRAGE : initialise l'affichage et bascule
    /// vers DETERMINER_REVEIL.
    void traiterDemarrage();

    /// Traitement de l'état DETERMINER_REVEIL : identifie la cause du réveil
    /// via determinerCauseReveil() et oriente vers RESTAURER_ETAT_VANNES ou
    /// directement ARROSAGE (cas du réveil par timer de light sleep).
    void traiterDeterminerReveil();

    /// Traitement de l'état RESTAURER_ETAT_VANNES : relit l'état de chaque
    /// vanne en NVS, referme par sécurité celles dont le délai d'arrosage est
    /// dépassé ou en cas de coupure secteur détectée, puis oriente
    /// l'automate vers ARROSAGE, ATTENTE_CONNEXION ou ENDORMISSEMENT selon la
    /// cause de réveil et l'état résultant des vannes.
    void traiterRestaurerEtatVannes();

    /// Traitement de l'état ARROSAGE : met à jour les impulsions de vannes en
    /// cours, vérifie la programmation de chaque vanne via
    /// verifierProgrammations(), puis bascule vers LIGHT_SLEEP (vanne
    /// ouverte) ou ENDORMISSEMENT (toutes fermées).
    void traiterArrosage();

    /// Traitement de l'état ATTENTE_CONNEXION : démarre le WiFi si besoin,
    /// et bascule vers DIALOGUE dès qu'un client WebSocket se connecte, ou
    /// vers ENDORMISSEMENT après expiration de DELAI_ATTENTE_CONNEXION_MS.
    void traiterAttenteConnexion();

    /// Traitement de l'état DIALOGUE : met à jour les impulsions de vannes en
    /// cours, rafraîchit périodiquement l'OLED, vérifie périodiquement la
    /// programmation des vannes (verifierProgrammations()), gère le timeout
    /// d'inactivité (DELAI_DECONNEXION_MS) et la déconnexion du client.
    void traiterDialogue();

    /// Traitement de l'état LIGHT_SLEEP : configure les sources de réveil du
    /// light sleep (timer + BP1), entre en veille légère, puis détermine à
    /// la sortie s'il faut revenir en ARROSAGE ou passer en
    /// ATTENTE_CONNEXION (appui BP1 pendant l'arrosage).
    void traiterLightSleep();

    /// Traitement de l'état ENDORMISSEMENT : calcule la prochaine alarme RTC
    /// à partir des programmations de chaque vanne, la programme dans le
    /// DS3231, configure les sources de réveil du deep sleep, puis entre en
    /// deep sleep (esp_deep_sleep_start(), ne retourne jamais).
    void traiterEndormissement();

    // ── Réveil ────────────────────────────────────────────────────────────────

    /**
     * @brief Détermine la cause du réveil de l'ESP32 à partir de
     *        esp_sleep_get_wakeup_cause().
     * @return La cause de réveil identifiée (::CauseReveil).
     */
    CauseReveil determinerCauseReveil() const;

    /// Configure les sources de réveil matérielles (EXT0 sur l'alarme RTC,
    /// EXT1 sur le bouton poussoir BP1) avant l'entrée en deep sleep.
    void configurerSourcesReveil();

    // ── Protocole ─────────────────────────────────────────────────────────────

    /**
     * @brief Callback invoqué par le serveur WebSocket à la réception d'un
     *        message. Met à jour l'horodatage d'activité et délègue le
     *        décodage/traitement à traiterRequete().
     * @param _message Trame JSON brute reçue du client.
     */
    void onMessageRecu(const String &_message);

    /**
     * @brief Traite une requête applicative décodée et produit la réponse,
     *        l'accusé de réception ou l'erreur correspondante sur le
     *        WebSocket, selon la commande demandée (heure système, état,
     *        mode, programmation, ouverture/fermeture de vanne...).
     * @param _requete Requête décodée par ProtocoleArrosageServeur::decoder().
     */
    void traiterRequete(const RequeteArrosage &_requete);

    // ── Utilitaires vannes ────────────────────────────────────────────────────

    /**
     * @brief Retrouve le pointeur de vanne correspondant à un identifiant.
     * @param _idVanne Identifiant de vanne, de 1 à 4.
     * @return Pointeur vers la Vanne correspondante, ou nullptr si
     *         l'identifiant est hors plage.
     */
    Vanne *vanneParId(int _idVanne) const;

    /**
     * @brief Indique si les 4 vannes sont actuellement fermées et sans
     *        impulsion en cours.
     * @return true si aucune vanne n'est ouverte ni en cours de manœuvre.
     */
    bool toutesVannesFermees() const;

    /// Rafraîchit l'affichage OLED avec l'état réel courant des 4 vannes et
    /// l'heure système.
    void mettreAJourAffichageVannes();

    /**
     * @brief Rafraîchit l'affichage OLED en forçant l'état affiché d'une
     *        vanne à une valeur cible, pendant que son impulsion physique
     *        est encore en cours (évite l'affichage transitoire de l'état
     *        inverse pendant le temps de bascule bistable).
     * @param _idVanneCible Identifiant (1 à 4) de la vanne dont l'état est forcé.
     * @param _etatCible    État à afficher pour cette vanne (true = ouverte).
     */
    void mettreAJourAffichageVannesAvecCible(uint8_t _idVanneCible, bool _etatCible);

    /**
     * @brief Persiste en NVS l'ouverture d'une vanne (état + horodatage),
     *        via StockageEtatVannes.
     * @param _idVanne Identifiant de la vanne concernée (1 à 4).
     */
    void sauvegarderOuvertureVanne(uint8_t _idVanne);

    /**
     * @brief Persiste en NVS la fermeture d'une vanne, via
     *        StockageEtatVannes.
     * @param _idVanne Identifiant de la vanne concernée (1 à 4).
     */
    void sauvegarderFermetureVanne(uint8_t _idVanne);

    /// Vérifie, pour chacune des 4 vannes en mode Programme, si l'heure
    /// système entre dans (ouverture) ou sort de (fermeture) la fenêtre
    /// d'arrosage programmée, déclenche l'impulsion correspondante, la
    /// notification WebSocket associée et la persistance NVS. Appelée à la
    /// fois depuis traiterArrosage() (après réveil RTC) et depuis
    /// traiterDialogue() (throttlée), afin que la programmation se
    /// déclenche que l'application soit connectée ou non.
    void verifierProgrammations();

    // ── Affichage batterie (BP2) ──────────────────────────────────────────────

    /**
     * @brief Gère l'affichage ponctuel de l'état de la batterie déclenché
     *        par BP2, pendant les phases éveillées (ATTENTE_CONNEXION,
     *        DIALOGUE).
     * @details Met à jour l'anti-rebond de BP2 (BoutonPoussoir::update()).
     *          Sur un front d'appui, prend une mesure fraîche
     *          (MesureBatteries::lireMesure()) et l'affiche
     *          (AfficheurOLED::afficherMesureBatterie()), en armant
     *          affichageBatterieActif et finAffichageBatterieMs. Une fois
     *          DUREE_AFFICHAGE_BATTERIE_MS écoulées sans nouvel appui,
     *          restaure l'affichage habituel via
     *          mettreAJourAffichageVannes(). N'a aucun effet sur l'automate
     *          d'états ni sur le pilotage des vannes : respecte les cycles
     *          en cours (arrosage, dialogue WebSocket) en n'agissant que
     *          sur le contenu de l'écran.
     */
    void verifierAffichageBatterie();
};

#endif // BOITIER_PILOTAGE_ARROSAGE_H
