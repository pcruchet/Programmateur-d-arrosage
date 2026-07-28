import QtQuick 2.15
import QtQuick.Controls 2.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Roue de sélection (Tumbler) d'un nombre à deux chiffres (00 à valeurMax-1).
 *
 * @details Factorise les roues heures/minutes de DatePicker.qml : valeurs
 *          affichées avec un zéro de tête (pad2), texte atténué hors de la
 *          position centrale. Émet valeurChangee() à chaque changement de
 *          sélection.
 */
Tumbler {
    id: control

    property int valeurMax: 60         ///< Nombre de valeurs de la roue (0 à valeurMax-1), ex. 24 pour les heures, 60 pour les minutes.
    property int valeurSelectionnee: 0 ///< Valeur initialement sélectionnée (index de départ de la roue).

    signal valeurChangee(int valeur) ///< Émis à chaque changement de sélection, avec la nouvelle valeur.

    model: valeurMax
    currentIndex: valeurSelectionnee
    visibleItemCount: 3

    implicitWidth: 60
    implicitHeight: 100
    wrap: true

    delegate: Item {
        id: delegateItem
        width: control.width
        height: control.height / control.visibleItemCount

        Text {
            anchors.centerIn: parent

            text: modelData < 10 ? "0" + modelData : modelData

            font.pixelSize: 20
            font.bold: true

            color: delegateItem.Tumbler.displacement === 0 ? Constantes.couleurTexteSombre : Constantes.couleurTexteAttenue

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            opacity: 1.0 - Math.abs(delegateItem.Tumbler.displacement) / 3
        }
    }

    onCurrentIndexChanged: control.valeurChangee(currentIndex)
}
