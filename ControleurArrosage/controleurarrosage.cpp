/**
 * @file    controleurarrosage.cpp
 * @brief   Implémentation de ControleurArrosage.
 *
 * @details Constructeur (chargement de la configuration, création des
 *          vannes, connexion des signaux), implémentation du modèle QML,
 *          actions invocables depuis QML, dispatch et traitement des
 *          trames reçues de l'ESP32, et utilitaires internes (accès aux
 *          vannes, émission, conversions de types).
 */

#include "controleurarrosage.h"

#include <QTimer>

ControleurArrosage::ControleurArrosage(int _nbVannes, QObject *_parent)
    : QAbstractListModel(_parent)
    , connecte(false)

{
    chargerConfiguration();
    sauvegarderConfiguration();
    m_vannes.reserve(_nbVannes);

    for (int i = 0; i < _nbVannes; i++)
    {
        Vanne *laVanne = new Vanne(i + 1, this);
        laVanne->setNom(QString("Vanne %1").arg(i + 1));
        m_vannes.append(laVanne);
    }

    connect(&com, &CommunicationESP32::messageReceived,this, &ControleurArrosage::traiterMessage);
    connect(&com, &CommunicationESP32::connected,this, &ControleurArrosage::onConnexionEtablie);
    connect(&com, &CommunicationESP32::disconnected,this, &ControleurArrosage::onConnexionPerdue);
    connect(&com, &CommunicationESP32::errorOccurred,this, &ControleurArrosage::onErreurCommunication);
    connect(&com, &CommunicationESP32::enConnexionChanged, this, &ControleurArrosage::onEnConnexionChanged);
    connect(&timerReconnexion, &QTimer::timeout,this, &ControleurArrosage::tenterReconnexion);

    timerReconnexion.setInterval(5000);
    com.connecterESP32(derniereUrl);
}

// ============================================================
// Modèle QML
// ============================================================

int ControleurArrosage::rowCount(const QModelIndex &_parent) const
{
    int retour = 0;

    if (!_parent.isValid())
        retour = m_vannes.size();

    return retour;
}

QHash<int, QByteArray> ControleurArrosage::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[EtatRole] = "etat";
    roles[ModeRole] = "mode";
    roles[ProgrammeStateRole] = "programmeState";
    roles[NomRole] = "nom";
    roles[DebutRole] = "debut";
    roles[DureeRole] = "duree";
    roles[FrequenceRole] = "frequence";
    roles[SynchroniseeRole] = "synchronisee";

    return roles;
}

QVariant ControleurArrosage::data(const QModelIndex &_index, int _role) const
{
    QVariant retour;

    Vanne *v = get(_index.row());

    if (v)
    {
        switch (_role)
        {
        case EtatRole:           retour = v->getEtat(); break;
        case ModeRole:           retour = v->getMode(); break;
        case ProgrammeStateRole: retour = v->getProgrammeState(); break;
        case NomRole:            retour = v->getNom(); break;
        case DebutRole:          retour = v->getDebut(); break;
        case DureeRole:          retour = v->getDuree(); break;
        case FrequenceRole:      retour = v->getFrequence(); break;
        case SynchroniseeRole:   retour = v->estSynchronisee(); break;
        }
    }

    return retour;
}

bool ControleurArrosage::estConnecte() const
{
    return connecte;
}

bool ControleurArrosage::estEnConnexion() const
{
    return com.estEnConnexion();
}

// ============================================================
// Actions QML  (logique locale + envoi ESP32)
// ============================================================

void ControleurArrosage::toggleEtat(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        bool nouvelEtat = !v->getEtat();

        // 1. Logique locale (mise à jour optimiste : l'UI réagit
        //    immédiatement, l'ESP32 confirmera ou corrigera)
        if (v->getMode() == Vanne::Programme &&
            v->getProgrammeState() == Vanne::Actif)
        {
            v->setMode(Vanne::Manuel);
            v->setProgrammeState(Vanne::Suspendu);
            envoyerMode(v);
        }

        v->setEtat(nouvelEtat);
        notifyRowChanged(_index);

        // 2. Envoi ESP32
        QByteArray trame;

        if (nouvelEtat)
            trame = protocole.ouvrirVanne(v->getId());
        else
            trame = protocole.fermerVanne(v->getId());

        envoyerTrame(trame);
    }
}

