/**
 * @file    vanne.h
 * @brief   Image logicielle d'une électrovanne, côté application Qt/QML.
 *
 * @details Déclare la classe Vanne, objet de données pur exposé à QML
 *          (propriétés Q_PROPERTY, énumérations Q_ENUM) représentant l'état
 *          connu d'une électrovanne physique pilotée par l'ESP32. Vanne ne
 *          communique jamais directement avec l'ESP32 : c'est
 *          ControleurArrosage qui met à jour ses propriétés à partir des
 *          réponses/notifications reçues via ProtocoleArrosageClient.
 */

#ifndef VANNE_H
#define VANNE_H

#include <QObject>
#include <QDateTime>
#include <QQmlEngine>

/**
 * @class Vanne
 * @ingroup cpp_classes
 * @brief Classe C++ : Image logicielle d'une électrovanne réelle.
 *
 * @details Objet de données pur : ne connaît ni le protocole ni la
 *          communication. Tant que synchronisee == false, les valeurs sont
 *          des valeurs par défaut qui ne reflètent PAS l'état réel de
 *          l'électrovanne. Les enums MODE et PROGRAM_STATE sont enregistrés
 *          dans le module QML "ControleurArrosage" (QML_NAMED_ELEMENT) sous
 *          le nom "Vanne", et utilisables depuis QML (ex. Vanne.Actif) sans
 *          import supplémentaire.
 */
