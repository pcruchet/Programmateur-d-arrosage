/**
 * @file    ServeurWebSocket.h
 * @brief   Serveur WebSocket exposant l'API de pilotage du boîtier
 *          d'arrosage à l'application Qt/QML.
 *
 * @details Encapsule ESPAsyncWebServer/AsyncWebSocket : démarrage du WiFi
 *          (mode point d'accès ou station), acceptation d'un client unique
 *          à la fois, diffusion des messages sortants et relais des
 *          messages entrants vers un callback applicatif. Les callbacks
 *          WebSocket (onEvent) sont exécutés par la tâche interne
 *          d'AsyncTCP, distincte de la tâche loop() Arduino : les échanges
 *          d'état avec le reste du firmware doivent en tenir compte.
 */

#ifndef SERVEURWEBSOCKET_H
#define SERVEURWEBSOCKET_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AfficheurOLED.h"
#include "Debug.h"
#include "secrets.h"

/// Port TCP d'écoute du serveur WebSocket.
#define PORT_SERVEUR 5000
/// SSID_AP / PASSWORD_AP (point d'accès WiFi de secours, utilisé si la
/// connexion en mode STA échoue) définis dans secrets.h (non versionné,
/// voir include/secrets.h.example).

/**
 * @class ServeurWebSocket
 * @brief Serveur WebSocket mono-client, avec bascule automatique STA/AP et
 *        relais des messages vers un callback applicatif.
 */
class ServeurWebSocket {
public:
    /**
     * @brief Construit le serveur (port 5000, chemin "/ws") sans démarrer
     *        le WiFi.
     * @param _afficheur Référence vers l'afficheur OLED, mis à jour
     *                    directement par les événements de connexion/
     *                    déconnexion (mode WiFi, IP, client connecté).
     */
    ServeurWebSocket(AfficheurOLED &_afficheur);

    // ── Démarrage ──

    /**
     * @brief Démarre le WiFi en mode point d'accès (AP) et le serveur
     *        WebSocket.
     * @details Utilisée en repli lorsque la connexion en mode STA échoue.
     *          Met à jour l'affichage avec le mode "AP" et l'IP du point
     *          d'accès.
     */
    void demarrerAP();

    /**
     * @brief Démarre le WiFi en mode station (STA) et le serveur
     *        WebSocket.
     * @details Tente la connexion au réseau donné pendant au plus 10 s
     *          (20 tentatives de 500 ms). En cas de succès, démarre le
     *          serveur WebSocket sur l'IP obtenue. En cas d'échec,
     *          déconnecte le WiFi STA et bascule automatiquement vers
     *          demarrerAP().
     * @param _ssid     SSID du réseau WiFi cible.
     * @param _password Mot de passe du réseau WiFi cible.
     */
    void demarrerSTA(const String &_ssid, const String &_password);

    // ── Communication ──

    /**
     * @brief Diffuse un message texte à tous les clients WebSocket
     *        connectés (en pratique, un seul client est accepté à la
     *        fois).
     * @details Sans effet si aucun client n'est connecté (évite un envoi
     *          inutile lorsque connected vaut false).
     * @param _message Message texte (trame JSON) à envoyer.
     */
    void envoyerMessage(const String &_message);

    /**
     * @brief Indique si un client WebSocket est actuellement connecté.
     * @return true si un client est connecté.
     */
    bool clientConnecte() const;

    /**
     * @brief Ferme immédiatement toute connexion WebSocket active, sans
     *        attendre de timeout côté client.
     * @details Utilisée avant l'entrée en deep sleep, afin que
     *          l'application Qt affiche aussitôt le badge de statut
     *          "déconnecté" plutôt que de découvrir la coupure au bout du
     *          délai de ping (voir ServeurWebSocket::onEvent() côté firmware
     *          et le mécanisme de ping périodique côté application).
     */
    void fermerConnexions();

    // ── Callback ──

    /**
     * @brief Enregistre le callback applicatif appelé à chaque message
     *        texte reçu d'un client.
     * @param _callback Fonction invoquée avec le contenu du message reçu.
     */
    void setOnMessage(std::function<void(const String &)> _callback);

private:
    AsyncWebServer serveur;   ///< Serveur HTTP asynchrone sous-jacent (ESPAsyncWebServer), écoutant sur PORT_SERVEUR.
    AsyncWebSocket  ws;         ///< Point de terminaison WebSocket ("/ws") attaché au serveur HTTP.
    AfficheurOLED  &afficheur; ///< Référence vers l'afficheur OLED, mis à jour lors des événements de connexion.
    volatile bool   connected;         ///< Indique si un client WebSocket est actuellement connecté (accès à un seul client à la fois) ; mis à jour depuis la tâche AsyncTCP (onEvent()) et lu depuis loop(), donc marqué volatile. Initialisé dans la liste d'initialisation du constructeur.

    std::function<void(const String &)> onMessage; ///< Callback applicatif invoqué à chaque message texte reçu.

    // ── Initialisation commune AP/STA ──

    /**
     * @brief Attache le gestionnaire d'événements WebSocket et démarre le
     *        serveur HTTP.
     * @details Factorise la partie commune à demarrerAP() et
     *          demarrerSTA() : enregistrement du callback onEvent() sur
     *          l'objet AsyncWebSocket, ajout du handler au serveur HTTP et
     *          démarrage effectif de ce dernier.
     */
    void initialiserWebSocket();

    // ── Gestionnaire d'événements WebSocket ──

    /**
     * @brief Gestionnaire des événements du cycle de vie WebSocket
     *        (connexion, déconnexion, données reçues, erreur).
     * @details Exécuté par la tâche interne d'AsyncTCP (asynchrone par
     *          rapport à la boucle loop() Arduino). N'accepte qu'un seul
     *          client à la fois : toute connexion supplémentaire alors
     *          qu'un client est déjà connecté est immédiatement fermée.
     *          Met à jour le drapeau connected et l'affichage
     *          (setClientConnecte()) sur connexion/déconnexion/erreur, et
     *          relaie tout message texte non vide reçu (WS_EVT_DATA) vers
     *          le callback onMessage.
     * @param _server Pointeur vers l'objet AsyncWebSocket source.
     * @param _client Pointeur vers le client concerné par l'événement.
     * @param _type   Type d'événement (WS_EVT_CONNECT, WS_EVT_DISCONNECT,
     *                WS_EVT_DATA, WS_EVT_ERROR, ...).
     * @param _arg    Argument additionnel dépendant du type d'événement
     *                (non utilisé ici).
     * @param _data   Pointeur vers les données reçues (pour WS_EVT_DATA).
     * @param _len    Longueur des données reçues, en octets.
     */
    void onEvent(AsyncWebSocket       *_server,
                 AsyncWebSocketClient *_client,
                 AwsEventType          _type,
                 void                 *_arg,
                 uint8_t              *_data,
                 size_t                _len);
};

#endif // SERVEURWEBSOCKET_H
