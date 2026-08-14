/**
 * @file    controleurarrosage.h
 * @brief   Orchestrateur central de l'application Qt/QML "Contrôleur d'Arrosage".
 *
 * @details Déclare la classe ControleurArrosage, qui joue quatre rôles :
 *          - modèle de liste pour QML (QAbstractListModel), exposant les
 *            objets Vanne aux composants d'interface ;
 *          - traduction des actions utilisateur (ouverture/fermeture,
 *            changement de mode, programmation) en trames du protocole
 *            applicatif via ProtocoleArrosageClient ;
 *          - émission/réception des trames via CommunicationESP32 ;
 *          - mise à jour des objets Vanne (miroir de l'état réel du
 *            boîtier) à partir des réponses et notifications spontanées
 *            reçues de l'ESP32, et persistance de la configuration de
 *            connexion (adresse IP / port) sur disque.
 */

#ifndef CONTROLEURARROSAGE_H
#define CONTROLEURARROSAGE_H

#include <QAbstractListModel>
#include <QVector>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include "vanne.h"
#include "communicationesp32.h"
#include "protocolearrosageclient.h"

/**
 * @class ControleurArrosage
 * @ingroup cpp_classes
 * @brief Classe C++ : Orchestrateur central de l'application.
 *
 * @details Agrège une CommunicationESP32 (transport WebSocket), un
 *          ProtocoleArrosageClient (encodage/décodage JSON) et un ensemble
 *          d'objets Vanne (état miroir), et implémente QAbstractListModel
 *          pour exposer directement ces vannes à une ListView QML. Les
 *          actions déclenchées depuis QML (Q_INVOKABLE) appliquent en
 *          général une mise à jour locale optimiste immédiate puis envoient
 *          la trame correspondante à l'ESP32, qui confirmera ou corrigera
 *          l'état via une réponse ou une notification spontanée traitée par
 *          traiterMessage(). La reconnexion après coupure est automatique
 *          (timerReconnexion), et l'adresse IP / le port de connexion sont
 *          persistés dans un fichier de configuration (QSettings).
 */
class ControleurArrosage : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool connecte READ estConnecte NOTIFY connecteChanged)
    Q_PROPERTY(bool enConnexion READ estEnConnexion NOTIFY enConnexionChanged)
    Q_PROPERTY(QString adresseIp READ getAdresseIp WRITE setAdresseIp NOTIFY adresseIpChanged)
    Q_PROPERTY(quint16 port READ getPort WRITE setPort NOTIFY portChanged)

