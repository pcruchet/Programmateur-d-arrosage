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
    , etat(false)                          // Valeurs PAR DÉFAUT :
    , mode(Manuel)                         // elles ne reflètent pas
    , programmeState(Suspendu)             // l'électrovanne réelle
    , debut(QDateTime::currentDateTime())  // tant que synchronisee
    , duree(60)                            // est faux
    , frequence(24)
    , synchronisee(false)
    , id(_id)
{
}

// ================= GETTERS =================

bool Vanne::getEtat() const
{
    return etat;
}

Vanne::MODE Vanne::getMode() const
{
    return mode;
}

Vanne::PROGRAM_STATE Vanne::getProgrammeState() const
{
    return programmeState;
}

QString Vanne::getNom() const
{
    return nom;
}

QDateTime Vanne::getDebut() const
{
    return debut;
}

int Vanne::getDuree() const
{
    return duree;
}

int Vanne::getFrequence() const
{
    return frequence;
}

bool Vanne::estSynchronisee() const
{
    return synchronisee;
}

int Vanne::getId() const
{
    return id;
}

// ================= SETTERS =================

void Vanne::setEtat(bool _etat)
{
    if (etat != _etat)
    {
        etat = _etat;
        emit etatChanged();
    }
}

void Vanne::setMode(MODE _mode)
{
    if (mode != _mode)
    {
        mode = _mode;
        emit modeChanged();
    }
}

void Vanne::setProgrammeState(PROGRAM_STATE _state)
{
    if (programmeState != _state)
    {
        programmeState = _state;
        emit programmeStateChanged();
    }
}

void Vanne::setNom(const QString &_nom)
{
    if (nom != _nom)
    {
        nom = _nom;
        emit nomChanged();
    }
}

void Vanne::setDebut(const QDateTime &_debut)
{
    if (debut != _debut)
    {
        debut = _debut;
        emit debutChanged();
    }
}

void Vanne::setDuree(int _duree)
{
    if (duree != _duree)
    {
        duree = _duree;
        emit dureeChanged();
    }
}

void Vanne::setFrequence(int _frequence)
{
    if (frequence != _frequence)
    {
        frequence = _frequence;
        emit frequenceChanged();
    }
}

void Vanne::setSynchronisee(bool _synchronisee)
{
    if (synchronisee != _synchronisee)
    {
        synchronisee = _synchronisee;
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
    setEtat(!etat);
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
