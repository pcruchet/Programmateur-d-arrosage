/**
 * @file    ProtocoleArrosageServeur.cpp
 * @brief   Implémentation de l'encodage/décodage des trames JSON du
 *          protocole applicatif. Documentation complète de chaque méthode
 *          dans ProtocoleArrosageServeur.h ; les commentaires ci-dessous
 *          précisent les champs JSON effectivement produits ou lus par
 *          chaque implémentation.
 */

#include "ProtocoleArrosageServeur.h"

/// Constructeur par défaut, sans état à initialiser.
ProtocoleArrosageServeur::ProtocoleArrosageServeur()
{
}

// ============================================================
// Décodage
// ============================================================

/**
 * @brief Décode une trame JSON en RequeteArrosage.
 * @details Échoue proprement (valide=false) en cas d'erreur de
 *          désérialisation, de champs obligatoires "v"/"t"/"c" absents ou
 *          mal typés, ou de version différente de VERSION. Les champs
 *          optionnels "i", "m", "h", "d", "f" ne sont recopiés que s'ils
 *          sont présents et correctement typés dans le JSON reçu.
 */
RequeteArrosage ProtocoleArrosageServeur::decoder(const String &_message) const
{
    RequeteArrosage requete;
    requete.idVanne   = -1;
    requete.duree     = 0;
    requete.frequence = 0;
    requete.valide    = false;

    JsonDocument document;
    DeserializationError erreur = deserializeJson(document, _message);

    if (erreur == DeserializationError::Ok)
    {
        if (document["v"].is<int>() && document["t"].is<const char*>() && document["c"].is<const char*>())
        {
            if (document["v"].as<int>() == VERSION)
            {
                requete.type     = document["t"].as<String>();
                requete.commande = document["c"].as<String>();

                if (document["i"].is<int>())
                    requete.idVanne = document["i"].as<int>();

                if (document["m"].is<const char*>())
                    requete.mode = document["m"].as<String>();

                if (document["h"].is<const char*>())
                    requete.heure = document["h"].as<String>();

                if (document["d"].is<int>())
                    requete.duree = document["d"].as<int>();

                if (document["f"].is<int>())
                    requete.frequence = document["f"].as<int>();

                requete.valide = true;
            }
        }
    }

    return requete;
}

// ============================================================
// Réponses
// ============================================================

/// Sérialise {"v","t":"r","c":CMD_GET_TIME,"h":_heure}.
String ProtocoleArrosageServeur::creerReponseHeure(const String &_heure) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_REPONSE);
    document["c"] = String(CMD_GET_TIME);
    document["h"] = _heure;

    serializeJson(document, trame);

    return trame;
}

/// Sérialise {"v","t":"r","c":CMD_GET_ETAT,"i":_idVanne,"e":_etat}.
String ProtocoleArrosageServeur::creerReponseEtat(int _idVanne, char _etat) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_REPONSE);
    document["c"] = String(CMD_GET_ETAT);
    document["i"] = _idVanne;
    document["e"] = String(_etat);

    serializeJson(document, trame);

    return trame;
}

/// Sérialise {"v","t":"r","c":CMD_GET_MODE,"i":_idVanne,"m":_mode}.
String ProtocoleArrosageServeur::creerReponseMode(int _idVanne, char _mode) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_REPONSE);
    document["c"] = String(CMD_GET_MODE);
    document["i"] = _idVanne;
    document["m"] = String(_mode);

    serializeJson(document, trame);

    return trame;
}

/// Sérialise {"v","t":"r","c":CMD_GET_PROG,"i","h","d","f"} à partir des 4 paramètres.
String ProtocoleArrosageServeur::creerReponseProgrammation(int _idVanne,
                                                           const String &_heure,
                                                           int _duree,
                                                           int _frequence) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_REPONSE);
    document["c"] = String(CMD_GET_PROG);
    document["i"] = _idVanne;
    document["h"] = _heure;
    document["d"] = _duree;
    document["f"] = _frequence;

    serializeJson(document, trame);

    return trame;
}

