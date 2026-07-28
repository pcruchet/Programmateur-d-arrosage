import QtQuick 2.15

/**
 * @ingroup qml_components
 *
 * @brief QML Component : Bouton retour rond ("←") pour les en-têtes de page.
 *
 * @details Dépile la page courante du StackView global (id "stackView")
 *          au clic. Sans propriété : usage direct par simple instanciation
 *          (voir EntetePage.qml).
 */
Rectangle {
    width: Constantes.tailleIcone
    height: Constantes.tailleIcone
    radius: Constantes.tailleIcone / 2
    color: Constantes.couleurBoutonRetourFond

    Text {
        anchors.centerIn: parent
        text: "←"
        font.pixelSize: Constantes.tailleTitre1
        color: Constantes.couleurBoutonRetourTexte
    }

    MouseArea {
        anchors.fill: parent
        onClicked: stackView.pop()
    }
}

