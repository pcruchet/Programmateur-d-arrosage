/**
 * @file    AfficheurOLED.cpp
 * @brief   Implémentation du pilotage de l'écran OLED SSD1306.
 */


#include "AfficheurOLED.h"

/**
 * @brief Construit l'objet Adafruit_SSD1306 sous-jacent (résolution
 *        LARGEUR_ECRAN x HAUTEUR_ECRAN, bus Wire, pas de broche reset).
 */
AfficheurOLED::AfficheurOLED()
    : Adafruit_SSD1306(LARGEUR_ECRAN, HAUTEUR_ECRAN, &Wire, -1)
{
}

/**
 * @brief Initialise le contrôleur SSD1306 sur le bus I2C.
 * @details Utilise la pompe de charge interne (SSD1306_SWITCHCAPVCC) et
 *          l'adresse ADRESSE_I2C. En cas de succès, efface l'écran,
 *          configure la couleur du texte en blanc et pousse un premier
 *          affichage vide.
 * @return true si begin() a réussi.
 */
bool AfficheurOLED::initialiser()
{
    bool resultat = begin(SSD1306_SWITCHCAPVCC, ADRESSE_I2C);
    if (resultat)
    {
        clearDisplay();
        setTextColor(SSD1306_WHITE);
        display();
    }
    return resultat;
}

/// Met à jour la donnée interne modeWifi (aucun accès I2C).
void AfficheurOLED::setModeWifi(const String &_mode)
{
    modeWifi = _mode;
}

/// Met à jour la donnée interne adresseIP (aucun accès I2C).
void AfficheurOLED::setAdresseIP(const String &_ip)
{
    adresseIP = _ip;
}

/// Met à jour la donnée interne clientConnecte (aucun accès I2C).
void AfficheurOLED::setClientConnecte(bool _connecte)
{
    clientConnecte = _connecte;
}

/**
 * @brief Recopie l'état des vannes dans le tableau interne.
 * @details Borne le nombre de vannes à 4 (nbVannes = min(_nb, 4)) avant de
 *          recopier terme à terme.
 */
void AfficheurOLED::setEtatVannes(bool _etats[], uint8_t _nb)
{
    nbVannes = _nb > 4 ? 4 : _nb;
    for (uint8_t i = 0; i < nbVannes; i++)
        etatsVannes[i] = _etats[i];
}

/// Met à jour la donnée interne heureSysteme (aucun accès I2C).
void AfficheurOLED::setHeureSysteme(const String &_heure)
{
    heureSysteme = _heure;
}

/// Positionne le curseur en haut de l'écran et affiche modeWifi suivi d'un espace et de adresseIP.
void AfficheurOLED::dessinerLigne1()
{
    setTextSize(1);
    setCursor(0, 0);
    print(modeWifi);
    print(" ");
    print(adresseIP);
}

/// Affiche "Client : connecte" ou "Client : deconnecte" selon clientConnecte.
void AfficheurOLED::dessinerLigne2()
{
    setCursor(0, 18);
    print("Client : ");
    print(clientConnecte ? "connecte" : "deconnecte");
}

/// Affiche l'état de chaque vanne sous la forme "Vi:O" ou "Vi:F", séparés par un espace, pour i de 1 à nbVannes.
void AfficheurOLED::dessinerLigne3()
{
    setCursor(0, 36);
    for (uint8_t i = 0; i < nbVannes; i++)
    {
        print("V");
        print(i + 1);
        print(":");
        print(etatsVannes[i] ? "O" : "F");
        if (i < nbVannes - 1)
            print(" ");
    }
}

/// Affiche la chaîne heureSysteme.
void AfficheurOLED::dessinerLigne4()
{
    setCursor(0, 52);
    print(heureSysteme);
}

/**
 * @brief Redessine intégralement l'écran à partir des données courantes.
 * @details Efface le buffer, dessine les 4 lignes dans l'ordre
 *          (dessinerLigne1() à dessinerLigne4()) puis transfère le buffer
 *          vers l'écran physique (display()).
 */
void AfficheurOLED::rafraichir()
{
    clearDisplay();
    dessinerLigne1();
    dessinerLigne2();
    dessinerLigne3();
    dessinerLigne4();
    display();
}

/**
 * @brief Affiche un écran dédié à l'état de la batterie, en remplacement
 *        temporaire de l'écran habituel.
 * @details Ne touche à aucune des données membres utilisées par
 *          rafraichir() : celui-ci reste utilisable tel quel pour revenir
 *          à l'affichage normal une fois l'écran batterie expiré.
 */
void AfficheurOLED::afficherMesureBatterie(const MesureBatterie &_mesure, bool _capteurDisponible)
{
    clearDisplay();
    setTextSize(1);
    setTextColor(SSD1306_WHITE);

    setCursor(0, 0);
    print("Batterie (INA219)");
    drawLine(0, 10, LARGEUR_ECRAN - 1, 10, SSD1306_WHITE);

    if (_capteurDisponible)
    {
        setCursor(0, 16);
        print("U : ");
        print(_mesure.tensionV, 2);
        print(" V");

        setCursor(0, 28);
        print("I : ");
        print(_mesure.courantMa, 0);
        print(" mA");

        setCursor(0, 40);
        print("Charge : ");
        print(_mesure.pourcentage);
        print(" %");

        setCursor(0, 52);
        print(_mesure.enCharge ? "EN CHARGE" : "repos / decharge");
    }
    else
    {
        setCursor(0, 24);
        print("INA219 non detecte");
        setCursor(0, 36);
        print("Verifier le bus I2C");
    }

    display();
}

/// Envoie la commande SSD1306_DISPLAYOFF (extinction sans perte du buffer).
void AfficheurOLED::eteindre()
{
    ssd1306_command(SSD1306_DISPLAYOFF);
}

/// Envoie la commande SSD1306_DISPLAYON (réaffiche le dernier buffer).
void AfficheurOLED::allumer()
{
    ssd1306_command(SSD1306_DISPLAYON);
}
