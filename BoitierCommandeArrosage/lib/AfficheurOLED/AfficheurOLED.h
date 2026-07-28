/**
 * @file    AfficheurOLED.h
 * @brief   Pilotage de l'écran OLED SSD1306 : affichage du statut réseau,
 *          de la connexion client, de l'état des 4 vannes et de l'heure
 *          système.
 *
 * @details Étend directement Adafruit_SSD1306 pour bénéficier de ses
 *          primitives de dessin (setCursor(), print(), etc.). L'affichage
 *          fonctionne en deux temps : les méthodes setXxx() ne font que
 *          mettre à jour les données internes (aucun accès au bus I2C),
 *          rafraichir() redessine effectivement les 4 lignes à l'écran en
 *          une seule transaction.
 */

#ifndef AFFICHEUR_OLED_H
#define AFFICHEUR_OLED_H

#include <Adafruit_SSD1306.h>
#include "MesureBatteries.h"

/// Largeur de l'écran OLED, en pixels.
#define LARGEUR_ECRAN  128
/// Hauteur de l'écran OLED, en pixels.
#define HAUTEUR_ECRAN   64
/// Adresse I2C du contrôleur SSD1306.
#define ADRESSE_I2C   0x3C

/**
 * @class AfficheurOLED
 * @brief Affichage 4 lignes (statut WiFi/IP, client, vannes, heure) sur
 *        écran OLED SSD1306 128x64.
 */
class AfficheurOLED : public Adafruit_SSD1306 {
public:
    /// Construit l'afficheur (résolution LARGEUR_ECRAN x HAUTEUR_ECRAN, bus
    /// I2C partagé Wire, pas de broche de reset dédiée) sans encore
    /// communiquer avec le contrôleur SSD1306.
    AfficheurOLED();

    /**
     * @brief Initialise la communication I2C avec le contrôleur SSD1306.
     * @details Appelle Adafruit_SSD1306::begin() avec la pompe de charge
     *          interne et l'adresse ADRESSE_I2C. En cas de succès, efface
     *          l'écran, configure la couleur de texte et affiche un écran
     *          vide initial.
     * @return true si l'initialisation du contrôleur a réussi.
     */
    bool initialiser();

    // ── Mise à jour des données ──

    /**
     * @brief Met à jour le mode WiFi affiché (donnée seule, sans
     *        redessiner).
     * @param _mode Libellé du mode WiFi ("AP" ou "STA").
     */
    void setModeWifi(const String &_mode);        // "AP" ou "STA"

    /**
     * @brief Met à jour l'adresse IP affichée (donnée seule).
     * @param _ip Adresse IP à afficher.
     */
    void setAdresseIP(const String &_ip);

    /**
     * @brief Met à jour le statut de connexion du client affiché (donnée
     *        seule).
     * @param _connecte true si un client WebSocket est connecté.
     */
    void setClientConnecte(bool _connecte);

    /**
     * @brief Met à jour l'état des vannes affiché (donnée seule).
     * @param _etats Tableau d'états (true = ouverte) ; seuls les 4 premiers
     *               éléments sont pris en compte.
     * @param _nb    Nombre d'éléments valides dans _etats (borné à 4).
     */
    void setEtatVannes(bool _etats[], uint8_t _nb);

    /**
     * @brief Met à jour l'heure système affichée (donnée seule).
     * @param _heure Heure à afficher (chaîne libre, typiquement ISO8601 ou
     *               "--:--" au repos).
     */
    void setHeureSysteme(const String &_heure);

    // ── Rafraîchissement ──

    /**
     * @brief Redessine intégralement l'écran à partir des données
     *        courantes.
     * @details Efface le buffer d'affichage, dessine successivement les 4
     *          lignes (mode WiFi + IP, statut client, état des vannes,
     *          heure système) puis transfère le buffer vers l'écran
     *          (display()). Seule méthode de cette classe à communiquer
     *          effectivement avec le contrôleur SSD1306 pour le contenu
     *          affiché.
     */
    void rafraichir();                            // redessine tout

    // ── Écran batterie ──

    /**
     * @brief Affiche un écran dédié à l'état de la batterie (tension,
     *        courant, pourcentage estimé, état de charge), en remplacement
     *        temporaire de l'écran habituel (statut réseau/vannes/heure).
     * @details Ne modifie aucune des données internes utilisées par
     *          rafraichir() (modeWifi, adresseIP, etatsVannes, etc.) : un
     *          appel ultérieur à rafraichir() restaure l'affichage normal
     *          sans perte d'information. Communique directement avec le
     *          contrôleur SSD1306 (clearDisplay() + display()), comme
     *          rafraichir().
     * @param _mesure             Relevé courant du capteur INA219 (ignoré
     *                            si _capteurDisponible est false).
     * @param _capteurDisponible  false si le capteur INA219 n'a pas pu être
     *                            initialisé : affiche un message d'erreur
     *                            à la place des valeurs.
     */
    void afficherMesureBatterie(const MesureBatterie &_mesure, bool _capteurDisponible);

    // ── Gestion de l'énergie ──

    /// Éteint l'écran (commande SSD1306_DISPLAYOFF) sans perdre le contenu
    /// du buffer, pour économiser l'énergie pendant les phases de veille.
    void eteindre();

    /// Rallume l'écran (commande SSD1306_DISPLAYON), réaffichant le
    /// dernier contenu du buffer.
    void allumer();

private:
    String  modeWifi        = "---";        ///< Mode WiFi courant affiché sur la ligne 1 ("AP", "STA" ou "---" au repos).
    String  adresseIP       = "---";        ///< Adresse IP courante affichée sur la ligne 1.
    bool    clientConnecte  = false;         ///< Statut de connexion du client WebSocket, affiché sur la ligne 2.
    bool    etatsVannes[4]  = {false};       ///< État courant (ouverte/fermée) de chacune des 4 vannes, affiché sur la ligne 3.
    uint8_t nbVannes        = 0;              ///< Nombre de vannes valides dans etatsVannes (borné à 4 par setEtatVannes()).
    String  heureSysteme    = "--:--";       ///< Heure système courante affichée sur la ligne 4.

    /// Dessine la ligne 1 : mode WiFi suivi de l'adresse IP.
    void dessinerLigne1();   // mode Wifi + IP

    /// Dessine la ligne 2 : statut de connexion du client ("connecte" /
    /// "deconnecte").
    void dessinerLigne2();   // statut client

    /// Dessine la ligne 3 : état ("O"/"F") de chacune des nbVannes vannes,
    /// sous la forme "V1:O V2:F ...".
    void dessinerLigne3();   // état vannes

    /// Dessine la ligne 4 : heure système courante.
    void dessinerLigne4();   // heure système
};

#endif // AFFICHEUR_OLED_H
