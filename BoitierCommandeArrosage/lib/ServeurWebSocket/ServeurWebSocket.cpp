/**
 * @file    ServeurWebSocket.cpp
 * @brief   Implémentation du serveur WebSocket mono-client (WiFi STA/AP,
 *          relais des messages, diffusion des réponses).
 */

#include "ServeurWebSocket.h"



/**
 * @brief Construit le serveur HTTP (port PORT_SERVEUR) et le point de
 *        terminaison WebSocket ("/ws"), sans démarrer le WiFi.
 * @param _afficheur Référence vers l'afficheur OLED à mettre à jour lors
 *                    des événements de connexion.
 */
ServeurWebSocket::ServeurWebSocket(AfficheurOLED &_afficheur)
    : serveur(5000), ws("/ws")
    , afficheur(_afficheur)
    , connected(false)
{
}

/**
 * @brief Démarre le point d'accès WiFi de secours et le serveur WebSocket.
 * @details Utilise les identifiants SSID_AP/PASSWORD_AP, met à jour
 *          l'affichage (mode "AP", IP du point d'accès) puis démarre
 *          effectivement le serveur via initialiserWebSocket().
 */
void ServeurWebSocket::demarrerAP()
{
    WiFi.softAP(SSID_AP, PASSWORD_AP);
    afficheur.setModeWifi("AP");
    afficheur.setAdresseIP(WiFi.softAPIP().toString());
    afficheur.rafraichir();
    initialiserWebSocket();
}

/**
 * @brief Tente une connexion WiFi en mode station, avec repli automatique
 *        en mode point d'accès en cas d'échec.
 * @details Lance WiFi.begin() puis attend la connexion par intervalles de
 *          500 ms, jusqu'à 20 tentatives (10 s au total). En cas de
 *          succès, affiche l'IP obtenue et démarre le serveur WebSocket.
 *          En cas d'échec, déconnecte proprement le WiFi STA avant de
 *          basculer vers demarrerAP().
 * @param _ssid     SSID du réseau cible.
 * @param _password Mot de passe du réseau cible.
 */
void ServeurWebSocket::demarrerSTA(const String &_ssid, const String &_password)
{
    WiFi.begin(_ssid.c_str(), _password.c_str());
    afficheur.setModeWifi("STA");
    afficheur.setAdresseIP("...");
    afficheur.rafraichir();

    uint8_t tentatives = 0;
    while (WiFi.status() != WL_CONNECTED && tentatives < 20)
    {
        delay(500);
        tentatives++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        afficheur.setAdresseIP(WiFi.localIP().toString());
        afficheur.rafraichir();
        initialiserWebSocket();
    }
    else
    {
        // STA échoue → repasse en AP
        WiFi.disconnect();
        demarrerAP();
    }
}

/**
 * @brief Attache le gestionnaire d'événements WebSocket et démarre le
 *        serveur HTTP.
 * @details Enregistre une lambda relayant chaque événement vers onEvent(),
 *          ajoute le handler WebSocket au serveur HTTP puis appelle
 *          serveur.begin().
 */
void ServeurWebSocket::initialiserWebSocket()
{
    ws.onEvent([this](AsyncWebSocket *_server,
                      AsyncWebSocketClient *_client,
                      AwsEventType _type,
                      void *_arg,
                      uint8_t *_data,
                      size_t _len)
               { onEvent(_server, _client, _type, _arg, _data, _len); });

    serveur.addHandler(&ws);
    serveur.begin();
}

/**
 * @brief Gestionnaire des événements du cycle de vie WebSocket.
 * @details WS_EVT_CONNECT : ferme immédiatement toute connexion
 *          supplémentaire si un client est déjà connecté (politique
 *          mono-client) ; sinon, marque connected=true et met à jour
 *          l'affichage. WS_EVT_DISCONNECT et WS_EVT_ERROR : marquent
 *          connected=false et mettent à jour l'affichage. WS_EVT_DATA :
 *          reconstruit le message texte reçu et le transmet au callback
 *          onMessage s'il est défini et que des données sont présentes.
 *          Exécuté sur la tâche interne d'AsyncTCP.
 */
void ServeurWebSocket::onEvent(AsyncWebSocket       *_server,
                                AsyncWebSocketClient *_client,
                                AwsEventType          _type,
                                void                 *_arg,
                                uint8_t              *_data,
                                size_t                _len)
{
    switch (_type)
    {
        case WS_EVT_CONNECT:
            if (connected)
            {
                _client->close();
            }
            else
            {
                connected = true;
                afficheur.setClientConnecte(true);
                afficheur.rafraichir();
            }
            break;

        case WS_EVT_DISCONNECT:
            connected = false;
            afficheur.setClientConnecte(false);
            afficheur.rafraichir();
            break;

        case WS_EVT_DATA:
            if (_len > 0 && onMessage)
            {
                String message = String((char *)_data, _len);
                onMessage(message);
            }
            break;

        case WS_EVT_ERROR:
            connected = false;
            afficheur.setClientConnecte(false);
            afficheur.rafraichir();
            break;

        default:
            break;
    }
}

/**
 * @brief Diffuse un message texte à tous les clients connectés
 *        (ws.textAll()), uniquement si connected vaut true.
 * @param _message Message à diffuser.
 */
void ServeurWebSocket::envoyerMessage(const String &_message)
{
    if (connected)
        ws.textAll(_message);
}

/**
 * @brief Indique si un client est actuellement connecté.
 * @return Valeur du drapeau interne connected.
 */
bool ServeurWebSocket::clientConnecte() const
{
    return connected;
}

/**
 * @brief Enregistre le callback applicatif de réception de message.
 * @param _callback Fonction appelée avec le contenu de chaque message reçu.
 */
void ServeurWebSocket::setOnMessage(std::function<void(const String &)> _callback)
{
    onMessage = _callback;
}

/**
 * @brief Ferme immédiatement toute connexion WebSocket active
 *        (ws.closeAll()), sans attendre l'expiration d'un timeout côté
 *        client.
 * @details Appelée avant l'entrée en deep sleep (voir
 *          BoitierPilotageArrosage::traiterEndormissement()), afin que
 *          l'application Qt détecte aussitôt la déconnexion et mette à
 *          jour son badge de statut, plutôt que de devoir attendre
 *          l'expiration de son propre timer de ping.
 */
void ServeurWebSocket::fermerConnexions()
{
    ws.closeAll();
}