void ControleurArrosage::passerEnManuel(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setMode(Vanne::Manuel);
        notifyRowChanged(_index);
        envoyerMode(v);
    }
}

void ControleurArrosage::activerProgramme(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setMode(Vanne::Programme);
        v->setProgrammeState(Vanne::Actif);
        notifyRowChanged(_index);
        envoyerMode(v);
    }
}

void ControleurArrosage::activerAutomatique(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setMode(Vanne::Automatique);
        v->setProgrammeState(Vanne::Actif);
        notifyRowChanged(_index);
        envoyerMode(v);
    }
}

void ControleurArrosage::suspendProgramme(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        // "Suspendu" n'existe pas côté protocole :
        // on le traduit par un passage en mode Manuel sur l'ESP32
        v->setProgrammeState(Vanne::Suspendu);
        v->setMode(Vanne::Manuel);
        notifyRowChanged(_index);
        envoyerMode(v);
    }
}

void ControleurArrosage::resumeProgramme(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setMode(Vanne::Programme);
        v->setProgrammeState(Vanne::Actif);
        notifyRowChanged(_index);
        envoyerMode(v);
    }
}

void ControleurArrosage::setDuree(int _index, int _duree)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setDuree(_duree);
        notifyRowChanged(_index);
    }
}

void ControleurArrosage::setFrequence(int _index, int _frequence)
{
    Vanne *v = get(_index);

    if (v)
    {
        v->setFrequence(_frequence);
        notifyRowChanged(_index);
    }
}

void ControleurArrosage::setDebut(int _index, const QString &_debut)
{
    Vanne *v = get(_index);

    if (v)
    {
        QDateTime debut = QDateTime::fromString(_debut, "dd/MM/yyyy HH:mm");

        if (debut.isValid())
        {
            v->setDebut(debut);
            notifyRowChanged(_index);
        }
    }
}

void ControleurArrosage::envoyerProgrammation(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        QByteArray trame = protocole.setProgrammation(v->getId(),
                                                      v->getDebut(),
                                                      v->getDuree(),
                                                      v->getFrequence());
        envoyerTrame(trame);
    }
}

void ControleurArrosage::definirHeureSysteme(const QString &_dateHeureIso)
{
    bool formatValide = (_dateHeureIso.length() == 19);

    if (formatValide)
        envoyerTrame(protocole.setHeureSysteme(_dateHeureIso));
    else
        emit erreurRecue("Format de date/heure invalide");
}

void ControleurArrosage::rafraichir(int _index)
{
    Vanne *v = get(_index);

    if (v)
    {
        envoyerTrame(protocole.demanderEtatVanne(v->getId()));
        envoyerTrame(protocole.demanderMode(v->getId()));
        envoyerTrame(protocole.demanderProgrammation(v->getId()));
    }
}

QVariant ControleurArrosage::vanne(int _index)
{
    QVariant retour = QVariant::fromValue(get(_index));

    return retour;
}

// ============================================================
// Réception ESP32
// ============================================================

void ControleurArrosage::traiterMessage(const QString &_message)
{
    QJsonObject trame = protocole.parse(_message.toUtf8());

    if (!trame.isEmpty())
    {
        // Réponses (t="r") et notifications spontanées (t="n")
        // portent les mêmes données : même traitement.
        // Les acks (t="a") ne portent pas d'information d'état :
        // l'état réel arrivera par notification.
        if (protocole.estReponse(trame) || protocole.estNotification(trame))
        {
            if (protocole.getCommande(trame)== ProtocoleArrosageClient::CMD_GET_SYSTEM)
                traiterEtatSysteme(trame);
            else
                traiterDonneesVanne(trame);
        }
        else if (protocole.estErreur(trame))
        {
            traiterErreur(trame);
        }
    }
}

