import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "Component"
import "js/PageVanne.js" as PageVanneJS

/**
 * @ingroup qml_pages
 * @brief QML : Page de détail et de programmation d'une vanne.
 *
 * @details Poussée sur le StackView global depuis PagePrincipale.qml, pour
 *          la vanne désignée par window.vanneActive. Propose l'ouverture/
 *          fermeture manuelle, l'activation/désactivation de la
 *          programmation, et un formulaire de configuration (programmeForm :
 *          début, fréquence, durée) déclenché par "Configurer le
 *          programme". Les valeurs modifiées dans le formulaire ne sont
 *          envoyées à l'ESP32 qu'au clic sur "Enregistrer"
 *          (PageVanneJS.enregistrerProgrammation()).
 */
Item {
    id: vanneRoot

    property int vanneIndex: window.vanneActive                          ///< Index de la vanne affichée (miroir de window.vanneActive).
    property var vanneCourante: controleurArrosage.vanne(vanneIndex)      ///< Vanne courante (QObject* exposé par ControleurArrosage::vanne()).

    /// Valeurs de durée proposées par le sélecteur (minutes).
    property var durees: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 30, 60, 90, 120]
    /// Valeurs de fréquence proposées par le sélecteur (heures).
    property var frequences: [1, 6, 12, 24, 48, 96]

    /// Durée actuellement sélectionnée dans le formulaire (avant validation).
    property int dureeSelectionnee: (vanneCourante && vanneCourante["duree"] > 0) ? vanneCourante["duree"] : durees[0]
    /// Fréquence actuellement sélectionnée dans le formulaire (avant validation).
    property int frequenceSelectionnee: (vanneCourante && vanneCourante["frequence"] > 0) ? vanneCourante["frequence"] : frequences[0]

    // ================= HEADER =================
    EntetePage {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Constantes.margePage
        height: 60

        titre: "Vanne " + (vanneIndex + 1)
    }

    // ================= ACTIONS =================
    ColumnLayout {
        anchors.top: header.bottom
        anchors.topMargin: 50
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Constantes.margePage
        spacing: 15

        // ================= OUVERTURE / FERMETURE =================
        AppButton {
            Layout.fillWidth: true

            text: vanneCourante["etat"]
                  ? "Fermer manuellement"
                  : "Ouvrir manuellement"

            onClicked: {
                controleurArrosage.toggleEtat(vanneIndex)
            }
        }

        // ================= MODE PROGRAMME =================
        AppButton {
            Layout.fillWidth: true

            text: vanneCourante["programmeState"] === Vanne.Actif
                  ? "Désactiver programme"
                  : "Activer programme"

            onClicked: PageVanneJS.togglerProgramme(controleurArrosage, vanneIndex, vanneCourante.programmeState === Vanne.Actif)
        }

        // ================= CONFIGURATION =================
        AppButton {
            Layout.fillWidth: true

            text: "Configurer le programme"
            enabled: vanneCourante.programmeState !== Vanne.Actif

            onClicked: stackView.push(programmeForm)
        }
    }

    // ================= FORMULAIRE =================

    /// Formulaire de configuration de la programmation (début, fréquence,
    /// durée), instancié à la demande via stackView.push(programmeForm) au
    /// clic sur "Configurer le programme".
    Component {
        id: programmeForm

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Constantes.margePage
                spacing: 16

                Text {
                    text: "Programmation Vanne " + (vanneIndex + 1)
                    font.pixelSize: 20
                    font.bold: true
                    color: Constantes.couleurTexteSombre
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 500
                    radius: Constantes.radiusAppli
                    color: Constantes.couleurBlanc
                    border.color: Constantes.couleurBordureClair

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Constantes.margePage
                        spacing: Constantes.tailleEspacement

                        // ================= DEBUT =================
                        ColumnLayout {
                            spacing: Constantes.tailleEspacement

                            Titre {
                                icone: "📅"
                                titre: "Début d'arrosage"
                            }

                            Item {
                                Layout.fillWidth: true
                                implicitHeight: debutProgramme.implicitHeight
                                AppTextField {
                                    id: debutProgramme
                                    anchors.fill: parent
                                    horizontalAlignment: TextInput.AlignHCenter

                                    font.pixelSize: 20
                                    placeholderText: vanneCourante.debut.toLocaleDateString(Qt.locale("fr_FR"),Locale.ShortFormat) +
                                                     " - " + vanneCourante.debut.toLocaleTimeString(Qt.locale("fr_FR"),Locale.ShortFormat)
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: PageVanneJS.ouvrirSelecteurDebut(stackView, debutProgramme, Qt.resolvedUrl("Component/DatePicker.qml"))
                                }
                            }
                        }

                        // ================= FREQUENCE =================
                        SelecteurTumbler {
                            Layout.fillWidth: true
                            icone: "⏲"
                            titre: "Fréquence d'arrosage"
                            valeurs: frequences
                            valeurSelectionnee: frequenceSelectionnee
                            texteValeur: PageVanneJS.libelleFrequence
                            onValeurChangee: function(_valeur) {
                                controleurArrosage.setFrequence(vanneIndex, _valeur)
                            }
                        }

                        // ================= DUREE =================
                        SelecteurTumbler {
                            Layout.fillWidth: true
                            icone: "⏳"
                            titre: "Durée d'arrosage"
                            valeurs: durees
                            valeurSelectionnee: dureeSelectionnee
                            texteValeur: PageVanneJS.libelleDuree
                            onValeurChangee: function(_valeur) {
                                controleurArrosage.setDuree(vanneIndex, _valeur)
                            }
                        }
                    }
                }

                // ================= ACTION BAR =================
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
                        text: "Enregistrer"
                        Layout.fillWidth: true

                        onClicked: PageVanneJS.enregistrerProgrammation(controleurArrosage, vanneIndex, dureeSelectionnee, frequenceSelectionnee, debutProgramme, stackView)
                    }
                }
            }
        }
    }
}
