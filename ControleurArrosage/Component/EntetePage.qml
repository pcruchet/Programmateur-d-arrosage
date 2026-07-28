import QtQuick 2.15
import QtQuick.Layouts 1.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : En-tête de page standard : bouton retour + titre.
 *
 * @details Factorise le motif "Retour + titre" répété en haut de
 *          PageVanne.qml et PageConfiguration.qml. Ne gère pas son propre
 *          positionnement (anchors, height) : c'est à la page appelante de
 *          les définir selon son contexte (voir les usages).
 */
RowLayout {
    id: control

    property string titre: "" ///< Titre affiché à droite du bouton retour.

    spacing: 15

    Retour {
    }

    Text {
        Layout.fillWidth: true
        text: control.titre
        font.pixelSize: 20
        font.bold: true
        color: Constantes.couleurTexteSombre
    }
}