public:
    /// Rôles de données exposés au délégué QML de la liste des vannes
    /// (voir roleNames()/data()).
    enum Roles {
        EtatRole = Qt::UserRole + 1, ///< bool : état ouvert/fermé (Vanne::getEtat()).
        ModeRole,                    ///< Vanne::MODE : mode courant (Vanne::getMode()).
        ProgrammeStateRole,          ///< Vanne::PROGRAM_STATE : activation de la programmation (Vanne::getProgrammeState()).
        NomRole,                     ///< QString : nom affiché (Vanne::getNom()).
        DebutRole,                   ///< QDateTime : heure de début programmée (Vanne::getDebut()).
        DureeRole,                   ///< int : durée programmée en minutes (Vanne::getDuree()).
        FrequenceRole,               ///< int : fréquence programmée en heures (Vanne::getFrequence()).
        SynchroniseeRole             ///< bool : état confirmé par l'ESP32 (Vanne::estSynchronisee()).
    };

    /**
     * @brief Construit l'orchestrateur : charge la configuration de
     *        connexion, crée _nbVannes objets Vanne (identifiants 1 à
     *        _nbVannes, nommés "Vanne 1", "Vanne 2", ...), connecte les
     *        signaux de CommunicationESP32 et du timer de reconnexion
     *        (intervalle 5 s), puis lance une première tentative de connexion.
     * @param _nbVannes Nombre de vannes à instancier (identifiants 1 à _nbVannes).
     * @param _parent Objet parent Qt.
     */
    explicit ControleurArrosage(int _nbVannes = 4, QObject *_parent = nullptr);

    // ================= MODÈLE QML =================

    /// @return Nombre de vannes (0 si _parent est un index valide : modèle plat).
    int rowCount(const QModelIndex &_parent = QModelIndex()) const override;

    /// @return Valeur du rôle _role pour la vanne à la ligne _index.row(),
    ///         ou un QVariant invalide si l'index ou le rôle est inconnu.
    QVariant data(const QModelIndex &_index, int _role) const override;

    /// @return Table de correspondance rôle -> nom de propriété QML (voir Roles).
    QHash<int, QByteArray> roleNames() const override;

    /// @return true si une connexion WebSocket est actuellement établie avec l'ESP32.
    bool estConnecte() const;

    /// @return true si une connexion est en cours d'établissement.
    bool estEnConnexion() const;

    // ================= ACTIONS QML =================

    /// @brief Bascule l'état ouvert/fermé de la vanne _index. Si elle était
    ///        en programmation active, la programmation est suspendue
    ///        (passage en mode Manuel) avant le basculement. Met à jour
    ///        l'état local immédiatement (optimiste), notifie la vue QML,
    ///        puis envoie la commande d'ouverture/fermeture à l'ESP32.
    /// @param _index Index de la vanne dans le modèle (0 à rowCount()-1).
    Q_INVOKABLE void toggleEtat(int _index);

    /// @brief Passe la vanne _index en mode Manuel, notifie la vue QML et
    ///        envoie le changement à l'ESP32.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void passerEnManuel(int _index);

    /// @brief Passe la vanne _index en mode Programme avec programmation
    ///        active, notifie la vue QML et envoie le changement à l'ESP32.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void activerProgramme(int _index);

    /// @brief Passe la vanne _index en mode Automatique avec programmation
    ///        active, notifie la vue QML et envoie le changement à l'ESP32.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void activerAutomatique(int _index);

    /// @brief Suspend la programmation de la vanne _index. Le protocole ne
    ///        connaissant pas d'état "Suspendu", ceci se traduit côté ESP32
    ///        par un passage en mode Manuel. Notifie la vue QML et envoie
    ///        le changement à l'ESP32.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void suspendProgramme(int _index);

    /// @brief Réactive la programmation de la vanne _index (mode Programme,
    ///        état Actif), notifie la vue QML et envoie le changement à l'ESP32.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void resumeProgramme(int _index);

    /// @brief Modifie localement la durée programmée de la vanne _index
    ///        (non envoyé à l'ESP32 : voir envoyerProgrammation()).
    /// @param _index Index de la vanne dans le modèle.
    /// @param _duree Nouvelle durée, en minutes.
    Q_INVOKABLE void setDuree(int _index, int _duree);

    /// @brief Modifie localement la fréquence programmée de la vanne _index
    ///        (non envoyé à l'ESP32 : voir envoyerProgrammation()).
    /// @param _index Index de la vanne dans le modèle.
    /// @param _frequence Nouvelle fréquence, en heures.
    Q_INVOKABLE void setFrequence(int _index, int _frequence);

    /// @brief Modifie localement l'heure de début programmée de la vanne
    ///        _index si _debut est un format de date/heure valide
    ///        ("dd/MM/yyyy HH:mm"), sinon ignore silencieusement l'appel
    ///        (non envoyé à l'ESP32 : voir envoyerProgrammation()).
    /// @param _index Index de la vanne dans le modèle.
    /// @param _debut Nouvelle heure de début, au format "dd/MM/yyyy HH:mm".
    Q_INVOKABLE void setDebut(int _index, const QString &_debut);

    /// @brief Envoie à l'ESP32 la programmation courante (début, durée,
    ///        fréquence) de la vanne _index. À appeler après un ou
    ///        plusieurs setDuree()/setFrequence()/setDebut() pour valider
    ///        les changements auprès du boîtier.
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void envoyerProgrammation(int _index);

    /// @brief Envoie à l'ESP32 une demande de réglage de l'heure système, si
    ///        _dateHeureIso est bien au format ISO 8601 complet (19
    ///        caractères) ; sinon émet erreurRecue().
    /// @param _dateHeureIso Date/heure au format ISO 8601 ("yyyy-MM-ddTHH:mm:ss").
    Q_INVOKABLE void definirHeureSysteme(const QString &_dateHeureIso);

    /// @brief Redemande à l'ESP32 l'état, le mode et la programmation
    ///        complets de la vanne _index (trois requêtes successives).
    /// @param _index Index de la vanne dans le modèle.
    Q_INVOKABLE void rafraichir(int _index);

    /// @brief Expose la vanne _index à QML sous forme de QVariant (pointeur
    ///        QObject*), pour accès direct à ses propriétés/méthodes invocables.
    /// @param _index Index de la vanne dans le modèle.
    /// @return QVariant enveloppant le pointeur Vanne*, ou un QVariant nul
    ///         si _index est hors limites.
    Q_INVOKABLE QVariant vanne(int _index);

    /// @brief Relance une connexion vers l'adresse IP / le port courants
    ///        (propriétés adresseIp/port), en fermant d'abord la connexion
    ///        existante si nécessaire.
    Q_INVOKABLE void configurerConnexion();

    QString getAdresseIp() const;                        ///< @return Adresse IP configurée du boîtier.
    void setAdresseIp(const QString &_newAdresseIp);      ///< @brief Modifie l'adresse IP, persiste la configuration et émet adresseIpChanged() si elle change.
    quint16 getPort() const;                              ///< @return Port WebSocket configuré du boîtier.
    void setPort(quint16 _newPort);                       ///< @brief Modifie le port, persiste la configuration et émet portChanged() si il change.


