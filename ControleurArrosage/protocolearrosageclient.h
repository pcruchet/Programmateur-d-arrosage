/**
 * @file    protocolearrosageclient.h
 * @brief   Encodage / décodage du protocole applicatif JSON, côté client Qt.
 *
 * @details Déclare la classe ProtocoleArrosageClient, pendant client de
 *          ProtocoleArrosageServeur (firmware ESP32) : construit les trames
 *          JSON de requête/commande envoyées au boîtier, et extrait les
 *          champs des trames de réponse/notification/erreur reçues.
 *          Structure générale d'une trame : {"v":1,"t":"<type>","c":"<commande>", ...}.
 *          Les types, commandes, modes et états sont représentés par des
 *          caractères (char), à l'image de ProtocoleArrosageServeur.
 */

#ifndef PROTOCOLEARROSAGECLIENT_H
#define PROTOCOLEARROSAGECLIENT_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QString>

/**
 * @class ProtocoleArrosageClient
 * @ingroup cpp_classes
 * @brief Classe C++ : Encodage / décodage des trames JSON du protocole
 *        "Contrôleur d'Arrosage" v1.1 (WebSocket, application Qt/QML <-> ESP32).
 *
 * @details Ne connaît pas le transport (WebSocket) : produit et consomme des
 *          QByteArray/QJsonObject, à charge de ControleurArrosage de les
 *          transmettre via CommunicationESP32. Les méthodes d'extraction
 *          (getType(), getCommande(), ...) renvoient une valeur par défaut
 *          neutre ('\0' ou 0) si le champ est absent de la trame, plutôt que
 *          d'échouer.
 */
class ProtocoleArrosageClient
{
public:
    // --- Types de trames (§3 du protocole) ---
    static const char TYPE_QUERY        = 'q'; ///< Trame de demande (question) émise par le client.
    static const char TYPE_REPONSE      = 'r'; ///< Trame de réponse à une demande.
    static const char TYPE_COMMANDE     = 'c'; ///< Trame de commande (action à exécuter) émise par le client.
    static const char TYPE_ACK          = 'a'; ///< Trame d'accusé de réception d'une commande exécutée avec succès.
    static const char TYPE_ERREUR       = 'e'; ///< Trame d'erreur.
    static const char TYPE_NOTIFICATION = 'n'; ///< Trame de notification spontanée émise par le serveur.

    // --- Commandes (§4 du protocole) ---
    static const char CMD_GET_TIME   = 'T'; ///< Lecture de l'heure système.
    static const char CMD_SET_TIME   = 't'; ///< Écriture de l'heure système.
    static const char CMD_GET_ETAT   = 'E'; ///< Lecture de l'état (ouverte/fermée) d'une vanne.
    static const char CMD_GET_MODE   = 'M'; ///< Lecture du mode d'une vanne.
    static const char CMD_SET_MODE   = 'm'; ///< Écriture du mode d'une vanne.
    static const char CMD_GET_PROG   = 'P'; ///< Lecture de la programmation d'une vanne.
    static const char CMD_SET_PROG   = 'p'; ///< Écriture de la programmation d'une vanne.
    static const char CMD_OUVRIR     = 'O'; ///< Commande d'ouverture manuelle d'une vanne.
    static const char CMD_FERMER     = 'F'; ///< Commande de fermeture manuelle d'une vanne.
    static const char CMD_PING       = 'G'; ///< Commande de test de présence (ping/pong).
    static const char CMD_GET_SYSTEM = 'S'; ///< Lecture de l'état système complet.

    // --- Modes (§5 du protocole) ---
    static const char MODE_MANUEL      = 'M'; ///< Mode Manuel.
    static const char MODE_PROGRAMME   = 'P'; ///< Mode Programme.
    static const char MODE_AUTOMATIQUE = 'A'; ///< Mode Automatique.

    // --- États (§5 du protocole) ---
    static const char ETAT_FERMEE  = 'F'; ///< Code d'état "vanne fermée".
    static const char ETAT_OUVERTE = 'O'; ///< Code d'état "vanne ouverte".

