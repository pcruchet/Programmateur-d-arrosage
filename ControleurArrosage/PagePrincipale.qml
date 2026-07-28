import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "js/PagePrincipale.js" as Principale

/**
 * @ingroup qml_pages
 * @brief QML : Page d'accueil : liste des vannes et accès à la configuration.
 *
 * @details Page initiale du StackView (voir Main.qml). Affiche l'en-tête
 *          (titre, bouton réglages) puis la liste des vannes du modèle
 *          controleurArrosage (ListView), chaque ligne indiquant le mode,
 *          l'état et un résumé de la programmation en cours (rafraîchi
 *          chaque minute via refreshMinute). Un clic sur une vanne ouvre
 *          PageVanne.qml pour cette vanne (Principale.ouvrirVanne()).
 */
Item {
    id: bootRoot

    property int refreshMinute: 0 ///< Compteur incrémenté chaque seconde (Timer ci-dessous), utilisé comme dépendance de binding pour forcer le recalcul périodique du statut d'arrosage affiché (voir statutVanne).

    ColumnLayout {
        anchors.fill: parent
        spacing: Constantes.tailleEspacement

        Timer {
            interval: 1000 // toutes les secondes
            repeat: true
            running: true
            triggeredOnStart: true
            onTriggered: {
                ++refreshMinute
            }
        }

        // ================= HEADER =================
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Constantes.tailleEspacement
            RowLayout {
                spacing: Constantes.tailleEspacement * 3
                Layout.alignment: Qt.AlignVCenter
                Button {
                    //anchors.top: parent.top
                    //anchors.left: parent.left
                    anchors.margins: 10
                    flat: true
                    icon.source: "images/settings.svg"
                    icon.color: Constantes.couleurPrimaire
                    icon.width: 50
                    icon.height: 50
                    onClicked: stackView.push("PageConfiguration.qml")
                }

                Rectangle {
                    width: 200
                    height: 100
                    //anchors.right: parent.right
                    radius: Constantes.radiusAppli
                    border.color: Constantes.couleurAppliTexte

                    Image {
                        anchors.margins: 3
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        source: "images/imageArrosage.png"
                    }
                }

            }



            Text {
                text: "Controleur d'arrosage"
                font.pixelSize: Constantes.tailleTitre1
                font.bold: true
                color: Constantes.couleurAppliTexte
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Sélectionnez la vanne à paramètrer"
                font.pixelSize: Constantes.tailleTitre2
                color: Constantes.couleurAppliTexteSecondaire
                Layout.alignment: Qt.AlignHCenter
            }

        }

        // ================= LISTE =================
        ListView {
            id: vannesList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Constantes.tailleEspacement

            model: controleurArrosage
            delegate: Rectangle {
                width: vannesList.width
                height: 80
                radius: Constantes.radiusAppli
                border.color: uiMode.border

                property var uiMode: Principale.getVanneUI(mode)
                property var uiEtat: Principale.getEtatVanne(etat)


                // ================= BACKGROUND =================
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: parent.radius
                    color: Constantes.couleurFondBouton

                    border.width: Constantes.bordureAppli
                    border.color: uiMode.border
                    opacity: ma.pressed ? 0.04 : 0.0
                }

                // ================= CONTENU =================
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: Constantes.tailleEspacement

                    // ICON
                    Rectangle {
                        width: Constantes.tailleIcone
                        height: Constantes.tailleIcone
                        radius: Constantes.tailleIcone / 2
                        color: uiMode.bg
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            anchors.centerIn: parent
                            text: "💧"
                            font.pixelSize: Constantes.tailleTitre1
                        }
                    }

                    // INFOS
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4

                        // NOM + BADGE + ETAT
                        RowLayout {
                            spacing: Constantes.tailleEspacement

                            Text {
                                text: nom
                                font.bold: true
                                font.pixelSize: Constantes.tailleTitre2
                                color: Constantes.couleurAppliTexte
                            }

                            Rectangle {
                                height: 18
                                width: statusTxt.implicitWidth + 30
                                radius: 9
                                color: uiMode.badge

                                Text {
                                    id: statusTxt
                                    anchors.centerIn: parent
                                    text: uiMode.label
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: uiMode.dot
                                }
                            }

                            Text {
                                text: uiEtat.label
                                font.pixelSize: 15
                                color: uiEtat.couleur
                            }
                        }

                        // ================= STATUT ARROSAGE =================
                        Text {
                            id: statutVanne
                            text:{ refreshMinute
                                var statut = " "
                                if(mode === Vanne.Programme)
                                    statut = Principale.statutArrosage(debut,duree,frequence)
                                return statut
                            }
                        }

                    }

                    Text {
                        text: "›"
                        color: Constantes.couleurAppliTexte
                        font.pixelSize: Constantes.tailleTitre1
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                // ================= CLICK =================
                MouseArea {
                    id: ma
                    anchors.fill: parent

                    onClicked: Principale.ouvrirVanne(window, stackView, index, Qt.resolvedUrl("PageVanne.qml"))
                }
            }


            Item {
                Layout.preferredHeight: Constantes.tailleEspacement * 4
            }

            footer: Item {
                width: vannesList.width
                height: Constantes.tailleEspacement * 10


            }
        }
    }


}
