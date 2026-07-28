/**
 * @file    PageVanne.js
 * @brief   Fonctions utilitaires de la page PageVanne.qml.
 *
 * @details Formatage des libellés du sélecteur fréquence/durée, correction
 *          du format de date saisi, et actions déclenchées par les
 *          interactions utilisateur (activation/suspension de la
 *          programmation, ouverture du sélecteur de date, enregistrement
 *          de la programmation). Chaque fonction reçoit explicitement les
 *          objets QML dont elle a besoin en paramètre : un module .js en
 *          .pragma library n'a pas accès aux ids du document QML qui
 *          l'importe.
 */
.pragma library

/**
 * @brief Formate une fréquence en heures avec accord singulier/pluriel.
 * @param _valeur Fréquence en heures.
 * @return "1 heure" si _valeur vaut 1, "_valeur heures" sinon.
 */
function libelleFrequence(_valeur) {
    return _valeur === 1 ? _valeur + " heure" : _valeur + " heures"
}

/**
 * @brief Formate une durée en minutes.
 * @param _valeur Durée en minutes.
 * @return "_valeur min".
 */
function libelleDuree(_valeur) {
    return _valeur + " min"
}

/**
 * @brief Corrige le séparateur de date/heure saisi par l'utilisateur
 *        (retire le tiret entouré d'espaces).
 * @param _texte Texte au format "dd/MM/yyyy - HH:mm".
 * @return Texte au format "dd/MM/yyyy HH:mm", attendu par
 *         QDateTime::fromString côté C++ (ControleurArrosage::setDebut()).
 */
function corrigerDateDebut(_texte) {
    return _texte.replace(" - ", " ")
}

/**
 * @brief Bascule l'activation de la programmation d'une vanne (suspend si
 *        active, reprend sinon).
 * @param _controleurArrosage Instance de ControleurArrosage sur laquelle agir.
 * @param _vanneIndex Index de la vanne concernée.
 * @param _programmeActif true si la programmation est actuellement active
 *                         (auquel cas elle sera suspendue), false sinon
 *                         (auquel cas elle sera reprise).
 */
function togglerProgramme(_controleurArrosage, _vanneIndex, _programmeActif) {
    if (_programmeActif)
        _controleurArrosage.suspendProgramme(_vanneIndex)
    else
        _controleurArrosage.resumeProgramme(_vanneIndex)
}

/**
 * @brief Ouvre le sélecteur de date/heure (DatePicker.qml) pour choisir
 *        l'heure de début d'arrosage, et reporte la sélection dans le
 *        champ de saisie une fois validée.
 * @param _stackView StackView sur lequel pousser le sélecteur.
 * @param _champDebut Champ de saisie (AppTextField) à mettre à jour avec
 *                     la date/heure sélectionnée.
 * @param _datePickerUrl URL résolue de Component/DatePicker.qml (résolue
 *                        par l'appelant QML via Qt.resolvedUrl()).
 */
function ouvrirSelecteurDebut(_stackView, _champDebut, _datePickerUrl) {
    var page = _stackView.push(_datePickerUrl)
    page.validated.connect(function(dateTime) {
        _champDebut.text = dateTime
        _champDebut.font.bold = true
    })
}

/**
 * @brief Envoie à l'ESP32 la programmation saisie dans le formulaire (durée,
 *        fréquence, début), si les trois champs sont renseignés, puis
 *        dépile le formulaire.
 * @param _controleurArrosage Instance de ControleurArrosage à qui
 *                             transmettre la programmation.
 * @param _vanneIndex Index de la vanne concernée.
 * @param _dureeSelectionnee Durée sélectionnée dans le formulaire (minutes).
 * @param _frequenceSelectionnee Fréquence sélectionnée dans le formulaire (heures).
 * @param _champDebut Champ de saisie contenant la date/heure de début
 *                     (ignoré si vide : aucun envoi n'est effectué).
 * @param _stackView StackView depuis lequel dépiler le formulaire après envoi.
 */
function enregistrerProgrammation(_controleurArrosage, _vanneIndex, _dureeSelectionnee, _frequenceSelectionnee, _champDebut, _stackView) {
    if (_dureeSelectionnee > 0 && _frequenceSelectionnee > 0 && _champDebut.text !== "") {
        _controleurArrosage.setDuree(_vanneIndex, _dureeSelectionnee)
        _controleurArrosage.setFrequence(_vanneIndex, _frequenceSelectionnee)

        var dateTimeCorrige = corrigerDateDebut(_champDebut.text)
        _controleurArrosage.setDebut(_vanneIndex, dateTimeCorrige)

        _controleurArrosage.envoyerProgrammation(_vanneIndex)

        _stackView.pop()
    }
}