    /// Codes d'erreur retournés par le boîtier dans une trame d'erreur (§8 du protocole).
    enum CodeErreur
    {
        ERREUR_AUCUNE            = 0, ///< Aucune erreur.
        ERREUR_VANNE_INEXISTANTE = 1, ///< L'identifiant de vanne ne correspond à aucune vanne connue du boîtier.
        ERREUR_MODE_INVALIDE     = 2, ///< Le mode demandé n'est pas une valeur MODE_* valide.
        ERREUR_PROG_INVALIDE     = 3, ///< La programmation demandée est invalide (heure, durée ou fréquence).
        ERREUR_RTC_INDISPONIBLE  = 4, ///< Le RTC du boîtier est indisponible.
        ERREUR_SQLITE_INDISPONIBLE = 5 ///< Le stockage persistant du boîtier est indisponible.
    };

    /// @brief Construit le codec pour la version de protocole donnée.
    /// @param _version Version du protocole insérée dans chaque trame émise (champ "v").
    explicit ProtocoleArrosageClient(int _version = 1);

    // ===== Encodage : commandes (t = "c") =====

    /// @brief Encode une commande d'ouverture manuelle. Trame : {"v":1,"t":"c","c":"O","i":_id}
    /// @param _id Identifiant de la vanne à ouvrir.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray ouvrirVanne(int _id);

    /// @brief Encode une commande de fermeture manuelle. Trame : {"v":1,"t":"c","c":"F","i":_id}
    /// @param _id Identifiant de la vanne à fermer.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray fermerVanne(int _id);

    /// @brief Encode une commande de changement de mode. Trame : {"v":1,"t":"c","c":"m","i":_id,"m":_mode}
    /// @param _id Identifiant de la vanne concernée.
    /// @param _mode Nouveau mode (une des constantes MODE_*).
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray setMode(int _id, char _mode);

