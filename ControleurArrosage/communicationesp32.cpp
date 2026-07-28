/**
 * @file    communicationesp32.cpp
 * @brief   Implémentation de CommunicationESP32.
 *
 * @details Connexion des signaux de la QWebSocket et du timer de ping dans
 *          le constructeur, encodage des URL de connexion, et relais des
 *          signaux Qt réseau vers les signaux applicatifs de la classe.
 */

#include "communicationesp32.h"

CommunicationESP32::CommunicationESP32(QObject *_parent)
    : QObject(_parent)
    ,reconnexionEnAttente(false)
{
    connect(&socket, &QWebSocket::connected,    this, &CommunicationESP32::onConnected);
    connect(&socket, &QWebSocket::disconnected, this, &CommunicationESP32::onDisconnected);
    connect(&socket, &QWebSocket::textMessageReceived, this, &CommunicationESP32::onMessageReceived);
    connect(&socket, &QWebSocket::errorOccurred, this, &CommunicationESP32::onErrorOccurred);
    connect(&socket, &QWebSocket::stateChanged, this, &CommunicationESP32::onStateChanged);
    connect(&timerPing, &QTimer::timeout, this, &CommunicationESP32::verifierConnexion);
    timerPing.setInterval(10000);
    timerPing.start();
}

void CommunicationESP32::verifierConnexion()
{
    bool connecte = (socket.state() == QAbstractSocket::ConnectedState);

    if (connecte)
        socket.ping();
}

void CommunicationESP32::connecterESP32(QUrl &_url)
{
    if (socket.state() == QAbstractSocket::UnconnectedState)
    {
        _url.setPath("/ws");
        socket.open(_url);
    }
}

void CommunicationESP32::connecterESP32(const QString &_adresseIP, quint16 _port)
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(_adresseIP);
    url.setPort(_port);
    connecterESP32(url);
}

void CommunicationESP32::deconnecterESP32()
{
    socket.close();
}

void CommunicationESP32::reconnecter(const QString &_adresseIP, quint16 _port)
{
    urlEnAttente.setScheme("ws");
    urlEnAttente.setHost(_adresseIP);
    urlEnAttente.setPort(_port);
    reconnexionEnAttente = true;

    if (socket.state() == QAbstractSocket::UnconnectedState)
        connecterESP32(urlEnAttente);
    else
        socket.close();
}

void CommunicationESP32::envoyerMessage(const QString &_message)
{
    bool connecte = (socket.state() == QAbstractSocket::ConnectedState);

    if (connecte)
        socket.sendTextMessage(_message);
    else
        qDebug() << "Message NON envoyé, socket déconnecté :" << _message;
}

bool CommunicationESP32::estConnecte() const
{
    bool resultat = (socket.state() == QAbstractSocket::ConnectedState);

    return resultat;
}

bool CommunicationESP32::estEnConnexion() const
{
    QAbstractSocket::SocketState etat = socket.state();
    bool resultat = (etat == QAbstractSocket::ConnectingState ||
                     etat == QAbstractSocket::HostLookupState);

    return resultat;
}

void CommunicationESP32::onConnected()
{
    emit connected();
}

void CommunicationESP32::onDisconnected()
{
    if (reconnexionEnAttente)
    {
        reconnexionEnAttente = false;
        connecterESP32(urlEnAttente);
    }

    emit disconnected();
}

void CommunicationESP32::onMessageReceived(const QString &_message)
{
    emit messageReceived(_message);
}

void CommunicationESP32::onErrorOccurred(QAbstractSocket::SocketError _error)
{
    Q_UNUSED(_error)
    emit errorOccurred(socket.errorString());
}

void CommunicationESP32::onStateChanged(QAbstractSocket::SocketState _state)
{
    Q_UNUSED(_state)
    emit enConnexionChanged();
}
