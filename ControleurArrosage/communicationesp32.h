/**
 * @file    communicationesp32.h
 * @brief   Client WebSocket bas niveau vers le boîtier ESP32.
 *
 * @details Déclare la classe CommunicationESP32, qui encapsule la QWebSocket
 *          Qt : connexion/déconnexion, envoi de messages texte, détection de
 *          coupure via un ping périodique, et reconnexion automatique après
 *          fermeture volontaire. Ne connaît rien du protocole applicatif
 *          (JSON) : elle transporte des chaînes de caractères brutes, à
 *          charge de ControleurArrosage/ProtocoleArrosageClient de les
 *          interpréter.
 */

#ifndef COMMUNICATIONESP32_H
#define COMMUNICATIONESP32_H

#include <QObject>
#include <QWebSocket>
#include <QAbstractSocket>
#include <QTimer>
#include <QUrl>

/**
 * @class CommunicationESP32
 * @ingroup cpp_classes
 * @brief Classe C++ : Encapsule la connexion WebSocket vers le boîtier ESP32.
 *
 * @details Gère l'ouverture/fermeture de la connexion, l'émission de
 *          messages texte, et un timer périodique (10 s) qui envoie un ping
 *          WebSocket tant que la connexion est active, permettant de
 *          détecter une coupure silencieuse. Une reconnexion demandée
 *          pendant qu'une connexion est déjà active attend la déconnexion
 *          effective (onDisconnected()) avant de se relancer.
 */
class CommunicationESP32 : public QObject
{
    Q_OBJECT

public:
    /// @brief Construit le client et connecte les signaux internes de la
    ///        QWebSocket et du timer de ping.
    /// @param _parent Objet parent Qt.
    explicit CommunicationESP32(QObject *_parent = nullptr);

    /// @brief Ouvre la connexion vers l'URL donnée (le chemin "/ws" est
    ///        ajouté automatiquement) si aucune connexion n'est en cours.
    /// @param _url URL de base du boîtier (ex. ws://192.168.4.1:80) ; modifiée
    ///             en place (ajout du chemin "/ws").
    void connecterESP32(QUrl &_url);

    /// @brief Construit l'URL ws://_adresseIP:_port et ouvre la connexion.
    /// @param _adresseIP Adresse IP du boîtier.
    /// @param _port Port WebSocket du boîtier.
    void connecterESP32(const QString &_adresseIP,quint16 _port);

    /// @brief Ferme la connexion en cours (sans reconnexion automatique).
    void deconnecterESP32();

    /// @brief Demande une reconnexion vers une nouvelle adresse : si aucune
    ///        connexion n'est active, se connecte immédiatement ; sinon,
    ///        ferme la connexion courante et se reconnectera dès
    ///        onDisconnected().
    /// @param _adresseIP Nouvelle adresse IP du boîtier.
    /// @param _port Nouveau port WebSocket du boîtier.
    void reconnecter(const QString &_adresseIP, quint16 _port);

    /// @brief Envoie un message texte si la connexion est active ; sinon,
    ///        trace un message de debug et n'envoie rien.
    /// @param _message Message à envoyer (trame JSON du protocole applicatif).
    void envoyerMessage(const QString &_message);

    /// @return true si la QWebSocket est dans l'état ConnectedState.
    bool estConnecte() const;

    /// @return true si une connexion est en cours d'établissement
    ///         (ConnectingState ou HostLookupState).
    bool estEnConnexion() const;

signals:
    void messageReceived(const QString &message);      ///< Émis à la réception d'un message texte du boîtier.
    void connected();                                   ///< Émis quand la connexion WebSocket est établie.
    void disconnected();                                ///< Émis quand la connexion WebSocket se ferme (volontairement ou non).
    void errorOccurred(const QString &error);           ///< Émis en cas d'erreur socket, avec le message d'erreur Qt.
    void enConnexionChanged();                          ///< Émis à chaque changement d'état de la socket (utilisé pour refléter estEnConnexion()).

private slots:
    void onConnected();                                          ///< Relaye le signal connected() de la QWebSocket.
    void onDisconnected();                                       ///< Déclenche la reconnexion en attente le cas échéant, puis relaye disconnected().
    void onMessageReceived(const QString &_message);             ///< Relaye le signal textMessageReceived() de la QWebSocket.
    void onErrorOccurred(QAbstractSocket::SocketError _error);   ///< Relaye l'erreur socket sous forme de errorOccurred(QString).
    void onStateChanged(QAbstractSocket::SocketState _state);    ///< Émet enConnexionChanged() à chaque changement d'état.
    void verifierConnexion();                                    ///< Slot du timer périodique : envoie un ping si la connexion est active.

private:
    QWebSocket socket;            ///< Socket WebSocket Qt vers le boîtier ESP32.
    QTimer timerPing;             ///< Timer périodique (10 s) déclenchant verifierConnexion().
    QUrl urlEnAttente;            ///< URL cible d'une reconnexion demandée via reconnecter(), utilisée dès onDisconnected().
    bool reconnexionEnAttente;    ///< true si une reconnexion doit être déclenchée à la prochaine déconnexion.
};

#endif // COMMUNICATIONESP32_H