void ControleurArrosage::traiterDonneesVanne(const QJsonObject &_trame)
{
    char commande = protocole.getCommande(_trame);
    int id = protocole.getIdVanne(_trame);
    Vanne *v = getParId(id);

    if (v)
    {
        switch (commande)
        {
            case ProtocoleArrosageClient::CMD_OUVRIR:
                v->setEtat(true);
                v->setSynchronisee(true);
                break;

            case ProtocoleArrosageClient::CMD_FERMER:
                v->setEtat(false);
                v->setSynchronisee(true);
                break;

            case ProtocoleArrosageClient::CMD_GET_ETAT:
                v->setEtat(protocole.getEtat(_trame) == ProtocoleArrosageClient::ETAT_OUVERTE);
                v->setSynchronisee(true);
                break;

            case ProtocoleArrosageClient::CMD_GET_MODE:
            case ProtocoleArrosageClient::CMD_SET_MODE:
                v->setMode(protocoleVersMode(protocole.getMode(_trame)));
                break;

            case ProtocoleArrosageClient::CMD_GET_PROG:
            case ProtocoleArrosageClient::CMD_SET_PROG:
                v->setDebut(protocole.getHeure(_trame));
                v->setDuree(protocole.getDuree(_trame));
                v->setFrequence(protocole.getFrequence(_trame));
                break;

            default:
                break;
        }

        notifyRowChanged(indexParId(id));
    }
}

void ControleurArrosage::traiterEtatSysteme(const QJsonObject &_trame)
{
    const QJsonArray vannes = protocole.getVannes(_trame);

    for (const QJsonValue &valeur : vannes)
    {
        QJsonObject element = valeur.toObject();
        int id = protocole.getIdVanne(element);
        Vanne *v = getParId(id);

        if (v)
        {
            v->setEtat(protocole.getEtat(element)
                       == ProtocoleArrosageClient::ETAT_OUVERTE);
            v->setMode(protocoleVersMode(protocole.getMode(element)));
            v->setDebut(protocole.getHeure(element));
            v->setDuree(protocole.getDuree(element));
            v->setFrequence(protocole.getFrequence(element));
            v->setSynchronisee(true);

            notifyRowChanged(indexParId(id));
        }
    }
}

void ControleurArrosage::traiterErreur(const QJsonObject &_trame)
{
    int code = protocole.getCodeErreur(_trame);
    int id = protocole.getIdVanne(_trame);
    int index = indexParId(id);
    emit erreurRecue(ProtocoleArrosageClient::messageErreur(code));
    if (index >= 0)
        rafraichir(index);
}

void ControleurArrosage::configurerConnexion()
{
    com.reconnecter(adresseIp, port);
}

void ControleurArrosage::onConnexionEtablie()
{
    connecte = true;
    timerReconnexion.stop();
    emit connecteChanged();
    envoyerTrame(protocole.setHeureSysteme(QDateTime::currentDateTime()));
    synchroniserVannes();
}

void ControleurArrosage::onConnexionPerdue()
{
    connecte = false;
    emit connecteChanged();
    if (!derniereUrl.isEmpty())
        timerReconnexion.start();
    for (int i = 0; i < m_vannes.size(); i++)
    {
        m_vannes[i]->setSynchronisee(false);
        notifyRowChanged(i);
    }
}

void ControleurArrosage::onErreurCommunication(const QString &_erreur)
{
    emit erreurRecue(_erreur);
}

void ControleurArrosage::onEnConnexionChanged()
{
    emit enConnexionChanged();
}

// ============================================================
// Configuration
// ============================================================

QString ControleurArrosage::cheminConfig() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/config.ini";
}

