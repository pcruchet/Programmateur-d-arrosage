import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.impl 2.15
import QtQuick.Controls as OldControls
import "../js/DatePicker.js" as DatePickerJS

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Page de sélection d'une date et d'une heure (calendrier + roues heure/minute).
 *
 * @details Poussée sur le StackView global (id "stackView") pour choisir la
 *          date/heure de début d'un arrosage (PageVanne.qml) ou régler
 *          l'heure système du boîtier (PageConfiguration.qml). Le calendrier
 *          (OldControls.MonthGrid) sélectionne le jour/mois/année, les deux
 *          RoueChiffres sélectionnent l'heure et la minute. Au clic sur
 *          "Valider", émet validated() avec la date/heure formatée
 *          ("dd/MM/yyyy - HH:mm") et se dépile elle-même.
 */
Item {
    id: root
    width: StackView.view ? StackView.view.width : 0
    height: StackView.view ? StackView.view.height : 0

    property int selectedDay: new Date().getDate()          ///< Jour sélectionné dans le calendrier (1-31), initialisé à aujourd'hui.
    property int selectedMonth: new Date().getMonth()       ///< Mois sélectionné (0-11), initialisé au mois courant.
    property int selectedYear: new Date().getFullYear()     ///< Année sélectionnée, initialisée à l'année courante.
    property int selectedHour: new Date().getHours()        ///< Heure sélectionnée (0-23), initialisée à l'heure courante.
    property int selectedMinute: new Date().getMinutes()    ///< Minute sélectionnée (0-59), initialisée à la minute courante.
    property string selectedDateTime: ""                    ///< Date/heure formatée ("dd/MM/yyyy - HH:mm"), calculée par validerSelection() au clic sur "Valider".

    property string titre: "Début d'arrosage" ///< Titre affiché en haut de la page (personnalisable via les propriétés de push()).
    signal validated(string dateTime) ///< Émis au clic sur "Valider", avec la date/heure sélectionnée formatée ("dd/MM/yyyy - HH:mm").

    Rectangle {
        anchors.fill: parent
        color: Constantes.couleurFondNeutre
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Constantes.margePage
        spacing: 24

        // ================= HEADER =================
        Text {
            text: titre
            font.pixelSize: 24
            font.bold: true
            color: Constantes.couleurTexteSombre
        }

        // ================= CARD CALENDRIER =================
        Rectangle {
            Layout.fillWidth: true
            radius: 22
            color: Constantes.couleurBlanc

            border.color: Constantes.couleurBordureClair

            implicitHeight: calendarColumn.implicitHeight + 32

            ColumnLayout {
                id: calendarColumn

                anchors.fill: parent
                anchors.margins: 16
                spacing: 15

                // ================= MOIS =================
                RowLayout {
                    Layout.fillWidth: true

                    AppButton {
                        text: "‹"
                        implicitWidth: 40
                        onClicked: DatePickerJS.changerMois(root, -1)
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter

                        text: Qt.locale().standaloneMonthName(
                                  selectedMonth + 1) + " " + selectedYear

                        font.pixelSize: 20
                        font.bold: true
                        color: Constantes.couleurTexteSombre
                    }

                    AppButton {
                        text: "›"
                        implicitWidth: 40
                        onClicked: DatePickerJS.changerMois(root, 1)
                    }
                }

                // ================= JOURS =================
                OldControls.DayOfWeekRow {
                    locale: Qt.locale()
                    Layout.fillWidth: true

                    delegate: Text {
                        text: model.shortName
                        font.bold: true
                        color: Constantes.couleurTexteMuted
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // ================= CALENDRIER =================
                OldControls.MonthGrid {
                    id: monthGrid

                    Layout.fillWidth: true

                    month: selectedMonth
                    year: selectedYear
                    locale: Qt.locale()

                    delegate: Rectangle {

                        required property var model

                        radius: width / 2

                        color: {
                            if (model.day === selectedDay
                                    && model.month === selectedMonth)
                                return Constantes.couleurSucces

                            return "transparent"
                        }

                        border.width: model.today ? 1 : 0
                        border.color: Constantes.couleurSucces

                        implicitWidth: 30
                        implicitHeight: 30

                        Text {
                            anchors.centerIn: parent

                            text: model.day

                            color: {
                                if (model.day === selectedDay
                                        && model.month === selectedMonth)
                                    return Constantes.couleurBlanc

                                if (model.month !== selectedMonth)
                                    return Constantes.couleurTexteAttenue

                                return Constantes.couleurTexteSombre
                            }

                            font.bold: model.day === selectedDay
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: DatePickerJS.selectionnerJour(root, model.day, model.month)
                        }
                    }
                }
            }
        }

        // ================= HEURE =================
        Rectangle {
            Layout.fillWidth: true
            radius: 22
            color: Constantes.couleurBlanc
            border.color: Constantes.couleurBordureClair
            border.width: 1

            implicitHeight: 100
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: (parent.height / 2) - 22
                width: parent.width - 40
                height: 40
                radius: 8
                color: Constantes.couleurPrimaire
                opacity: 0.08
            }


            RowLayout {
                anchors.centerIn: parent
                spacing: 15

                RoueChiffres {
                    valeurMax: 24
                    valeurSelectionnee: selectedHour
                    onValeurChangee: function(_valeur) { selectedHour = _valeur }
                }

                Text {
                    text: ":"
                    font.pixelSize: 24
                    font.bold: true
                    color: Constantes.couleurTexteSombre
                    Layout.alignment: Qt.AlignVCenter
                }

                RoueChiffres {
                    valeurMax: 60
                    valeurSelectionnee: selectedMinute
                    onValeurChangee: function(_valeur) { selectedMinute = _valeur }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        // ================= BOUTON =================
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            AppButton {
                text: "Annuler"
                isPrimary: false
                Layout.fillWidth: true
                onClicked: stackView.pop()
            }

            AppButton {
                text: "Valider"
                Layout.fillWidth: true
                onClicked: DatePickerJS.validerSelection(root, stackView)
            }
        }
        Item {
            Layout.preferredHeight: Constantes.tailleEspacement * 4
        }
    }
}
