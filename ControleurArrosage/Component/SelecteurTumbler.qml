import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Carte de sélection d'une valeur dans une liste, avec titre et roue
 *        (Tumbler) surlignée.
 *
 * @details Factorise les blocs Fréquence/Durée d'arrosage de PageVanne.qml :
 *          un Titre (icône + libellé), puis une carte contenant une roue de
 *          sélection parmi un tableau de valeurs quelconque (valeurs), avec
 *          un formatage de texte personnalisable (texteValeur). Émet
 *          valeurChangee() à chaque changement de sélection.
 */
ColumnLayout {
    id: control

    property string icone: ""              ///< Émoji/caractère affiché à gauche du titre de la carte.
    property string titre: ""              ///< Libellé affiché au-dessus de la roue.
    property var valeurs: []               ///< Tableau des valeurs proposées par la roue (ex. [1, 6, 12, 24]).
    property int valeurSelectionnee: 0     ///< Valeur initialement sélectionnée (doit être présente dans valeurs).
    property var texteValeur: function(_valeur) { return _valeur } ///< Fonction de formatage : transforme une valeur du tableau en texte affiché (ex. pluriel "heure"/"heures").

    signal valeurChangee(int valeur) ///< Émis à chaque changement de sélection, avec la nouvelle valeur (élément de valeurs).

    spacing: Constantes.tailleEspacement

    Titre {
        Layout.fillWidth: true
        icone: control.icone
        titre: control.titre
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 100

        radius: Constantes.radiusAppli
        color: Constantes.couleurFondNeutre
        border.color: Constantes.couleurBordureClair

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: (parent.height / 2) - 22
            width: parent.width - 20
            height: 40
            radius: 8
            color: Constantes.couleurPrimaire
            opacity: 0.08
        }

        Tumbler {
            id: roue
            anchors.centerIn: parent
            width: parent.width * 0.8
            height: parent.height

            model: control.valeurs
            currentIndex: Math.max(0, control.valeurs.indexOf(control.valeurSelectionnee))
            visibleItemCount: 3

            wrap: true
            implicitWidth: 80
            implicitHeight: 40

            delegate: Text {
                text: control.texteValeur(modelData)

                font.pixelSize: 20
                font.bold: true

                color: Tumbler.displacement === 0 ? Constantes.couleurTexteSombre : Constantes.couleurTexteAttenue

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                opacity: 1.0 - Math.abs(Tumbler.displacement) / 3
            }
            onCurrentIndexChanged: control.valeurChangee(control.valeurs[currentIndex])
        }
    }
}