signals:
    void connecteChanged();                        ///< Émis quand l'état de connexion (connecte) change.
    void enConnexionChanged();                      ///< Émis quand l'état "connexion en cours" change.
    void erreurRecue(const QString &_message);      ///< Émis à réception d'une erreur applicative ou de communication, avec un message lisible.
    void adresseIpChanged();                        ///< Émis quand l'adresse IP configurée change.
    void portChanged();                             ///< Émis quand le port configuré change.

private slots:
    // ============ RÉCEPTION ESP32 ============

    /// @brief Point d'entrée de tout message WebSocket reçu : parse la
    ///        trame JSON puis la dispatche selon sa nature (données de
    ///        vanne, état système complet, erreur).
    /// @param _message Message JSON brut reçu de l'ESP32.
    void traiterMessage(const QString &_message);

    /// @brief Réagit à l'établissement de la connexion : arrête le timer de
    ///        reconnexion, règle l'heure système du boîtier sur l'heure
    ///        locale, et demande l'état complet de toutes les vannes.
    void onConnexionEtablie();

    /// @brief Réagit à la perte de connexion : démarre le timer de
    ///        reconnexion (si une adresse est connue) et marque toutes les
    ///        vannes comme non synchronisées.
    void onConnexionPerdue();

    /// @brief Relaye une erreur de communication (transport) sous forme de erreurRecue().
    /// @param _erreur Message d'erreur du transport.
    void onErreurCommunication(const QString &_erreur);

    /// @brief Relaye le changement d'état "connexion en cours" du transport.
    void onEnConnexionChanged();

