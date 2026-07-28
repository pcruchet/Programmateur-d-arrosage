/**
 * @file    PageConfiguration.js
 * @brief   Fonctions utilitaires de la page PageConfiguration.qml.
 *
 * @details Conversion de la date/heure saisie vers le format ISO 8601
 *          attendu par le protocole, formatage de l'heure système affichée,
 *          et ouverture du sélecteur de date/heure. Chaque fonction reçoit
 *          explicitement les objets QML dont elle a besoin en paramètre :
 *          un module .js en .pragma library n'a pas accès aux ids du
 *          document QML qui l'importe.
 */
.pragma library

/**
 * @brief Convertit une date/heure au format d'affichage de l'application
 *        vers le format ISO 8601 attendu par le protocole applicatif.
 * @param _dateTime Date/heure au format "dd/MM/yyyy - HH:mm".
 * @return Date/heure au format ISO 8601 ("yyyy-MM-ddTHH:mm:00").
 */
function construireDateHeureIso(_dateTime) {
    var partiesDateHeure = _dateTime.split(" - ")
    var partiesDate = partiesDateHeure[0].split("/")
    var partiesHeure = partiesDateHeure[1].split(":")

    var jour   = partiesDate[0]
    var mois   = partiesDate[1]
    var annee  = partiesDate[2]
    var heure  = partiesHeure[0]
    var minute = partiesHeure[1]

    return annee + "-" + mois + "-" + jour + "T" + heure + ":" + minute + ":00"
}

/**
 * @brief Formate la date/heure locale courante pour l'affichage.
 * @return Date et heure actuelles formatées "dd/MM/yyyy  HH:mm".
 */
function dateHeureActuelleTexte() {
    var d = new Date()
    return d.toLocaleDateString(Qt.locale("fr_FR"), "dd/MM/yyyy")
            + "  "
            + d.toLocaleTimeString(Qt.locale("fr_FR"), "HH:mm")
}

/**
 * @brief Ouvre le sélecteur de date/heure (DatePicker.qml) pour régler
 *        l'heure système du boîtier, et envoie le réglage à l'ESP32 une
 *        fois la sélection validée.
 * @param _stackView StackView sur lequel pousser le sélecteur.
 * @param _controleurArrosage Instance de ControleurArrosage à qui
 *                             transmettre le réglage (definirHeureSysteme()).
 * @param _datePickerUrl URL résolue de Component/DatePicker.qml (résolue
 *                        par l'appelant QML via Qt.resolvedUrl(), un
 *                        chemin relatif ne serait pas résolu correctement
 *                        depuis ce module .js).
 */
function ouvrirReglageDateHeure(_stackView, _controleurArrosage, _datePickerUrl) {
    var page = _stackView.push(_datePickerUrl, { titre: "Date et heure système" })
    page.validated.connect(function(dateTime) {
        var dateHeureIso = construireDateHeureIso(dateTime)

        console.log("Heure envoyée :", dateHeureIso)   // ← trace temporaire
        _controleurArrosage.definirHeureSysteme(dateHeureIso)
    })
}
