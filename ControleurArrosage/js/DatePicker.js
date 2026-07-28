/**
 * @file    DatePicker.js
 * @brief   Fonctions utilitaires du composant Component/DatePicker.qml.
 *
 * @details Formatage de la date/heure sélectionnée et actions déclenchées
 *          par les interactions utilisateur (changement de mois,
 *          sélection d'un jour, validation). Chaque fonction reçoit
 *          explicitement les objets QML dont elle a besoin (root, stackView)
 *          en paramètre : un module .js en .pragma library n'a pas accès
 *          aux ids du document QML qui l'importe.
 */
.pragma library

/**
 * @brief Complète un nombre à un chiffre par un zéro de tête.
 * @param _valeur Nombre à formater.
 * @return _valeur sous forme de chaîne à deux chiffres (ex. 5 -> "05"),
 *         ou _valeur tel quel si déjà à deux chiffres ou plus.
 */
function pad2(_valeur) {
    return _valeur < 10 ? "0" + _valeur : _valeur
}

/**
 * @brief Formate une date/heure au format d'affichage de l'application.
 * @param _jour Jour du mois (1-31).
 * @param _mois Mois (1-12, déjà incrémenté par l'appelant si besoin).
 * @param _annee Année.
 * @param _heure Heure (0-23).
 * @param _minute Minute (0-59).
 * @return Date/heure formatée "dd/MM/yyyy - HH:mm".
 */
function formaterDateHeure(_jour, _mois, _annee, _heure, _minute) {
    return pad2(_jour) + "/" + pad2(_mois) + "/" + _annee
            + " - " + pad2(_heure) + ":" + pad2(_minute)
}

/**
 * @brief Change le mois sélectionné, avec gestion du changement d'année en
 *        cas de dépassement des bornes (décembre -> janvier et inversement).
 * @param _root Instance de DatePicker.qml dont selectedMonth/selectedYear
 *              doivent être mis à jour.
 * @param _delta Décalage à appliquer au mois (-1 ou +1).
 */
function changerMois(_root, _delta) {
    var nouveauMois = _root.selectedMonth + _delta

    if (nouveauMois < 0) {
        _root.selectedMonth = 11
        _root.selectedYear--
    } else if (nouveauMois > 11) {
        _root.selectedMonth = 0
        _root.selectedYear++
    } else {
        _root.selectedMonth = nouveauMois
    }
}

/**
 * @brief Sélectionne un jour du calendrier.
 * @param _root Instance de DatePicker.qml dont selectedDay/selectedMonth
 *              doivent être mis à jour.
 * @param _jour Jour sélectionné (1-31).
 * @param _mois Mois du jour sélectionné (0-11), utile pour les jours du
 *              mois précédent/suivant affichés en grisé dans le calendrier.
 */
function selectionnerJour(_root, _jour, _mois) {
    _root.selectedDay = _jour
    _root.selectedMonth = _mois
}

/**
 * @brief Valide la sélection courante : formate la date/heure, émet le
 *        signal validated() de DatePicker.qml, puis dépile la page.
 * @param _root Instance de DatePicker.qml dont la sélection (selectedDay,
 *              selectedMonth, selectedYear, selectedHour, selectedMinute)
 *              est lue, et dont selectedDateTime est mis à jour.
 * @param _stackView StackView depuis lequel dépiler la page.
 */
function validerSelection(_root, _stackView) {
    _root.selectedDateTime = formaterDateHeure(
                _root.selectedDay, _root.selectedMonth + 1, _root.selectedYear,
                _root.selectedHour, _root.selectedMinute)

    _root.validated(_root.selectedDateTime)

    console.log(_root.selectedDateTime)

    _stackView.pop()
}
