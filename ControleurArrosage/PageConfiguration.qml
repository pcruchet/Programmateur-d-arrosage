import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "Component"
import "js/PageConfiguration.js" as PageConfigurationJS

/**
 * @ingroup qml_pages
 * @brief QML : Page de configuration : connexion au boîtier et heure système.
 *
 * @details Poussée depuis PagePrincipale.qml (bouton réglages). Deux blocs :
 *          la connexion (adresse IP / port, liés aux propriétés
 *          controleurArrosage.adresseIp/port, appliqués via
 *          configurerConnexion()), et le réglage de l'heure système du
 *          boîtier (ouvre DatePicker.qml via
 *          PageConfigurationJS.ouvrirReglageDateHeure()).
 */
Item {
    property int refreshMinute: 0 ///< Compteur incrémenté chaque seconde (Timer ci-dessous), utilisé comme dépendance de binding pour rafraîchir l'affichage de l'heure système courante (voir dateheureSystem).

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Constantes.margePage
        spacing: 16

        Timer {
            interval: 1000
            repeat: true
            running: true
            triggeredOnStart: true
            onTriggered: {
                ++refreshMinute
            }
        }
        EntetePage {
            id: header
            anchors.margins: Constantes.margePage
            height: 60

            titre: "Configuration"
        }
        Item {
            Layout.preferredHeight: Constantes.tailleEspacement * 4
        }
        // ── Bloc connexion ──
        Rectangle {
            Layout.fillWidth: true
            radius: Constantes.radiusAppli
            color: Constantes.couleurFondBouton
            border.width: Constantes.bordureAppli
            border.color: Constantes.couleurAccentSecondaire
            implicitHeight: colonneConnexion.implicitHeight + 40

            ColumnLayout {
                id: colonneConnexion
                anchors.fill: parent
                anchors.margins: Constantes.margePage
                spacing: Constantes.tailleEspacement

                Titre {
                    icone: "🌐"
                    titre: "Adresse IP du boîtier"
                }
                AppTextField {
                    Layout.fillWidth: true
                    horizontalAlignment: TextInput.AlignHCenter
                    font.pixelSize: Constantes.tailleTitre2
                    text: controleurArrosage.adresseIp
                    onEditingFinished: controleurArrosage.adresseIp = text
                }

                Titre {
                    icone: "🔌"
                    titre: "Port de communication"
                }
                AppTextField {
                    Layout.fillWidth: true
                    horizontalAlignment: TextInput.AlignHCenter
                    font.pixelSize: Constantes.tailleTitre2
                    text: controleurArrosage.port
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: controleurArrosage.port = parseInt(text)
                }

                AppButton {
                    Layout.fillWidth: true
                    text: "Appliquer et reconnecter"
                    onClicked: {
                        controleurArrosage.configurerConnexion()
                    }
                }
            }
        }
        Item {
            Layout.preferredHeight: Constantes.tailleEspacement * 4
        }
        // ── Bloc date/heure ──
        Rectangle {
            id: reglageDateHeure
            Layout.fillWidth: true
            implicitHeight: 80
            radius: Constantes.radiusAppli
            color: Constantes.couleurFondBouton
            border.width: Constantes.bordureAppli
            border.color: Constantes.couleurAccentSecondaire

            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                radius: parent.radius
                color: Constantes.couleurAccentSecondaire
                opacity: boutonDate.pressed ? 0.08 : 0.0
            }

            Column {
                anchors.centerIn: parent
                spacing: Constantes.tailleEspacement

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Constantes.tailleEspacement
                    Titre {
                        icone: "🕐"
                        titre: "Réglage date et heure système"
                    }
                }

                Text {
                    id: dateheureSystem
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: {
                        refreshMinute
                        return PageConfigurationJS.dateHeureActuelleTexte()
                    }
                    font.pixelSize: 15
                    color: Constantes.couleurAppliTexteSecondaire
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            MouseArea {
                id: boutonDate
                anchors.fill: parent
                onClicked: PageConfigurationJS.ouvrirReglageDateHeure(stackView, controleurArrosage, Qt.resolvedUrl("Component/DatePicker.qml"))
            }
            Item {
                Layout.preferredHeight: Constantes.tailleEspacement * 4
            }
        }
    }
}
