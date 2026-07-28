/**
 * @file    PagePrincipale.js
 * @brief   Fonctions et données utilitaires de la page PagePrincipale.qml.
 *
 * @details Tables de correspondance mode/état de vanne -> style visuel
 *          (couleurs, libellé) pour l'affichage de chaque ligne de la liste,
 *          calcul du texte de statut d'arrosage affiché pour une vanne en
 *          mode Programme, et action déclenchée par le clic sur une vanne.
 *          La fonction ouvrirVanne() reçoit explicitement les objets QML
 *          dont elle a besoin en paramètre : ce module n'étant pas en
 *          .pragma library, chaque import obtient sa propre instance, mais
 *          les ids du document QML restent inaccessibles depuis un fichier .js.
 */

/// Style visuel associé à chaque valeur de Vanne::MODE (0 = Manuel,
/// 1 = Programme, 2 = Automatique) : couleur de bordure, de puce, de fond
/// d'icône, de fond de badge, et libellé affiché.
const VanneUI = {
    0: { // [Vanne.Manuel]
        border: "Green",
        dot: "Green",
        bg: "lightGreen",
        badge: "lightGreen",
        label: "Manuel"
    },

    1: { // [Vanne.Programme]
        border: "Blue",
        dot: "Blue",
        bg: "LightBlue",
        badge: "LightBlue",
        label: "Programmé"
    },

    2: { // [Vanne.Automatique]
        border: "IndianRed",
        dot: "IndianRed",
        bg: "LightSalmon",
        badge: "LightSalmon",
        label: "Automatique"
    }
};

/// Style visuel associé à l'état ouvert/fermé d'une vanne : couleur et
/// libellé (avec émoji) affichés à côté du badge de mode.
const EtatUI = {
    true: {
        couleur: "Blue",
        label: "💦 Ouverte"
    },
    false: {
        couleur: "Red",
        label: "🚱 Fermée"
    }

};

/**
 * @brief Style visuel associé à un mode de vanne.
 * @param statut Valeur de Vanne::MODE (0, 1 ou 2).
 * @return Entrée de VanneUI correspondante, ou un style neutre ("Indéfinit")
 *         si statut ne correspond à aucune entrée connue.
 */
function getVanneUI(statut) {
    var ui = VanneUI[statut];

    if (ui === undefined) {
        ui = {
            border: "Black",
            dot: "Black",
            bg: "Black",
            badge: "Black",
            label: "Indéfinit"
        };
    }
    return ui;
}

/**
 * @brief Style visuel associé à l'état ouvert/fermé d'une vanne.
 * @param statut État ouvert (true) / fermé (false) de la vanne.
 * @return Entrée de EtatUI correspondante, ou un style neutre ("Indéfinit")
 *         si statut ne correspond à aucune entrée connue.
 */
function getEtatVanne(statut) {
    var ui = EtatUI[statut];
    if (ui === undefined) {
        ui = {
            border: "Black",
            bg: "Black",
            label: "Indéfinit"
        };
    }
    return ui;
}

/**
 * @brief Ouvre la page de détail (PageVanne.qml) pour la vanne cliquée.
 * @param _window Fenêtre racine (Main.qml) dont vanneActive doit être mis à jour.
 * @param _stackView StackView sur lequel pousser PageVanne.qml.
 * @param _index Index de la vanne cliquée dans la liste.
 * @param _pageVanneUrl URL résolue de PageVanne.qml (résolue par l'appelant
 *                       QML via Qt.resolvedUrl()).
 */
function ouvrirVanne(_window, _stackView, _index, _pageVanneUrl) {
    _window.vanneActive = _index
    _stackView.push(_pageVanneUrl)
}

/**
 * @brief Calcule le texte de statut d'arrosage d'une vanne programmée.
 * @param debut Date/heure du premier cycle d'arrosage programmé (objet Date).
 * @param dureeMinutes Durée d'un cycle d'arrosage, en minutes.
 * @param frequenceHeures Fréquence de répétition des cycles, en heures.
 * @return "Prochain arrosage dans : ..." si le premier cycle n'a pas encore
 *         commencé, ou si on est entre deux cycles ; "Fin d'arrosage dans :
 *         ..." si un cycle est en cours.
 */
function statutArrosage(debut, dureeMinutes, frequenceHeures) {
    // debut est maintenant un objet Date directement
    var maintenant  = new Date()
    var dureeMs     = dureeMinutes * 60 * 1000
    var frequenceMs = frequenceHeures * 60 * 60 * 1000
    var resultat    = ""

    if (maintenant < debut) {
        resultat = "Prochain arrosage dans : " + _formatDuree(debut - maintenant)
    } else {
        var positionDansCycle = (maintenant - debut) % frequenceMs

        if (positionDansCycle < dureeMs) {
            var restantMin = Math.ceil((dureeMs - positionDansCycle) / 60000)
            resultat = "Fin d'arrosage dans : " + restantMin
                       + (restantMin <= 1 ? " minute" : " minutes")
        } else {
            resultat = "Prochain arrosage dans : " + _formatDuree(frequenceMs - positionDansCycle)
        }
    }

    return resultat
}

/**
 * @brief Formate une durée en millisecondes en texte lisible (heures/minutes,
 *        ou secondes si moins d'une minute).
 * @param ms Durée en millisecondes.
 * @return Durée formatée ("XhYYmin", "Xh", "Ymin" ou "Zs" selon l'ordre de
 *         grandeur).
 */
function _formatDuree(ms) {
    var totalSec = Math.floor(ms / 1000)
    var heures   = Math.floor(totalSec / 3600)
    var minutes  = Math.floor((totalSec % 3600) / 60)
    var secondes = totalSec % 60
    var resultat = ""

    if (heures > 0 && minutes > 0) {
        resultat = heures + "h " + minutes + "min"
    } else if (heures > 0) {
        resultat = heures + "h"
    } else if (minutes > 0) {
        resultat = minutes + "min"
    } else {
        resultat = secondes + "s"
    }

    return resultat
}