class Vanne : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Vanne)
    QML_UNCREATABLE("Enum only")

    Q_PROPERTY(bool etat READ getEtat WRITE setEtat NOTIFY etatChanged)
    Q_PROPERTY(MODE mode READ getMode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(PROGRAM_STATE programmeState READ getProgrammeState WRITE setProgrammeState NOTIFY programmeStateChanged)
    Q_PROPERTY(QString nom READ getNom WRITE setNom NOTIFY nomChanged)
    Q_PROPERTY(QDateTime debut READ getDebut WRITE setDebut NOTIFY debutChanged)
    Q_PROPERTY(int duree READ getDuree WRITE setDuree NOTIFY dureeChanged)
    Q_PROPERTY(int frequence READ getFrequence WRITE setFrequence NOTIFY frequenceChanged)
    Q_PROPERTY(bool synchronisee READ estSynchronisee WRITE setSynchronisee NOTIFY synchroniseeChanged)

public:

    /// Mode de fonctionnement d'une vanne.
    enum MODE {
        Manuel,      ///< La vanne n'obéit qu'aux commandes explicites d'ouverture/fermeture ; la programmation est suspendue.
        Programme,   ///< La vanne s'ouvre/se ferme automatiquement selon l'heure, la durée et la fréquence programmées.
        Automatique  ///< Réservé pour une logique d'arrosage automatisée au-delà de la simple programmation horaire.
    };
    Q_ENUM(MODE)

    /// État d'activation de la programmation d'une vanne.
    enum PROGRAM_STATE {
        Actif,    ///< La programmation est active : la vanne s'ouvre/se ferme automatiquement.
        Suspendu  ///< La programmation est suspendue (ex. après un passage manuel en mode Manuel).
    };
    Q_ENUM(PROGRAM_STATE)

    /**
     * @brief Construit une vanne avec des valeurs par défaut non
     *        synchronisées (synchronisee == false).
     * @param _id Identifiant de la vanne (1 à 4), utilisé pour la retrouver
     *            dans les trames du protocole.
     * @param _parent Objet parent Qt (gestion de la durée de vie).
     */
    explicit Vanne(const int _id, QObject *_parent = nullptr);

    // ================= GETTERS =================
    bool getEtat() const;                    ///< @return true si la vanne est (censée être) ouverte.
    MODE getMode() const;                    ///< @return Mode courant de la vanne.
    PROGRAM_STATE getProgrammeState() const; ///< @return État d'activation de la programmation.

    QString getNom() const;      ///< @return Nom affiché de la vanne (ex. "Vanne 1").
    QDateTime getDebut() const;  ///< @return Heure de début programmée du prochain cycle d'arrosage.
    int getDuree() const;        ///< @return Durée d'arrosage programmée, en minutes.
    int getFrequence() const;    ///< @return Fréquence de répétition programmée, en heures.

    bool estSynchronisee() const; ///< @return true si l'état reflète une réponse/notification confirmée par l'ESP32.
    int getId() const;            ///< @return Identifiant de la vanne (1 à 4).

    // ================= SETTERS =================

    /// @brief Modifie l'état ouvert/fermé et émet etatChanged() si la valeur change.
    /// @param _etat Nouvel état (true = ouverte).
    void setEtat(bool _etat);

    /// @brief Modifie le mode et émet modeChanged() si la valeur change.
    /// @param _mode Nouveau mode.
    void setMode(MODE _mode);

    /// @brief Modifie l'état de programmation et émet programmeStateChanged() si la valeur change.
    /// @param _state Nouvel état d'activation de la programmation.
    void setProgrammeState(PROGRAM_STATE _state);

    /// @brief Modifie le nom affiché et émet nomChanged() si la valeur change.
    /// @param _nom Nouveau nom.
    void setNom(const QString &_nom);

    /// @brief Modifie l'heure de début programmée et émet debutChanged() si la valeur change.
    /// @param _debut Nouvelle heure de début.
    void setDebut(const QDateTime &_debut);

    /// @brief Modifie la durée programmée et émet dureeChanged() si la valeur change.
    /// @param _duree Nouvelle durée, en minutes.
    void setDuree(int _duree);

    /// @brief Modifie la fréquence programmée et émet frequenceChanged() si la valeur change.
    /// @param _frequence Nouvelle fréquence, en heures.
    void setFrequence(int _frequence);

    /// @brief Modifie l'indicateur de synchronisation et émet synchroniseeChanged() si la valeur change.
    /// @param _synchronisee true si l'état reflète désormais une confirmation de l'ESP32.
    void setSynchronisee(bool _synchronisee);

    /// @brief Modifie l'identifiant de la vanne (sans notification dédiée).
    /// @param _newId Nouvel identifiant.
    void setId(int _newId);

    // ================= API MÉTIER =================

    /// @brief Bascule l'état ouvert/fermé (raccourci pour setEtat(!getEtat())).
    Q_INVOKABLE void toggleEtat();

    /// @brief Passe en mode Programme avec programmation active (raccourci
    ///        pour setMode(Programme) + setProgrammeState(Actif)).
    Q_INVOKABLE void activerProgramme();

    /// @brief Suspend la programmation sans changer le mode (raccourci pour
    ///        setProgrammeState(Suspendu)).
    Q_INVOKABLE void suspendreProgramme();

    /// @brief Réactive la programmation en mode Programme (raccourci pour
    ///        setMode(Programme) + setProgrammeState(Actif)).
    Q_INVOKABLE void reprendreProgramme();

    /// @brief Passe en mode Manuel (raccourci pour setMode(Manuel)).
    Q_INVOKABLE void passerEnManuel();

signals:

    void etatChanged();           ///< Émis quand l'état ouvert/fermé change.
    void modeChanged();           ///< Émis quand le mode change.
    void programmeStateChanged(); ///< Émis quand l'état d'activation de la programmation change.
    void nomChanged();            ///< Émis quand le nom change.
    void debutChanged();          ///< Émis quand l'heure de début programmée change.
    void dureeChanged();          ///< Émis quand la durée programmée change.
    void frequenceChanged();      ///< Émis quand la fréquence programmée change.
    void synchroniseeChanged();   ///< Émis quand l'indicateur de synchronisation change.

private:

    bool etat;                    ///< État ouvert (true) / fermé (false) connu de la vanne.
    MODE mode;                    ///< Mode courant de la vanne.
    PROGRAM_STATE programmeState; ///< État d'activation de la programmation.

    QString nom; ///< Nom affiché de la vanne.

    QDateTime debut; ///< Heure de début programmée du prochain cycle d'arrosage.
    int duree;       ///< Durée d'arrosage programmée, en minutes.
    int frequence;   ///< Fréquence de répétition programmée, en heures.

    bool synchronisee; ///< true si l'état ci-dessus reflète une confirmation de l'ESP32 (et non une valeur par défaut).
    int id;               ///< Identifiant de la vanne (1 à 4).
};

#endif // VANNE_H