void ControleurArrosage::chargerConfiguration()
{
    QSettings settings(cheminConfig(), QSettings::IniFormat);
    adresseIp = settings.value("connexion/ip", "127.0.0.1").toString();
    port      = settings.value("connexion/port", 5000).toInt();
    reconstruireUrl();
}

void ControleurArrosage::sauvegarderConfiguration()
{
    QSettings settings(cheminConfig(), QSettings::IniFormat);
    settings.setValue("connexion/ip", adresseIp);
    settings.setValue("connexion/port",port);
}

void ControleurArrosage::reconstruireUrl()
{
    derniereUrl.setScheme("ws");
    derniereUrl.setHost(adresseIp);
    derniereUrl.setPort(port);
}

// ============================================================
// Accès aux vannes
// ============================================================

Vanne* ControleurArrosage::get(int _index) const
{
    Vanne *retour = nullptr;

    if (_index >= 0 && _index < m_vannes.size())
        retour = m_vannes[_index];

    return retour;
}

Vanne* ControleurArrosage::getParId(int _id) const
{
    Vanne *retour = nullptr;

    for (Vanne *v : m_vannes)
    {
        if (v->getId() == _id)
            retour = v;
    }

    return retour;
}

int ControleurArrosage::indexParId(int _id) const
{
    int retour = -1;

    for (int i = 0; i < m_vannes.size(); i++)
    {
        if (m_vannes[i]->getId() == _id)
            retour = i;
    }

    return retour;
}

void ControleurArrosage::notifyRowChanged(int _index)
{
    QModelIndex topLeft = index(_index, 0);

    emit dataChanged(topLeft, topLeft);
}

// ============================================================
// Émission
// ============================================================

void ControleurArrosage::envoyerTrame(const QByteArray &_trame)
{
    com.envoyerMessage(QString::fromUtf8(_trame));
}


void ControleurArrosage::envoyerMode(Vanne *_v)
{
    QByteArray trame = protocole.setMode(_v->getId(),modeVersProtocole(_v->getMode()));
    envoyerTrame(trame);
}

void ControleurArrosage::synchroniserVannes()
{
    envoyerTrame(protocole.demanderEtatSysteme());
}

void ControleurArrosage::tenterReconnexion()
{
    if (!connecte)
        com.connecterESP32(derniereUrl);
}

// ============================================================
// Conversions enum Vanne <-> protocole
// ============================================================

char ControleurArrosage::modeVersProtocole(Vanne::MODE _mode)
{
    char retour = ProtocoleArrosageClient::MODE_MANUEL;

    switch (_mode)
    {
        case Vanne::Programme:
            retour = ProtocoleArrosageClient::MODE_PROGRAMME;
            break;
        case Vanne::Automatique:
            retour = ProtocoleArrosageClient::MODE_AUTOMATIQUE;
            break;
        default:
            break;
    }

    return retour;
}

Vanne::MODE ControleurArrosage::protocoleVersMode(char _mode)
{
    Vanne::MODE retour = Vanne::Manuel;

    switch (_mode)
    {
        case ProtocoleArrosageClient::MODE_PROGRAMME:
            retour = Vanne::Programme;
            break;
        case ProtocoleArrosageClient::MODE_AUTOMATIQUE:
            retour = Vanne::Automatique;
            break;
        default:
            break;
    }

    return retour;
}

// ============================================================
// Configuration de connexion (accesseurs)
// ============================================================

quint16 ControleurArrosage::getPort() const
{
    return port;
}

void ControleurArrosage::setPort(quint16 _newPort)
{
    if (port != _newPort)
    {
        port = _newPort;
        reconstruireUrl();
        sauvegarderConfiguration();
        emit portChanged();
    }
}

void ControleurArrosage::setAdresseIp(const QString &_newAdresseIp)
{
    if (adresseIp != _newAdresseIp)
    {
        adresseIp = _newAdresseIp;
        reconstruireUrl();
        sauvegarderConfiguration();
        emit adresseIpChanged();
    }
}

QString ControleurArrosage::getAdresseIp() const
{
    return adresseIp;
}