/// Délègue à creerTrame(TYPE_REPONSE, CMD_PING) : trame minimale sans donnée supplémentaire.
String ProtocoleArrosageServeur::creerReponsePing() const
{
    String trame;

    trame = creerTrame(TYPE_REPONSE, CMD_PING);

    return trame;
}

/// Sérialise {"v","t":"r","c":CMD_GET_SYSTEM,"h":_heureSysteme,"s":_vannes}, le tableau _vannes étant inséré tel quel.
String ProtocoleArrosageServeur::creerReponseSysteme(const String &_heureSysteme,
                                                     const JsonArray &_vannes) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_REPONSE);
    document["c"] = String(CMD_GET_SYSTEM);
    document["h"] = _heureSysteme;
    document["s"] = _vannes;

    serializeJson(document, trame);

    return trame;
}

// ============================================================
// Acks
// ============================================================

/// Délègue à creerTrame(TYPE_ACK, _commande) : accusé sans donnée supplémentaire.
String ProtocoleArrosageServeur::creerAck(char _commande) const
{
    String trame;

    trame = creerTrame(TYPE_ACK, _commande);

    return trame;
}

/// Sérialise {"v","t":"a","c":_commande,"i":_idVanne}.
String ProtocoleArrosageServeur::creerAckVanne(char _commande, int _idVanne) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_ACK);
    document["c"] = String(_commande);
    document["i"] = _idVanne;

    serializeJson(document, trame);

    return trame;
}

// ============================================================
// Erreurs
// ============================================================

/// Sérialise {"v","t":"e","c":_commande,"x":_codeErreur}.
String ProtocoleArrosageServeur::creerErreur(char _commande, int _codeErreur) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_ERREUR);
    document["c"] = String(_commande);
    document["x"] = _codeErreur;

    serializeJson(document, trame);

    return trame;
}

// ============================================================
// Notifications
// ============================================================

/// Sérialise {"v","t":"n","c":_commande,"i":_idVanne} ; _commande vaut typiquement CMD_OUVRIR ou CMD_FERMER.
String ProtocoleArrosageServeur::creerNotifEtatVanne(char _commande, int _idVanne) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_NOTIF);
    document["c"] = String(_commande);
    document["i"] = _idVanne;

    serializeJson(document, trame);

    return trame;
}

/// Sérialise {"v","t":"n","c":CMD_GET_MODE,"i":_idVanne,"m":_mode}.
String ProtocoleArrosageServeur::creerNotifMode(int _idVanne, char _mode) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_NOTIF);
    document["c"] = String(CMD_GET_MODE);
    document["i"] = _idVanne;
    document["m"] = String(_mode);

    serializeJson(document, trame);

    return trame;
}

/// Sérialise {"v","t":"n","c":CMD_GET_PROG,"i","h","d","f"} à partir des 4 paramètres.
String ProtocoleArrosageServeur::creerNotifProgrammation(int _idVanne,
                                                         const String &_heure,
                                                         int _duree,
                                                         int _frequence) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(TYPE_NOTIF);
    document["c"] = String(CMD_GET_PROG);
    document["i"] = _idVanne;
    document["h"] = _heure;
    document["d"] = _duree;
    document["f"] = _frequence;

    serializeJson(document, trame);

    return trame;
}

/// Délègue à creerTrame(TYPE_NOTIF, CMD_VEILLE) : notification sans donnée supplémentaire.
String ProtocoleArrosageServeur::creerNotifVeille() const
{
    String trame;

    trame = creerTrame(TYPE_NOTIF, CMD_VEILLE);

    return trame;
}

// ============================================================
// Privé
// ============================================================

/**
 * @brief Construit une trame JSON minimale {"v":VERSION,"t":_type,"c":_commande},
 *        factorisée pour les trames ne portant aucune donnée
 *        supplémentaire (ping, ack simple, notification de veille).
 */
String ProtocoleArrosageServeur::creerTrame(char _type, char _commande) const
{
    String trame;

    JsonDocument document;
    document["v"] = VERSION;
    document["t"] = String(_type);
    document["c"] = String(_commande);

    serializeJson(document, trame);

    return trame;
}