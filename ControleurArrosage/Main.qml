import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import ControleurArrosage

/**
 * @ingroup qml_pages
 * @brief QML : Fenêtre racine de l'application.
 *
 * @details Point d'entrée QML (chargé par main.cpp via
 *          engine.loadFromModule). Héberge le StackView de navigation
 *          (id "stackView", page initiale PagePrincipale.qml) et la barre
 *          de statut de connexion en bas d'écran. Les ids "window" et
 *          "stackView" définis ici sont utilisés directement par les autres
 *          pages/composants (ex. window.vanneActive, stackView.push()),
 *          la portée des ids QML étant globale au sein du même arbre de
 *          composants.
 */
Window {
    id: window
    visible: true
    width: Constantes.largeurAppli
    height: Constantes.hauteurAppli
    color: Constantes.couleurAppliTexte
    title: "Controleur d'arrosage"

    property int vanneActive: 0 ///< Index de la vanne actuellement affichée par PageVanne.qml (mis à jour par PagePrincipale.qml avant de pousser cette page).

    Rectangle {
        id: appFrame
        anchors.fill: parent
        anchors.margins: Constantes.margeMiniAppli
        radius: Constantes.radiusAppli
        color: Constantes.couleurAppliFond
        border.color: Constantes.couleurAppliTexte
        border.width: Constantes.bordureAppli
        clip: true

        StackView {
            id: stackView
            anchors.fill: parent
            anchors.margins: Constantes.margeStackView
            initialItem: "PagePrincipale.qml"
        }

        // --- BARRE DE STATUT EN BAS ---
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Constantes.hauteurStatus
            color: Constantes.couleurStatusFond
            radius: 0

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Constantes.hauteurStatus / 2
                anchors.rightMargin: Constantes.hauteurStatus / 2
                spacing: Constantes.hauteurStatus / 2

                RowLayout {
                    spacing: 5

                    Rectangle {
                        id: pastilleStatut
                        width: 10
                        height: 10
                        radius: 5
                        color: controleurArrosage.connecte
                               ? Constantes.couleurSucces
                               : (controleurArrosage.enConnexion ? Constantes.couleurAlerte : Constantes.couleurErreur)

                        SequentialAnimation on opacity {
                            running: controleurArrosage.enConnexion
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }

                    Text {
                        text: controleurArrosage.connecte
                              ? "Connecté"
                              : (controleurArrosage.enConnexion ? "Connexion..." : "Déconnecté")
                        font.pixelSize: 11
                        color: controleurArrosage.connecte
                               ? Constantes.couleurSucces
                               : (controleurArrosage.enConnexion ? Constantes.couleurAlerte : Constantes.couleurErreur)
                    }
                }

                RowLayout {
                    spacing: 5
                    Text {

                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