    /// @brief Encode une commande de programmation. Trame :
    ///        {"v":1,"t":"c","c":"p","i":_id,"h":"<ISO 8601>","d":_dureeMinutes,"f":_frequenceHeures}
    /// @param _id Identifiant de la vanne concernée.
    /// @param _debut Heure de début du prochain cycle d'arrosage.
    /// @param _dureeMinutes Durée d'arrosage, en minutes.
    /// @param _frequenceHeures Fréquence de répétition, en heures.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray setProgrammation(int _id,
                                const QDateTime& _debut,
                                int _dureeMinutes,
                                int _frequenceHeures);

    /// @brief Encode une commande de réglage de l'heure système du boîtier.
    ///        Trame : {"v":1,"t":"c","c":"t","h":"<ISO 8601>"}
    /// @param _dateHeure Nouvelle date/heure à appliquer.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray setHeureSysteme(const QDateTime &_dateHeure);

    /// @brief Surcharge acceptant directement une date/heure déjà au format
    ///        ISO 8601, sans repasser par QDateTime.
    /// @param _dateHeureIso Date/heure au format ISO 8601.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray setHeureSysteme(const QString &_dateHeureIso);

    // ===== Encodage : demandes (t = "q") =====

    /// @brief Encode une demande de lecture de l'heure système. Trame : {"v":1,"t":"q","c":"T"}
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray demanderHeureSysteme();

    /// @brief Encode une demande de lecture de l'état d'une vanne. Trame : {"v":1,"t":"q","c":"E","i":_id}
    /// @param _id Identifiant de la vanne concernée.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray demanderEtatVanne(int _id);

    /// @brief Encode une demande de lecture du mode d'une vanne. Trame : {"v":1,"t":"q","c":"M","i":_id}
    /// @param _id Identifiant de la vanne concernée.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray demanderMode(int _id);

    /// @brief Encode une demande de lecture de la programmation d'une vanne. Trame : {"v":1,"t":"q","c":"P","i":_id}
    /// @param _id Identifiant de la vanne concernée.
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray demanderProgrammation(int _id);

    /// @brief Encode une demande de lecture de l'état système complet
    ///        (toutes les vannes en une seule trame). Trame : {"v":1,"t":"q","c":"S"}
    /// @return Trame JSON compacte prête à être envoyée.
    /// @see getVannes() pour l'extraction du tableau de vannes de la réponse.
    QByteArray demanderEtatSysteme();

    /// @brief Encode une demande de test de présence. Trame : {"v":1,"t":"q","c":"G"}
    /// @return Trame JSON compacte prête à être envoyée.
    QByteArray ping();

    // ===== Décodage =====

    /// @brief Parse un message JSON brut reçu du boîtier.
    /// @param _message Message brut reçu (trame JSON).
    /// @return Objet JSON correspondant, ou un objet vide si _message n'est
    ///         pas un document JSON valide représentant un objet.
    QJsonObject parse(const QByteArray& _message);

    // Nature de la trame reçue
    bool estReponse(const QJsonObject& _trame) const;        ///< @return true si le champ "t" vaut TYPE_REPONSE.
    bool estAck(const QJsonObject& _trame) const;             ///< @return true si le champ "t" vaut TYPE_ACK.
    bool estErreur(const QJsonObject& _trame) const;          ///< @return true si le champ "t" vaut TYPE_ERREUR.
    bool estNotification(const QJsonObject& _trame) const;    ///< @return true si le champ "t" vaut TYPE_NOTIFICATION.

    // Extraction des champs (valeur par défaut si champ absent)
    int       getVersion(const QJsonObject& _trame) const;    ///< @return Champ "v", ou 0 si absent.
    char      getType(const QJsonObject& _trame) const;       ///< @return Champ "t" (une des constantes TYPE_*), ou '\\0' si absent.
    char      getCommande(const QJsonObject& _trame) const;   ///< @return Champ "c" (une des constantes CMD_*), ou '\\0' si absent.
    int       getIdVanne(const QJsonObject& _trame) const;    ///< @return Champ "i", ou -1 si absent.
    char      getMode(const QJsonObject& _trame) const;       ///< @return Champ "m" (une des constantes MODE_*), ou '\\0' si absent.
    char      getEtat(const QJsonObject& _trame) const;       ///< @return Champ "e" (une des constantes ETAT_*), ou '\\0' si absent.
    QDateTime getHeure(const QJsonObject& _trame) const;      ///< @return Champ "h" interprété comme date/heure ISO 8601.
    int       getDuree(const QJsonObject& _trame) const;      ///< @return Champ "d" (durée en minutes), ou 0 si absent.
    int       getFrequence(const QJsonObject& _trame) const;  ///< @return Champ "f" (fréquence en heures), ou 0 si absent.
    int       getCodeErreur(const QJsonObject& _trame) const; ///< @return Champ "x" (une des constantes CodeErreur), ou ERREUR_AUCUNE si absent.

    /// @brief Extrait le tableau des états de vannes d'une réponse à
    ///        demanderEtatSysteme() (champ "s" : tableau d'objets {"i","e","m","h","d","f"}).
    /// @param _trame Trame de réponse à décoder.
    /// @return Tableau JSON des vannes, vide si le champ "s" est absent.
    QJsonArray getVannes(const QJsonObject& _trame) const;

    /// @brief Vérifie qu'un caractère de mode correspond à une valeur MODE_* connue.
    /// @param _mode Caractère à valider.
    /// @return true si _mode est MODE_MANUEL, MODE_PROGRAMME ou MODE_AUTOMATIQUE.
    static bool estModeValide(char _mode);

    /// @brief Traduit un code d'erreur du protocole en message lisible (français).
    /// @param _code Code d'erreur (une des constantes CodeErreur).
    /// @return Message décrivant l'erreur, ou "Erreur inconnue" si _code n'est pas reconnu.
    static QString messageErreur(int _code);

private:
    /// @brief Construit une trame JSON compacte {"v":version,"t":_type,"c":_commande, ...donnees}.
    /// @param _type Type de trame (une des constantes TYPE_*).
    /// @param _commande Commande (une des constantes CMD_*).
    /// @param _donnees Champs additionnels à fusionner dans l'objet JSON (facultatif).
    /// @return Trame JSON compacte (QJsonDocument::Compact).
    QByteArray creerTrame(char _type,
                          char _commande,
                          const QJsonObject& _donnees = QJsonObject());

    /// @brief Extrait un champ JSON d'un caractère (chaîne d'un seul caractère).
    /// @param _trame Trame à interroger.
    /// @param _cle Nom du champ JSON.
    /// @return Premier caractère du champ, ou '\\0' si le champ est absent ou vide.
    static char champCaractere(const QJsonObject& _trame, const QString& _cle);

    int version; ///< Version de protocole insérée dans chaque trame émise (champ "v").
};

#endif // PROTOCOLEARROSAGECLIENT_H
