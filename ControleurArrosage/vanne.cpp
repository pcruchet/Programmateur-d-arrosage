/**
 * @file    vanne.cpp
 * @brief   Implémentation de la classe Vanne.
 *
 * @details Constructeur (valeurs par défaut non synchronisées), accesseurs,
 *          mutateurs (avec notification Qt uniquement sur changement réel de
 *          valeur), et raccourcis d'API métier invocables depuis QML.
 */

#include "vanne.h"

Vanne::Vanne(const int _id, QObject *_parent)
    : QObject(_parent)
    , m_etat(false)                          // Valeurs PAR DÉFAUT :
    , m_mode(Manuel)                         // elles ne reflètent pas
    , m_programmeState(Suspendu)             // l'électrovanne réelle
    , m_debut(QDateTime::currentDateTime())  // tant que m_synchronisee
    , m_duree(60)                            // est faux
    , m_frequence(24)
    , m_synchronisee(false)
    , id(_id)
{
}

// ================= GETTERS =================

bool Vanne::getEtat() const
{
    return m_etat;
}

Vanne::MODE Vanne::getMode() const
{
    return m_mode;
}

Vanne::PROGRAM_STATE Vanne::getProgrammeState() const
{
    return m_programmeState;
}

QString Vanne::getNom() const
{
    return m_nom;
}

QDateTime Vanne::getDebut() const
{
    return m_debut;
}

int Vanne::getDuree() const
{
    return m_duree;
}

int Vanne::getFrequence() const
{
    return m_frequence;
}

bool Vanne::estSynchronisee() const
{
    return m_synchronisee;
}

int Vanne::getId() const
{
    return id;
}

// ================= SETTERS =================

void Vanne::setEtat(bool _etat)
{
    if (m_etat != _etat)
    {
        m_etat = _etat;
        emit etatChanged();
    }
}

void Vanne::setMode(MODE _mode)
{
    if (m_mode != _mode)
    {
        m_mode = _mode;
        emit modeChanged();
    }
}

void Vanne::setProgrammeState(PROGRAM_STATE _state)
{
    if (m_programmeState != _state)
    {
        m_programmeState = _state;
        emit programmeStateChanged();
    }
}

void Vanne::setNom(const QString &_nom)
{
    if (m_nom != _nom)
    {
        m_nom = _nom;
        emit nomChanged();
    }
}

void Vanne::setDebut(const QDateTime &_debut)
{
    if (m_debut != _debut)
    {
        m_debut = _debut;
        emit debutChanged();
    }
}

void Vanne::setDuree(int _duree)
{
    if (m_duree != _duree)
    {
        m_duree = _duree;
        emit dureeChanged();
    }
}

void Vanne::setFrequence(int _frequence)
{
    if (m_frequence != _frequence)
    {
        m_frequence = _frequence;
        emit frequenceChanged();
    }
}

void Vanne::setSynchronisee(bool _synchronisee)
{
    if (m_synchronisee != _synchronisee)
    {
        m_synchronisee = _synchronisee;
        emit synchroniseeChanged();
    }
}

void Vanne::setId(int _newId)
{
    if (id != _newId)
        id = _newId;
}

// ================= API MÉTIER =================

void Vanne::toggleEtat()
{
    setEtat(!m_etat);
}

void Vanne::activerProgramme()
{
    setMode(Programme);
    setProgrammeState(Actif);
}

void Vanne::suspendreProgramme()
{
    setProgrammeState(Suspendu);
}

void Vanne::reprendreProgramme()
{
    setMode(Programme);
    setProgrammeState(Actif);
}

void Vanne::passerEnManuel()
{
    setMode(Manuel);
}
