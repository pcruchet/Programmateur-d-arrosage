import QtQuick 2.15
import QtQuick.Layouts 1.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Motif "icône + libellé en gras", utilisé comme sous-titre de section.
 *
 * @details Factorise le couple Text(icone)/Text(titre en gras) répété dans
 *          PageVanne.qml et PageConfiguration.qml (Début d'arrosage,
 *          Fréquence d'arrosage, Adresse IP du boîtier, ...). Style basé sur
 *          Constantes.tailleTitre1/tailleTitre2/couleurAppliTexte.
 */
RowLayout {
    id: control

    property string icone: "" ///< Émoji/caractère affiché à gauche du libellé.
    property string titre: "" ///< Libellé affiché en gras à droite de l'icône.

    spacing: Constantes.tailleEspacement

    Text {
        text: control.icone
        font.pixelSize: Constantes.tailleTitre1
    }

    Text {
        text: control.titre
        font.bold: true
        font.pixelSize: Constantes.tailleTitre2
        color: Constantes.couleurAppliTexte
        Layout.alignment: Qt.AlignVCenter
    }
}