private:
    // Configuration de l'application
    QString cheminConfig() const;         ///< @return Chemin du fichier de configuration (créé si besoin), dans le dossier de configuration standard de l'application.
    void chargerConfiguration();          ///< @brief Charge adresseIp/port depuis le fichier de configuration (valeurs par défaut si absent) et reconstruit derniereUrl.
    void sauvegarderConfiguration();      ///< @brief Persiste adresseIp/port dans le fichier de configuration.
    void reconstruireUrl();               ///< @brief Reconstruit derniereUrl (schéma "ws", host, port) à partir de adresseIp/port.

    // Accès aux vannes
    Vanne* get(int _index) const;              ///< @return Vanne à l'index _index, ou nullptr si hors limites.
    Vanne* getParId(int _id) const;            ///< @return Vanne dont l'identifiant protocole vaut _id, ou nullptr si non trouvée.
    int indexParId(int _id) const;             ///< @return Index dans le modèle de la vanne d'identifiant _id, ou -1 si non trouvée.
    void notifyRowChanged(int _index);         ///< @brief Émet dataChanged() pour la ligne _index, afin de rafraîchir la vue QML.

    // Émission
    void envoyerTrame(const QByteArray &_trame);   ///< @brief Envoie une trame déjà encodée au boîtier via CommunicationESP32.
    void envoyerMode(Vanne *_v);                   ///< @brief Encode et envoie une commande de changement de mode pour la vanne _v, à partir de son mode courant.
    void synchroniserVannes();                     ///< @brief Envoie une demande d'état système complet (toutes les vannes en une trame).
    void tenterReconnexion();                      ///< @brief Slot du timer de reconnexion : retente une connexion si toujours déconnecté.

    // Réception : dispatch par type de trame

    /// @brief Traite une trame de réponse/notification concernant une seule
    ///        vanne (ouverture, fermeture, état, mode ou programmation) :
    ///        met à jour l'objet Vanne correspondant et notifie la vue QML.
    /// @param _trame Trame décodée.
    void traiterDonneesVanne(const QJsonObject &_trame);

    /// @brief Traite une réponse à une demande d'état système complet :
    ///        met à jour l'état, le mode et la programmation de chaque
    ///        vanne listée dans la trame.
    /// @param _trame Trame décodée (champ "s" : tableau de vannes).
    void traiterEtatSysteme(const QJsonObject &_trame);

    /// @brief Traite une trame d'erreur : émet erreurRecue() avec le
    ///        message correspondant, et redemande l'état de la vanne
    ///        concernée si son identifiant est connu.
    /// @param _trame Trame décodée.
    void traiterErreur(const QJsonObject &_trame);

    /// @brief Traduit un mode Vanne::MODE en caractère du protocole applicatif.
    /// @param _mode Mode à traduire.
    /// @return Constante ProtocoleArrosageClient::MODE_* correspondante.
    static char modeVersProtocole(Vanne::MODE _mode);

    /// @brief Traduit un caractère du protocole applicatif en mode Vanne::MODE.
    /// @param _mode Constante ProtocoleArrosageClient::MODE_* à traduire.
    /// @return Vanne::MODE correspondant (Vanne::Manuel si _mode n'est pas reconnu).
    static Vanne::MODE protocoleVersMode(char _mode);

    QVector<Vanne*> vannes;              ///< Vannes gérées par le modèle, dans l'ordre d'exposition à QML.
    CommunicationESP32 com;                ///< Transport WebSocket vers le boîtier ESP32.
    ProtocoleArrosageClient protocole;     ///< Encodeur/décodeur des trames JSON du protocole applicatif.
    bool connecte;                         ///< État de connexion courant (voir estConnecte()/connecteChanged()).
    QTimer timerReconnexion;               ///< Timer périodique (5 s) déclenchant tenterReconnexion() tant que déconnecté.
    QUrl derniereUrl;                      ///< URL WebSocket courante (reconstruite par reconstruireUrl() à partir de adresseIp/port).
    QString adresseIp;                     ///< Adresse IP configurée du boîtier (persistée).
    quint16 port;                          ///< Port WebSocket configuré du boîtier (persisté).
};

#endif // CONTROLEURARROSAGE_H
