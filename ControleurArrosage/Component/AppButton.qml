import QtQuick 2.15
import QtQuick.Controls 2.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Bouton stylé de l'application (variantes primaire/secondaire).
 *
 * @details Redéfinit l'apparence du Button Qt Quick Controls standard :
 *          fond plein + ombre douce en variante primaire (isPrimary true,
 *          par défaut), fond neutre + bordure en variante secondaire. Les
 *          couleurs et tailles proviennent de Constantes (radiusComposant,
 *          hauteurComposant, palette "composants").
 */
Button {
    id: control

    property bool isPrimary: true ///< true (défaut) pour la variante primaire (fond plein), false pour la variante secondaire (fond neutre + bordure).

    /// Couleur du texte selon la variante (ignorée si le bouton est désactivé, voir contentItem).
    property color textColor: {
        if (isPrimary) { return Constantes.couleurBlanc; }
        return Constantes.couleurTexteSecondaireSombre;
    }

    contentItem: Text {
        text: control.text
        color: {
            if (control.enabled) { return control.textColor; }
            return Constantes.couleurTexteDesactive;
        }
        font.pixelSize: 14
        font.bold: true
        font.letterSpacing: 0.5
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: Constantes.hauteurComposant
        radius: Constantes.radiusComposant

        color: {
            if (!control.enabled) { return Constantes.couleurBordureClair; }
            if (isPrimary) {
                if (control.pressed) { return Constantes.couleurPrimaireSombre; }
                return Constantes.couleurPrimaire;
            }
            if (control.pressed) { return Constantes.couleurBordureClair; }
            return Constantes.couleurFondNeutre;
        }

        border.width: {
            if (isPrimary) { return 0; }
            return 1;
        }
        border.color: Constantes.couleurBordureClair

        // Ombre douce sous le bouton primaire
        Rectangle {
            visible: isPrimary && control.enabled
            anchors.fill: parent
            anchors.topMargin: 2
            radius: parent.radius
            color: Constantes.couleurPrimaire
            opacity: 0.2
            z: -1
        }
    }
}
