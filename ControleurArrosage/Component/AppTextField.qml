import QtQuick 2.15
import QtQuick.Controls 2.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Champ de saisie stylé de l'application.
 *
 * @details Redéfinit l'apparence du TextField Qt Quick Controls standard :
 *          fond neutre au repos, fond blanc + bordure bleue en focus.
 *          Couleurs et tailles issues de Constantes (radiusComposant,
 *          hauteurComposant, palette "composants").
 */
TextField {
    id: control

    font.pixelSize: 14
    color: Constantes.couleurTexteSombre
    placeholderTextColor: Constantes.couleurTexteAttenue
    leftPadding: 14
    rightPadding: 14

    background: Rectangle {
        implicitHeight: Constantes.hauteurComposant
        color: {
            if (control.activeFocus) { return Constantes.couleurBlanc; }
            return Constantes.couleurFondNeutre;
        }
        radius: Constantes.radiusComposant
        border.width: {
            if (control.activeFocus) { return 2; }
            return 1;
        }
        border.color: {
            if (control.activeFocus) { return Constantes.couleurPrimaire; }
            return Constantes.couleurBordureClair;
        }
    }
}
