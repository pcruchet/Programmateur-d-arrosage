/**
 * @file    protocolearrosageclient.cpp
 * @brief   Implémentation de ProtocoleArrosageClient.
 *
 * @details Construction des trames de commande/demande (une méthode par
 *          commande du protocole), décodage des trames reçues (nature de la
 *          trame, extraction de champs avec valeur par défaut si absents),
 *          et utilitaires (validation de mode, message d'erreur lisible).
 */

#include "protocolearrosageclient.h"

#include <QJsonDocument>

ProtocoleArrosageClient::ProtocoleArrosageClient(int _version)
    : version(_version)
{
}

// ============================================================
// Encodage : commandes (t = "c")
// ============================================================

QByteArray ProtocoleArrosageClient::ouvrirVanne(int _id)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;

    trame = creerTrame(TYPE_COMMANDE, CMD_OUVRIR, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::fermerVanne(int _id)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;

    trame = creerTrame(TYPE_COMMANDE, CMD_FERMER, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::setMode(int _id, char _mode)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;
    donnees["m"] = QString(1, QChar(_mode));

    trame = creerTrame(TYPE_COMMANDE, CMD_SET_MODE, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::setProgrammation(int _id,
                                                     const QDateTime& _debut,
                                                     int _dureeMinutes,
                                                     int _frequenceHeures)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;
    donnees["h"] = _debut.toString(Qt::ISODate);
    donnees["d"] = _dureeMinutes;
    donnees["f"] = _frequenceHeures;

    trame = creerTrame(TYPE_COMMANDE, CMD_SET_PROG, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::setHeureSysteme(const QDateTime& _dateHeure)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["h"] = _dateHeure.toString(Qt::ISODate);

    trame = creerTrame(TYPE_COMMANDE, CMD_SET_TIME, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::setHeureSysteme(const QString &_dateHeureIso)
{
    QJsonObject donnees;
    donnees["h"] = _dateHeureIso;

    QByteArray trame = creerTrame(TYPE_COMMANDE, CMD_SET_TIME, donnees);

    return trame;
}

// ============================================================
// Encodage : demandes (t = "q")
// ============================================================

QByteArray ProtocoleArrosageClient::demanderHeureSysteme()
{
    QByteArray trame;

    trame = creerTrame(TYPE_QUERY, CMD_GET_TIME);

    return trame;
}

QByteArray ProtocoleArrosageClient::demanderEtatVanne(int _id)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;

    trame = creerTrame(TYPE_QUERY, CMD_GET_ETAT, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::demanderMode(int _id)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;

    trame = creerTrame(TYPE_QUERY, CMD_GET_MODE, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::demanderProgrammation(int _id)
{
    QByteArray trame;

    QJsonObject donnees;
    donnees["i"] = _id;

    trame = creerTrame(TYPE_QUERY, CMD_GET_PROG, donnees);

    return trame;
}

QByteArray ProtocoleArrosageClient::ping()
{
    QByteArray trame;

    trame = creerTrame(TYPE_QUERY, CMD_PING);

    return trame;
}

QByteArray ProtocoleArrosageClient::demanderEtatSysteme()
{
    QByteArray trame;

    trame = creerTrame(TYPE_QUERY, CMD_GET_SYSTEM);

    return trame;
}

// ============================================================
// Décodage
// ============================================================

QJsonObject ProtocoleArrosageClient::parse(const QByteArray& _message)
{
    QJsonObject objet;

    QJsonDocument document = QJsonDocument::fromJson(_message);

    if(document.isObject())
        objet = document.object();

    return objet;
}

bool ProtocoleArrosageClient::estReponse(const QJsonObject& _trame) const
{
    bool resultat = (getType(_trame) == TYPE_REPONSE);

    return resultat;
}

bool ProtocoleArrosageClient::estAck(const QJsonObject& _trame) const
{
    bool resultat = (getType(_trame) == TYPE_ACK);

    return resultat;
}

bool ProtocoleArrosageClient::estErreur(const QJsonObject& _trame) const
{
    bool resultat = (getType(_trame) == TYPE_ERREUR);

    return resultat;
}

bool ProtocoleArrosageClient::estNotification(const QJsonObject& _trame) const
{
    bool resultat = (getType(_trame) == TYPE_NOTIFICATION);

    return resultat;
}

int ProtocoleArrosageClient::getVersion(const QJsonObject& _trame) const
{
    int valeur = _trame.value("v").toInt(0);

    return valeur;
}

char ProtocoleArrosageClient::getType(const QJsonObject& _trame) const
{
    return champCaractere(_trame, "t");
}

char ProtocoleArrosageClient::getCommande(const QJsonObject& _trame) const
{
    return champCaractere(_trame, "c");
}

int ProtocoleArrosageClient::getIdVanne(const QJsonObject& _trame) const
{
    int valeur = _trame.value("i").toInt(-1);

    return valeur;
}

char ProtocoleArrosageClient::getMode(const QJsonObject& _trame) const
{
    return champCaractere(_trame, "m");
}

char ProtocoleArrosageClient::getEtat(const QJsonObject& _trame) const
{
    return champCaractere(_trame, "e");
}

QDateTime ProtocoleArrosageClient::getHeure(const QJsonObject& _trame) const
{
    QDateTime valeur = QDateTime::fromString(_trame.value("h").toString(),
                                             Qt::ISODate);

    return valeur;
}

int ProtocoleArrosageClient::getDuree(const QJsonObject& _trame) const
{
    int valeur = _trame.value("d").toInt(0);

    return valeur;
}

int ProtocoleArrosageClient::getFrequence(const QJsonObject& _trame) const
{
    int valeur = _trame.value("f").toInt(0);

    return valeur;
}

int ProtocoleArrosageClient::getCodeErreur(const QJsonObject& _trame) const
{
    int valeur = _trame.value("x").toInt(ERREUR_AUCUNE);

    return valeur;
}

QJsonArray ProtocoleArrosageClient::getVannes(const QJsonObject& _trame) const
{
    QJsonArray valeur = _trame.value("s").toArray();

    return valeur;
}

// ============================================================
// Utilitaires
// ============================================================

bool ProtocoleArrosageClient::estModeValide(char _mode)
{
    bool valide = false;

    switch (_mode)
    {
        case MODE_MANUEL:
        case MODE_PROGRAMME:
        case MODE_AUTOMATIQUE:
            valide = true;
            break;
        default:
            break;
    }

    return valide;
}

QString ProtocoleArrosageClient::messageErreur(int _code)
{
    QString message;

    switch(_code)
    {
        case ERREUR_AUCUNE:
            message = "Aucune erreur";
            break;
        case ERREUR_VANNE_INEXISTANTE:
            message = "Vanne inexistante";
            break;
        case ERREUR_MODE_INVALIDE:
            message = "Mode invalide";
            break;
        case ERREUR_PROG_INVALIDE:
            message = "Programmation invalide";
            break;
        case ERREUR_RTC_INDISPONIBLE:
            message = "RTC indisponible";
            break;
        case ERREUR_SQLITE_INDISPONIBLE:
            message = "Base SQLite indisponible";
            break;
        default:
            message = "Erreur inconnue";
            break;
    }

    return message;
}

// ============================================================
// Privé
// ============================================================

QByteArray ProtocoleArrosageClient::creerTrame(char _type,
                                               char _commande,
                                               const QJsonObject& _donnees)
{
    QByteArray trame;

    QJsonObject objet;
    objet["v"] = version;
    objet["t"] = QString(1, QChar(_type));
    objet["c"] = QString(1, QChar(_commande));

    for(auto it = _donnees.begin(); it != _donnees.end(); ++it)
        objet.insert(it.key(), it.value());

    trame = QJsonDocument(objet).toJson(QJsonDocument::Compact);

    return trame;
}

char ProtocoleArrosageClient::champCaractere(const QJsonObject& _trame, const QString& _cle)
{
    QString valeur = _trame.value(_cle).toString();
    char retour = valeur.isEmpty() ? '\0' : valeur.at(0).toLatin1();

    return retour;
}
