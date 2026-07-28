pragma Singleton

import QtQuick 2.15

/**
 * @brief Singleton QML regroupant les constantes visuelles de l'application
 *        (couleurs, tailles, espacements).
 *
 * @details Accessible depuis n'importe quel fichier QML du module
 *          "ControleurArrosage" sans import explicite (singleton du même
 *          module). Objectif : centraliser les valeurs de style pour éviter
 *          leur duplication et faciliter une charte graphique cohérente.
 */
QtObject {
    readonly property color couleurAppliFond: "AliceBlue"             ///< Couleur de fond générale de l'application.
    readonly property color couleurAppliTexte: "RoyalBlue"            ///< Couleur de texte principale de l'application.
    readonly property color couleurAppliTexteSecondaire: "Gray"       ///< Couleur de texte secondaire (libellés atténués).

    readonly property int largeurAppli: 360      ///< Largeur de référence de la fenêtre applicative.
    readonly property int hauteurAppli: 720      ///< Hauteur de référence de la fenêtre applicative.
    readonly property int radiusAppli: 20        ///< Rayon d'arrondi standard des cartes/blocs.
    readonly property int margeMiniAppli: 2      ///< Marge minimale utilisée ponctuellement.
    readonly property double bordureAppli: 2     ///< Épaisseur de bordure standard des cartes/blocs.

    readonly property color couleurStatusTexte: "Black"       ///< Couleur de texte de la barre de statut.
    readonly property color couleurStatusFond: "LightGray"    ///< Couleur de fond de la barre de statut.

    readonly property color couleurFondBouton: "White"        ///< Fond des cartes/boutons clairs (ex. PagePrincipale).

    readonly property color couleurIconFond: "#3498db" ///< Fond des icônes rondes.
    readonly property int tailleIcone: 50               ///< Taille standard des icônes rondes.

    readonly property int tailleEspacement: 10 ///< Espacement standard entre éléments d'une même mise en page.

    readonly property int margeStackView: 20 ///< Marge standard autour du contenu d'une page (StackView).

    readonly property int hauteurStatus: 36 ///< Hauteur de la barre de statut de connexion.

    readonly property int tailleTitre1: 30 ///< Taille de police des titres de premier niveau (et des icônes de Titre).
    readonly property int tailleTitre2: 15 ///< Taille de police des titres de second niveau.

    // --- Palette "composants" (AppButton, AppTextField, DatePicker) ---
    readonly property color couleurBlanc: "white"                  ///< Blanc pur, utilisé pour les fonds de carte et le texte sur fond coloré.
    readonly property color couleurPrimaire: "#1E88E5"              ///< Bleu principal (boutons, focus, accents).
    readonly property color couleurPrimaireSombre: "#1565C0"        ///< Bleu principal, variante pressée/foncée.
    readonly property color couleurAccentSecondaire: "#4A90E2"      ///< Bleu secondaire (blocs de configuration).

    readonly property color couleurTexteSombre: "#263238"           ///< Texte principal sombre.
    readonly property color couleurTexteSecondaireSombre: "#37474F" ///< Texte secondaire sombre (ex. bouton non-primaire).
    readonly property color couleurTexteAttenue: "#B0BEC5"          ///< Texte atténué / placeholder.
    readonly property color couleurTexteDesactive: "#9E9E9E"        ///< Texte désactivé.
    readonly property color couleurTexteMuted: "#607D8B"            ///< Texte discret (ex. libellés des jours de la semaine).

    readonly property color couleurBordureClair: "#E0E0E0" ///< Bordures claires des composants.
    readonly property color couleurFondNeutre: "#F5F5F5"   ///< Fonds neutres clairs des composants.

    readonly property color couleurBoutonRetourFond: "#FFE0B2" ///< Fond du bouton retour "←" des en-têtes de page.
    readonly property color couleurBoutonRetourTexte: "#555555" ///< Texte du bouton retour "←" des en-têtes de page.

    readonly property color couleurSucces: "#4CAF50" ///< Couleur de succès / statut connecté / jour sélectionné.
    readonly property color couleurAlerte: "#FFA000" ///< Couleur d'alerte / statut de connexion en cours.
    readonly property color couleurErreur: "#F44336" ///< Couleur d'erreur / statut déconnecté.

    // --- Tailles "composants" ---
    readonly property int radiusComposant: 10 ///< Rayon d'arrondi standard des composants (boutons, champs).
    readonly property int hauteurComposant: 48 ///< Hauteur standard des boutons/champs de saisie.
    readonly property int margePage: 20 ///< Marge standard autour du contenu d'une page.
}
