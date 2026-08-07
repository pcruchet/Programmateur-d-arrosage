/**
 * @file    BoitierPilotageArrosage.cpp
 * @brief   Implémentation de l'automate d'états BoitierPilotageArrosage.
 *
 * @details Contient le constructeur, l'initialisation matérielle, la boucle
 *          de dispatch de l'automate (controler()), le traitement de chacun
 *          des états (::EtatBoitier), le décodage/traitement des requêtes
 *          WebSocket (protocole applicatif), la gestion des sources de
 *          réveil ESP32 (deep sleep / light sleep) et les utilitaires de
 *          pilotage des vannes et de l'affichage OLED.
 */

// src/BoitierPilotageArrosage.cpp

#include "BoitierPilotageArrosage.h"

// ============================================================
// Constructeur
// ============================================================

/**
 * @brief Construit l'automate et initialise les pointeurs de composants et
 *        les compteurs d'activité à leurs valeurs de repos, sans toucher au
 *        matériel. L'état de départ est EtatBoitier::DEMARRAGE.
 */
BoitierPilotageArrosage::BoitierPilotageArrosage()
    : afficheur(nullptr), serveurWebSocket(nullptr), protocole(nullptr), gestionnaireTemps(nullptr), stockage(nullptr), stockageEtat(nullptr), mesureBatteries(nullptr), bp2(nullptr), etatCourant(EtatBoitier::DEMARRAGE), causeReveil(CauseReveil::INCONNUE), dernierMessageRecuMs(0), dernierRafraichissementOledMs(0), wifiDemarre(false), affichageBatterieActif(false), finAffichageBatterieMs(0)
{
    vannes[0] = nullptr;
    vannes[1] = nullptr;
    vannes[2] = nullptr;
    vannes[3] = nullptr;
}

// ============================================================
// Initialisation
// ============================================================

/**
 * @brief Alloue et initialise tous les composants matériels/logiciels
 *        (bus I2C, bouton BP1, protocole, stockages NVS, les 4 vannes,
 *        afficheur OLED, RTC, serveur WebSocket) et enregistre le callback
 *        de réception des messages WebSocket.
 * @return true si l'initialisation de tous les sous-systèmes (stockage
 *         programmation, stockage état, afficheur, RTC) a réussi ; false
 *         sinon, avec le détail des échecs tracé sur le port série.
 */
bool BoitierPilotageArrosage::initialiser()
{
    bool resultat = false;

    Wire.begin();
    pinMode(BROCHE_BP1, INPUT);

    protocole = new ProtocoleArrosageServeur();
    stockage = new StockageProgrammationVannes();
    stockageEtat = new StockageEtatVannes();
    vannes[0] = new Vanne(PIN_SEL_V1, PIN_INA, PIN_INB);
    vannes[1] = new Vanne(PIN_SEL_V2, PIN_INA, PIN_INB);
    vannes[2] = new Vanne(PIN_SEL_V3, PIN_INA, PIN_INB);
    vannes[3] = new Vanne(PIN_SEL_V4, PIN_INA, PIN_INB);
    afficheur = new AfficheurOLED();
    gestionnaireTemps = new GestionnaireTemps(BROCHE_IRQ_RTC);
    serveurWebSocket = new ServeurWebSocket(*afficheur);
    mesureBatteries = new MesureBatteries();
    bp2 = new BoutonPoussoir(BROCHE_BP2, false); // GPIO39 : pas de tirage interne, resistance externe sur la carte

    bool stockageOk = stockage->initialiser();
    bool stockageEtatOk = stockageEtat->initialiser();
    bool afficheurOk = afficheur->initialiser();
    bool gestionnaireTempsOk = gestionnaireTemps->initialiser();

    // Composant auxiliaire : une absence de capteur INA219 ne doit pas
    // empêcher le boîtier de fonctionner (arrosage/dialogue), seulement
    // désactiver l'affichage batterie (voir afficherMesureBatterie()).
    bool mesureBatteriesOk = mesureBatteries->initialiser();
    if (!mesureBatteriesOk)
        DEBUG("INA219 non detecte — affichage batterie indisponible, reste du boitier non affecte");

    if (stockageOk && stockageEtatOk && afficheurOk && gestionnaireTempsOk)
    {
        serveurWebSocket->setOnMessage([this](const String &_message)
                                       { onMessageRecu(_message); });

        DEBUG("Initialisation OK");
        etatCourant = EtatBoitier::DEMARRAGE;
        resultat = true;
        dernierVerificationProgMs = millis();
    }
    else
    {
        DEBUG_VAL("stockage->initialiser()          : ", stockageOk ? "OK" : "ECHEC");
        DEBUG_VAL("stockageEtat->initialiser()      : ", stockageEtatOk ? "OK" : "ECHEC");
        DEBUG_VAL("afficheur->initialiser()         : ", afficheurOk ? "OK" : "ECHEC");
        DEBUG_VAL("gestionnaireTemps->initialiser() : ", gestionnaireTempsOk ? "OK" : "ECHEC");
        DEBUG("Erreur initialisation composants");
    }

    return resultat;
}

// ============================================================
// Automate principal
// ============================================================

/**
 * @brief Dispatch principal de l'automate : exécute la méthode de
 *        traitement correspondant à l'état courant (etatCourant). Chaque
 *        traitement est responsable de faire progresser l'automate vers
 *        l'état suivant.
 */
void BoitierPilotageArrosage::controler()
{
    switch (etatCourant)
    {
    case EtatBoitier::DEMARRAGE:
        traiterDemarrage();
        break;

    case EtatBoitier::DETERMINER_REVEIL:
        traiterDeterminerReveil();
        break;

    case EtatBoitier::RESTAURER_ETAT_VANNES:
        traiterRestaurerEtatVannes();
        break;

    case EtatBoitier::ARROSAGE:
        traiterArrosage();
        break;

    case EtatBoitier::ATTENTE_CONNEXION:
        traiterAttenteConnexion();
        break;

    case EtatBoitier::DIALOGUE:
        traiterDialogue();
        break;

    case EtatBoitier::SOMMEIL_LEGER:
        traiterLightSleep();
        break;

    case EtatBoitier::ENDORMISSEMENT:
        traiterEndormissement();
        break;
    }
}

// ============================================================
// États de l'automate
// ============================================================

/**
 * @brief Traitement de l'état DEMARRAGE.
 * @details Réinitialise l'affichage (mode WiFi, IP, client connecté et état
 *          des 4 vannes à des valeurs neutres), affiche l'heure RTC courante,
 *          puis bascule systématiquement vers DETERMINER_REVEIL.
 */
void BoitierPilotageArrosage::traiterDemarrage()
{
    DEBUG("=== DEMARRAGE ===");
    DEBUG_VAL("Heure RTC : ", gestionnaireTemps->lireHeureSysteme());

    afficheur->setModeWifi("---");
    afficheur->setAdresseIP("---");
    afficheur->setClientConnecte(false);

    bool etatsInitiaux[4] = {false, false, false, false};
    afficheur->setEtatVannes(etatsInitiaux, 4);
    afficheur->setHeureSysteme(gestionnaireTemps->lireHeureSysteme());
    afficheur->rafraichir();

    etatCourant = EtatBoitier::DETERMINER_REVEIL;
}

/**
 * @brief Traitement de l'état DETERMINER_REVEIL.
 * @details Détermine la cause du réveil via determinerCauseReveil() et
 *          journalise le résultat. Sur CauseReveil::ALARME_RTC, efface
 *          l'alarme du DS3231 avant de poursuivre. Sur
 *          CauseReveil::LIGHT_SLEEP_TIMER, saute directement l'étape de
 *          restauration d'état pour repartir en ARROSAGE (l'état des vannes
 *          n'a pas pu changer pendant un simple light sleep). Dans tous les
 *          autres cas (RTC, BP1, premier démarrage, cause inconnue),
 *          bascule vers RESTAURER_ETAT_VANNES ; réarme au passage
 *          dernierMessageRecuMs pour les réveils autres que RTC afin
 *          d'initialiser le timeout d'attente de connexion.
 */
void BoitierPilotageArrosage::traiterDeterminerReveil()
{
    causeReveil = determinerCauseReveil();

    switch (causeReveil)
    {
    case CauseReveil::ALARME_RTC:
        DEBUG("Reveil : ALARME RTC");
        gestionnaireTemps->effacerAlarme();
        etatCourant = EtatBoitier::RESTAURER_ETAT_VANNES;
        break;

    case CauseReveil::LIGHT_SLEEP_TIMER:
        DEBUG("Reveil : LIGHT SLEEP TIMER");
        etatCourant = EtatBoitier::ARROSAGE;
        break;

    case CauseReveil::BOUTON_POUSSOIR:
        DEBUG("Reveil : BOUTON POUSSOIR");
        dernierMessageRecuMs = millis();
        etatCourant = EtatBoitier::RESTAURER_ETAT_VANNES;
        break;

    case CauseReveil::PREMIER_DEMARRAGE:
        DEBUG("Reveil : PREMIER DEMARRAGE");
        dernierMessageRecuMs = millis();
        etatCourant = EtatBoitier::RESTAURER_ETAT_VANNES;
        break;

    case CauseReveil::INCONNUE:
        DEBUG("Reveil : CAUSE INCONNUE");
        dernierMessageRecuMs = millis();
        etatCourant = EtatBoitier::RESTAURER_ETAT_VANNES;
        break;
    }
}

/**
 * @brief Traitement de l'état RESTAURER_ETAT_VANNES.
 * @details Parcourt les 4 vannes et relit leur état persisté en NVS
 *          (StockageEtatVannes). Pour toute vanne trouvée ouverte :
 *          - si le réveil correspond à un premier démarrage (coupure
 *            secteur potentielle) ou si le délai d'arrosage programmé est
 *            dépassé (StockageEtatVannes::delaiDepasse()), la vanne est
 *            refermée par sécurité (impulsion bloquante avec timeout de
 *            2 s) et l'état NVS mis à jour ;
 *          - sinon, l'état logique "ouverte" est simplement restauré en
 *            mémoire (reprise d'un arrosage en cours après un simple
 *            réveil).
 *
 *          Une fois l'affichage rafraîchi, oriente l'automate :
 *          - vers ATTENTE_CONNEXION si toutes les vannes sont fermées et
 *            que le réveil provient du bouton poussoir ou d'un premier
 *            démarrage ;
 *          - vers ARROSAGE si toutes les vannes sont fermées mais que le
 *            réveil provient d'une alarme RTC (vérification de la
 *            programmation nécessaire, y compris pour une ouverture qui
 *            doit démarrer maintenant) ;
 *          - vers ENDORMISSEMENT pour les autres causes ;
 *          - vers ARROSAGE si au moins une vanne est restée ouverte (reprise
 *            du suivi d'un arrosage en cours).
 */
void BoitierPilotageArrosage::traiterRestaurerEtatVannes()
{
    DEBUG("=== RESTAURATION ETAT VANNES ===");
    String heureActuelle = gestionnaireTemps->lireHeureSysteme();
    uint8_t idVanne = 0;
    while (idVanne < 4)
    {
        EtatVanne etat = stockageEtat->lireEtat(idVanne + 1);
        if (etat.ouverte)
        {
            ProgrammationVanne prog = stockage->lireProgrammation(idVanne + 1);
            bool coupureSecteur = (causeReveil == CauseReveil::PREMIER_DEMARRAGE);
            bool delaiDepasse = stockageEtat->delaiDepasse(
                idVanne + 1, prog.duree, heureActuelle);
            if (coupureSecteur || delaiDepasse)
            {
                DEBUG_VAL("Fermeture securite vanne ", idVanne + 1);
                vannes[idVanne]->setEtatLogique(true);
                vannes[idVanne]->fermer();
                uint32_t debut = millis();
                while (vannes[idVanne]->estEnCours() && millis() - debut < 2000)
                {
                    vannes[idVanne]->update();
                    delay(10);
                }
                sauvegarderFermetureVanne(idVanne + 1);
            }
            else
            {
                DEBUG_VAL("Reprise arrosage vanne ", idVanne + 1);
                vannes[idVanne]->setEtatLogique(true);
            }
        }
        else
        {
            DEBUG_VAL("Vanne fermee (NVS) : ", idVanne + 1);
        }
        idVanne++;
    }
    mettreAJourAffichageVannes();
    if (toutesVannesFermees())
    {
        if (causeReveil == CauseReveil::BOUTON_POUSSOIR ||
            causeReveil == CauseReveil::PREMIER_DEMARRAGE)
        {
            DEBUG("Toutes vannes fermees → ATTENTE CONNEXION");
            dernierMessageRecuMs = millis();
            etatCourant = EtatBoitier::ATTENTE_CONNEXION;
        }
        else if (causeReveil == CauseReveil::ALARME_RTC)
        {
            DEBUG("Reveil alarme RTC → verification programmation (ARROSAGE)");
            etatCourant = EtatBoitier::ARROSAGE;
        }
        else
        {
            DEBUG("Toutes vannes fermees → ENDORMISSEMENT");
            etatCourant = EtatBoitier::ENDORMISSEMENT;
        }
    }
    else
    {
        DEBUG("Vannes ouvertes → ARROSAGE");
        etatCourant = EtatBoitier::ARROSAGE;
    }
}

/**
 * @brief Traitement de l'état ARROSAGE.
 * @details Met à jour l'état de l'impulsion en cours de chaque vanne
 *          (Vanne::update()), applique la programmation horaire via
 *          verifierProgrammations() (ouverture/fermeture selon l'heure
 *          système), rafraîchit l'affichage, puis bascule vers
 *          ENDORMISSEMENT si toutes les vannes sont fermées, ou vers
 *          LIGHT_SLEEP si au moins une reste ouverte (poursuite de
 *          l'arrosage avec vérifications périodiques).
 */
void BoitierPilotageArrosage::traiterArrosage()
{
    uint8_t idVanne = 0;
    while (idVanne < 4)
    {
        vannes[idVanne]->update();
        idVanne++;
    }

    verifierProgrammations();

    mettreAJourAffichageVannes();

    if (toutesVannesFermees())
    {
        DEBUG("Toutes vannes fermees → ENDORMISSEMENT");
        etatCourant = EtatBoitier::ENDORMISSEMENT;
    }
    else
    {
        DEBUG("Vannes ouvertes → LIGHT SLEEP");
        etatCourant = EtatBoitier::SOMMEIL_LEGER;
    }
}

/**
 * @brief Traitement de l'état ATTENTE_CONNEXION.
 * @details Démarre la connexion WiFi STA (avec repli en point d'accès AP en
 *          cas d'échec) une seule fois par cycle d'attente, grâce au drapeau
 *          wifiDemarre qui évite de relancer demarrerSTA() à chaque appel.
 *          Met à jour l'affichage avec le mode WiFi et l'adresse IP obtenue.
 *          Bascule ensuite vers ENDORMISSEMENT si aucun client ne s'est
 *          connecté avant expiration de DELAI_ATTENTE_CONNEXION_MS, ou vers
 *          DIALOGUE dès qu'un client WebSocket se connecte (réinitialise au
 *          passage le compteur d'inactivité et allume l'affichage).
 */
void BoitierPilotageArrosage::traiterAttenteConnexion()
{
    verifierAffichageBatterie();

    if (!serveurWebSocket->clientConnecte() && !wifiDemarre)
    {
        wifiDemarre = true;
        serveurWebSocket->demarrerSTA(SSID_STA, PASSWORD_STA);

        if (WiFi.status() == WL_CONNECTED)
        {
            DEBUG_VAL("WiFi connecte en STA — IP : ", WiFi.localIP());
        }
        else
        {
            DEBUG("WiFi STA echoue — bascule en AP");
            DEBUG_VAL("IP AP : ", WiFi.softAPIP());
        }

        afficheur->setClientConnecte(false);
        if (!affichageBatterieActif)
            afficheur->rafraichir();
    }

    if (!serveurWebSocket->clientConnecte() &&
        millis() - dernierMessageRecuMs > DELAI_ATTENTE_CONNEXION_MS)
    {
        DEBUG("Timeout connexion → ENDORMISSEMENT");
        wifiDemarre = false;
        etatCourant = EtatBoitier::ENDORMISSEMENT;
    }
    else if (serveurWebSocket->clientConnecte())
    {
        DEBUG("Client WebSocket connecte → DIALOGUE");
        dernierMessageRecuMs = millis();
        wifiDemarre = false;
        affichageBatterieActif = false;
        afficheur->allumer();
        afficheur->setClientConnecte(true);
        mettreAJourAffichageVannes();
        afficheur->rafraichir();
        etatCourant = EtatBoitier::DIALOGUE;
    }
}

/**
 * @brief Traitement de l'état DIALOGUE.
 * @details Exécuté à chaque tour de loop() tant qu'un client WebSocket est
 *          connecté. Effectue, dans l'ordre :
 *          - la mise à jour des impulsions de vannes en cours ;
 *          - le rafraîchissement périodique de l'heure affichée sur l'OLED
 *            (toutes les DELAI_RAFRAICHISSEMENT_OLED_MS) ;
 *          - la vérification périodique de la programmation des vannes
 *            (toutes les DELAI_VERIFICATION_PROGRAMMATION_MS, via
 *            verifierProgrammations()), afin que le mode Programme se
 *            déclenche même si un client reste connecté ;
 *          - la détection d'une connexion fantôme : au-delà de
 *            DELAI_DECONNEXION_MS sans message reçu, envoie une notification
 *            de veille au client, éteint l'affichage et bascule vers
 *            ENDORMISSEMENT ;
 *          - la détection d'une déconnexion normale du client : éteint
 *            l'affichage et bascule vers SOMMEIL_LEGER si une vanne reste
 *            ouverte, ou vers ATTENTE_CONNEXION (sans relancer le WiFi déjà
 *            actif) si toutes les vannes sont fermées.
 */
void BoitierPilotageArrosage::traiterDialogue()
{
    // Mise à jour des impulsions en cours
    uint8_t idVanne = 0;
    while (idVanne < 4)
    {
        vannes[idVanne]->update();
        idVanne++;
    }

    verifierAffichageBatterie();

    // ── Rafraîchissement périodique de l'heure sur l'OLED ────────────────────
    uint32_t maintenant = millis();
    if (!affichageBatterieActif && maintenant - dernierRafraichissementOledMs >= DELAI_RAFRAICHISSEMENT_OLED_MS)
    {
        dernierRafraichissementOledMs = maintenant;
        afficheur->setHeureSysteme(gestionnaireTemps->lireHeureSysteme());
        afficheur->rafraichir();
    }

    // ── Verification programmation (mode Programme actif meme connecte) ──────
    if (maintenant - dernierVerificationProgMs >= DELAI_VERIFICATION_PROGRAMMATION_MS)
    {
        dernierVerificationProgMs = maintenant;
        verifierProgrammations();
        if (!affichageBatterieActif)
            mettreAJourAffichageVannes();
    }

    uint32_t inactiviteMs = millis() - dernierMessageRecuMs;

    // Timeout de sécurité : connexion fantôme → déconnexion propre
    if (inactiviteMs > DELAI_DECONNEXION_MS)
    {
        DEBUG("Timeout inactivite → notification veille + endormissement");
        serveurWebSocket->envoyerMessage(protocole->creerNotifVeille());
        delay(200);

        // Éteint l'écran car on va dormir
        affichageBatterieActif = false;
        afficheur->setClientConnecte(false);
        afficheur->rafraichir();
        delay(500);
        afficheur->eteindre();

        etatCourant = EtatBoitier::ENDORMISSEMENT;
    }

    // Déconnexion normale du client → écran éteint
    if (!serveurWebSocket->clientConnecte())
    {
        DEBUG("Client deconnecte → ecran eteint");
        affichageBatterieActif = false;
        mettreAJourAffichageVannes();
        afficheur->setClientConnecte(false);
        afficheur->rafraichir();
        delay(500);
        afficheur->eteindre();

        if (!toutesVannesFermees())
        {
            etatCourant = EtatBoitier::SOMMEIL_LEGER;
        }
        else
        {
            DEBUG("Vannes fermees → attente reconnexion avant endormissement");
            dernierMessageRecuMs = millis();
            wifiDemarre = true; // le WiFi STA est deja actif, pas besoin de le relancer
            etatCourant = EtatBoitier::ATTENTE_CONNEXION;
        }
    }
}

/**
 * @brief Traitement de l'état LIGHT_SLEEP.
 * @details Éteint l'affichage puis configure deux sources de réveil pour
 *          la veille légère : un timer (DUREE_LIGHT_SLEEP_MS) pour revenir
 *          périodiquement vérifier l'état des vannes, et le bouton BP1
 *          (EXT1) pour permettre une reprise en main immédiate. Entre en
 *          light sleep (RAM conservée, exécution reprend juste après
 *          l'appel). Au réveil, bascule vers ATTENTE_CONNEXION si le réveil
 *          provient de BP1 (l'utilisateur souhaite reprendre la main), ou
 *          vers ARROSAGE si le réveil provient du timer (poursuite normale
 *          du suivi de l'arrosage en cours).
 */
void BoitierPilotageArrosage::traiterLightSleep()
{
    DEBUG("Entree en light sleep (vannes ouvertes)");
    afficheur->eteindre();

    esp_sleep_enable_timer_wakeup((uint64_t)DUREE_LIGHT_SLEEP_MS * 1000ULL);

    uint64_t masqueBP1 = (1ULL << BROCHE_BP1);
    esp_sleep_enable_ext1_wakeup(masqueBP1, ESP_EXT1_WAKEUP_ALL_LOW);

    esp_light_sleep_start();

    DEBUG("Reveil light sleep");

    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    if (wakeupCause == ESP_SLEEP_WAKEUP_EXT1)
    {
        DEBUG("BP1 appuye pendant arrosage → ATTENTE CONNEXION");
        dernierMessageRecuMs = millis();
        afficheur->allumer();
        etatCourant = EtatBoitier::ATTENTE_CONNEXION;
    }
    else
    {
        DEBUG("Timer → reprise ARROSAGE");
        etatCourant = EtatBoitier::ARROSAGE;
    }
}

/**
 * @brief Traitement de l'état ENDORMISSEMENT.
 * @details Parcourt la programmation des 4 vannes et calcule, pour chacune,
 *          la prochaine occurrence d'ouverture (GestionnaireTemps::
 *          calculerProchaineAlarme()) ainsi que l'heure de fermeture
 *          correspondante (ouverture + durée). Retient la plus proche des
 *          échéances (ouverture ou fermeture, toutes vannes confondues)
 *          comme prochaine alarme à programmer dans le DS3231. Si au moins
 *          une programmation est active, programme l'alarme RTC
 *          correspondante et l'affiche brièvement ; sinon, le boîtier
 *          s'endort sans alarme RTC (seul BP1 pourra le réveiller). Ferme
 *          ensuite explicitement toute connexion WebSocket
 *          (ServeurWebSocket::fermerConnexions()) et laisse un court délai
 *          pour que la trame de fermeture parte effectivement, afin que
 *          l'application Qt affiche aussitôt le statut déconnecté plutôt que
 *          de découvrir la coupure au bout de son propre timeout. Éteint
 *          enfin l'affichage, configure les sources de réveil matérielles
 *          (configurerSourcesReveil()) et entre en deep sleep
 *          (esp_deep_sleep_start() ne retourne jamais : le prochain code
 *          exécuté sera setup() après redémarrage).
 */
void BoitierPilotageArrosage::traiterEndormissement()
{
    DEBUG("=== ENDORMISSEMENT ===");

    String prochaineAlarme = "";

    uint8_t idVanne = 0;
    while (idVanne < 4)
    {
        ProgrammationVanne prog = stockage->lireProgrammation(idVanne + 1);

        String alarmeOuverture = gestionnaireTemps->calculerProchaineAlarme(
            prog.mode, prog.heure, prog.frequence);

        if (alarmeOuverture.length() > 0)
        {
            String alarmeFermeture = gestionnaireTemps->ajouterMinutes(
                alarmeOuverture, prog.duree);

            DEBUG_VAL("Alarme ouverture vanne ", idVanne + 1);
            DEBUG_VAL("  -> ", alarmeOuverture);
            DEBUG_VAL("  -> fermeture : ", alarmeFermeture);

            if (prochaineAlarme.isEmpty() || alarmeOuverture < prochaineAlarme)
                prochaineAlarme = alarmeOuverture;

            if (!prochaineAlarme.isEmpty() && alarmeFermeture < prochaineAlarme)
                prochaineAlarme = alarmeFermeture;
        }

        idVanne++;
    }

    if (prochaineAlarme.length() > 0)
    {
        DEBUG_VAL("Prochaine alarme programmee : ", prochaineAlarme);
        gestionnaireTemps->programmerAlarme(prochaineAlarme);
        afficheur->setHeureSysteme(prochaineAlarme);
        afficheur->rafraichir();
        delay(1000);
    }
    else
    {
        DEBUG("Aucune programmation active — sommeil sans alarme RTC");
    }

    DEBUG("Entree en deep sleep");
    serveurWebSocket->fermerConnexions();
    delay(200); // laisse le temps a la frame de fermeture de partir

    afficheur->eteindre();
    configurerSourcesReveil();
    esp_deep_sleep_start();
}

// ============================================================
// Traitement des messages WebSocket
// ============================================================

/**
 * @brief Callback de réception d'un message WebSocket.
 * @details Réarme immédiatement dernierMessageRecuMs (utilisé pour détecter
 *          les timeouts d'inactivité et de connexion), journalise la trame
 *          brute reçue, la décode via ProtocoleArrosageServeur::decoder()
 *          puis délègue son traitement à traiterRequete().
 * @param _message Trame JSON brute reçue du client WebSocket.
 */
void BoitierPilotageArrosage::onMessageRecu(const String &_message)
{
    dernierMessageRecuMs = millis();
    DEBUG_VAL("Message recu : ", _message);

    RequeteArrosage requete = protocole->decoder(_message);
    traiterRequete(requete);
}

/**
 * @brief Traite une requête applicative décodée et produit la réponse
 *        WebSocket correspondante.
 * @details Rejette immédiatement une requête invalide (échec de parsing ou
 *          de version) par une trame d'erreur générique. Sinon, dispatch
 *          sur le caractère de commande :
 *          - CMD_PING : répond par un accusé "pong" ;
 *          - CMD_GET_TIME / CMD_SET_TIME : lit ou resynchronise l'heure
 *            système du RTC (en forçant la mise à jour si l'année RTC est
 *            manifestement invalide, ou en journalisant une dérive
 *            détectée) ;
 *          - CMD_GET_SYSTEM : construit et renvoie l'état complet du
 *            système (heure + état/mode/programmation des 4 vannes) ;
 *          - CMD_GET_ETAT / CMD_GET_MODE / CMD_GET_PROG : renvoient
 *            respectivement l'état, le mode ou la programmation d'une
 *            vanne ;
 *          - CMD_SET_MODE : valide et enregistre le nouveau mode d'une
 *            vanne (Manuel, Programme ou Automatique) ;
 *          - CMD_SET_PROG : valide (heure, durée et fréquence non nulles)
 *            et enregistre une nouvelle programmation pour une vanne ;
 *          - CMD_OUVRIR / CMD_FERMER : forcent le mode de la vanne à Manuel
 *            (suspend toute programmation en cours), déclenchent
 *            l'impulsion physique correspondante, persistent le nouvel état
 *            en NVS, mettent à jour l'affichage avec l'état cible et
 *            envoient un accusé de réception ;
 *          - toute autre commande : renvoie une erreur "commande inconnue".
 *
 *          Pour les commandes portant sur une vanne, une erreur
 *          ERREUR_VANNE_INEXISTANTE est renvoyée si l'identifiant ne
 *          correspond à aucune des 4 vannes.
 * @param _requete Requête décodée par ProtocoleArrosageServeur::decoder().
 */
void BoitierPilotageArrosage::traiterRequete(const RequeteArrosage &_requete)
{
    if (!_requete.valide)
    {
        DEBUG("Requete invalide");
        serveurWebSocket->envoyerMessage(
            protocole->creerErreur('?', ProtocoleArrosageServeur::ERREUR_AUCUNE));
        return;
    }

    char commande = _requete.commande[0];
    DEBUG_VAL("Commande : ", String(commande));

    switch (commande)
    {
    case ProtocoleArrosageServeur::CMD_PING:
    {
        DEBUG("CMD : PING");
        serveurWebSocket->envoyerMessage(protocole->creerReponsePing());
        break;
    }

    case ProtocoleArrosageServeur::CMD_GET_TIME:
    {
        String heure = gestionnaireTemps->lireHeureSysteme();
        DEBUG_VAL("CMD : GET_TIME → ", heure);
        serveurWebSocket->envoyerMessage(protocole->creerReponseHeure(heure));
        break;
    }

    case ProtocoleArrosageServeur::CMD_SET_TIME:
    {
        DEBUG_VAL("CMD : SET_TIME → ", _requete.heure);
        if (_requete.heure.length() > 0)
        {
            String heureRTC = gestionnaireTemps->lireHeureSysteme();
            String heureClient = _requete.heure;

            int anneeRTC = heureRTC.substring(0, 4).toInt();
            int anneeClient = heureClient.substring(0, 4).toInt();

            bool rtcInvalide = (anneeRTC < 2026);
            bool ecartImportant = (heureRTC.substring(0, 10) != heureClient.substring(0, 10));

            if (rtcInvalide)
            {
                DEBUG("RTC invalide → mise a jour forcee");
                gestionnaireTemps->ecrireHeureSysteme(heureClient);
            }
            else if (ecartImportant)
            {
                DEBUG_VAL("Derive RTC detectee — ancienne heure : ", heureRTC);
                DEBUG_VAL("Nouvelle heure client : ", heureClient);
                gestionnaireTemps->ecrireHeureSysteme(heureClient);
                DEBUG("RTC resynchronisee");
            }
            else
            {
                gestionnaireTemps->ecrireHeureSysteme(heureClient);
                DEBUG("RTC synchronisee (derive mineure)");
            }

            afficheur->setHeureSysteme(heureClient);
            afficheur->rafraichir();
            serveurWebSocket->envoyerMessage(
                protocole->creerAck(ProtocoleArrosageServeur::CMD_SET_TIME));
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_SET_TIME,
                    ProtocoleArrosageServeur::ERREUR_RTC_INDISPONIBLE));
        }
        break;
    }
    case ProtocoleArrosageServeur::CMD_GET_SYSTEM:
    {
        DEBUG("CMD : GET_SYSTEM");
        String heureSysteme = gestionnaireTemps->lireHeureSysteme();

        JsonDocument docVannes;
        JsonArray tableauVannes = docVannes.to<JsonArray>();

        uint8_t idVanne = 0;
        while (idVanne < 4)
        {
            ProgrammationVanne prog = stockage->lireProgrammation(idVanne + 1);

            JsonObject objVanne = tableauVannes.add<JsonObject>();
            objVanne["i"] = idVanne + 1;
            objVanne["e"] = String(vannes[idVanne]->estOuverte()
                                       ? ProtocoleArrosageServeur::ETAT_OUVERTE
                                       : ProtocoleArrosageServeur::ETAT_FERMEE);
            objVanne["m"] = String(prog.mode);
            objVanne["h"] = prog.heure;
            objVanne["d"] = prog.duree;
            objVanne["f"] = prog.frequence;

            idVanne++;
        }

        String reponse = protocole->creerReponseSysteme(heureSysteme, tableauVannes);
        DEBUG_VAL("  -> reponse envoyee : ", reponse);
        serveurWebSocket->envoyerMessage(reponse);
        break;
    }

    case ProtocoleArrosageServeur::CMD_GET_ETAT:
    {
        DEBUG_VAL("CMD : GET_ETAT vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            char etat = vanne->estOuverte()
                            ? ProtocoleArrosageServeur::ETAT_OUVERTE
                            : ProtocoleArrosageServeur::ETAT_FERMEE;
            DEBUG_VAL("  -> etat : ", String(etat));
            serveurWebSocket->envoyerMessage(
                protocole->creerReponseEtat(_requete.idVanne, etat));
        }
        else
        {
            DEBUG("  -> vanne inexistante");
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_GET_ETAT,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_GET_MODE:
    {
        DEBUG_VAL("CMD : GET_MODE vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            char mode = stockage->lireMode(_requete.idVanne);
            DEBUG_VAL("  -> mode : ", String(mode));
            serveurWebSocket->envoyerMessage(
                protocole->creerReponseMode(_requete.idVanne, mode));
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_GET_MODE,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_SET_MODE:
    {
        DEBUG_VAL("CMD : SET_MODE vanne ", _requete.idVanne);
        DEBUG_VAL("  -> mode : ", _requete.mode);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            if (_requete.mode == String(ProtocoleArrosageServeur::MODE_MANUEL) ||
                _requete.mode == String(ProtocoleArrosageServeur::MODE_PROGRAMME) ||
                _requete.mode == String(ProtocoleArrosageServeur::MODE_AUTOMATIQUE))
            {
                stockage->ecrireMode(_requete.idVanne, _requete.mode[0]);
                DEBUG("  -> mode enregistre");
                serveurWebSocket->envoyerMessage(
                    protocole->creerAckVanne(
                        ProtocoleArrosageServeur::CMD_SET_MODE,
                        _requete.idVanne));
            }
            else
            {
                DEBUG("  -> mode invalide");
                serveurWebSocket->envoyerMessage(
                    protocole->creerErreur(
                        ProtocoleArrosageServeur::CMD_SET_MODE,
                        ProtocoleArrosageServeur::ERREUR_MODE_INVALIDE));
            }
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_SET_MODE,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_GET_PROG:
    {
        DEBUG_VAL("CMD : GET_PROG vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            ProgrammationVanne prog = stockage->lireProgrammation(_requete.idVanne);
            DEBUG_VAL("  -> heure : ", prog.heure);
            DEBUG_VAL("  -> duree : ", prog.duree);
            DEBUG_VAL("  -> freq  : ", prog.frequence);
            serveurWebSocket->envoyerMessage(
                protocole->creerReponseProgrammation(
                    _requete.idVanne, prog.heure, prog.duree, prog.frequence));
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_GET_PROG,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_SET_PROG:
    {
        DEBUG_VAL("CMD : SET_PROG vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            if (_requete.heure.length() > 0 &&
                _requete.duree > 0 &&
                _requete.frequence > 0)
            {
                ProgrammationVanne prog;
                prog.mode = stockage->lireMode(_requete.idVanne);
                prog.heure = _requete.heure;
                prog.duree = _requete.duree;
                prog.frequence = _requete.frequence;

                stockage->ecrireProgrammation(_requete.idVanne, prog);
                DEBUG("  -> programmation enregistree");
                serveurWebSocket->envoyerMessage(
                    protocole->creerAckVanne(
                        ProtocoleArrosageServeur::CMD_SET_PROG,
                        _requete.idVanne));
            }
            else
            {
                DEBUG("  -> programmation invalide");
                serveurWebSocket->envoyerMessage(
                    protocole->creerErreur(
                        ProtocoleArrosageServeur::CMD_SET_PROG,
                        ProtocoleArrosageServeur::ERREUR_PROG_INVALIDE));
            }
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_SET_PROG,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_OUVRIR:
    {
        DEBUG_VAL("CMD : OUVRIR vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            // Force le mode Manuel — suspend la programmation
            stockage->ecrireMode(_requete.idVanne, ProtocoleArrosageServeur::MODE_MANUEL);
            DEBUG_VAL("  -> mode force a M pour vanne ", _requete.idVanne);
            // Notifie l'application du changement de mode forcé, avant
            // même que l'impulsion ne soit terminée.
            serveurWebSocket->envoyerMessage(
                protocole->creerNotifMode(_requete.idVanne, ProtocoleArrosageServeur::MODE_MANUEL));

            vanne->ouvrir();
            // Attente active bornée (2 s max) de la fin de l'impulsion
            // physique, en appelant update() à intervalles réguliers, afin
            // que sauvegarderOuvertureVanne() et l'accusé de réception
            // reflètent l'état réellement atteint par la vanne plutôt que
            // l'état visé.
            uint32_t debutImpulsion = millis();
            while (vanne->estEnCours() && millis() - debutImpulsion < 2000)
            {
                vanne->update();
                delay(10);
            }
            sauvegarderOuvertureVanne(_requete.idVanne);
            mettreAJourAffichageVannesAvecCible(_requete.idVanne, true);
            DEBUG("  -> impulsion ouverture envoyee");
            serveurWebSocket->envoyerMessage(
                protocole->creerAckVanne(
                    ProtocoleArrosageServeur::CMD_OUVRIR,
                    _requete.idVanne));
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_OUVRIR,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    case ProtocoleArrosageServeur::CMD_FERMER:
    {
        DEBUG_VAL("CMD : FERMER vanne ", _requete.idVanne);
        Vanne *vanne = vanneParId(_requete.idVanne);
        if (vanne != nullptr)
        {
            // Force le mode Manuel — suspend la programmation
            stockage->ecrireMode(_requete.idVanne, ProtocoleArrosageServeur::MODE_MANUEL);
            DEBUG_VAL("  -> mode force a M pour vanne ", _requete.idVanne);
            // Notifie l'application du changement de mode forcé, avant
            // même que l'impulsion ne soit terminée.
            serveurWebSocket->envoyerMessage(
                protocole->creerNotifMode(_requete.idVanne,
                                          ProtocoleArrosageServeur::MODE_MANUEL));

            vanne->fermer();

            // Attente active bornée (2 s max) de la fin de l'impulsion
            // physique, symétrique du traitement de CMD_OUVRIR.
            uint32_t debutImpulsion = millis();
            while (vanne->estEnCours() && millis() - debutImpulsion < 2000)
            {
                vanne->update();
                delay(10);
            }

            sauvegarderFermetureVanne(_requete.idVanne);
            mettreAJourAffichageVannesAvecCible(_requete.idVanne, false);
            DEBUG("  -> impulsion fermeture envoyee");
            serveurWebSocket->envoyerMessage(
                protocole->creerAckVanne(
                    ProtocoleArrosageServeur::CMD_FERMER,
                    _requete.idVanne));
        }
        else
        {
            serveurWebSocket->envoyerMessage(
                protocole->creerErreur(
                    ProtocoleArrosageServeur::CMD_FERMER,
                    ProtocoleArrosageServeur::ERREUR_VANNE_INEXISTANTE));
        }
        break;
    }

    default:
    {
        DEBUG_VAL("Commande inconnue : ", String(commande));
        serveurWebSocket->envoyerMessage(
            protocole->creerErreur(commande,
                                   ProtocoleArrosageServeur::ERREUR_AUCUNE));
        break;
    }
    }
}

// ============================================================
// Réveil
// ============================================================

/**
 * @brief Détermine la cause du réveil de l'ESP32.
 * @details Interroge esp_sleep_get_wakeup_cause() et traduit le résultat en
 *          ::CauseReveil : EXT0 → alarme RTC, EXT1 → bouton poussoir,
 *          TIMER → réveil périodique du light sleep, UNDEFINED → premier
 *          démarrage (mise sous tension ou reset). Toute autre valeur est
 *          rapportée comme CauseReveil::INCONNUE.
 * @return La cause de réveil identifiée.
 */
CauseReveil BoitierPilotageArrosage::determinerCauseReveil() const
{
    CauseReveil cause = CauseReveil::INCONNUE;
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    if (wakeupCause == ESP_SLEEP_WAKEUP_EXT0)
        cause = CauseReveil::ALARME_RTC;
    else if (wakeupCause == ESP_SLEEP_WAKEUP_EXT1)
        cause = CauseReveil::BOUTON_POUSSOIR;
    else if (wakeupCause == ESP_SLEEP_WAKEUP_TIMER)
        cause = CauseReveil::LIGHT_SLEEP_TIMER;
    else if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED)
        cause = CauseReveil::PREMIER_DEMARRAGE;

    return cause;
}

/**
 * @brief Configure les sources de réveil matérielles avant l'entrée en deep
 *        sleep.
 * @details Purge d'abord toutes les sources de réveil précédemment
 *          configurées (esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL)),
 *          afin d'éliminer un éventuel timer résiduel laissé actif par un
 *          cycle de light sleep précédent (qui provoquerait un réveil
 *          parasite prématuré). Active ensuite EXT0 sur BROCHE_IRQ_RTC
 *          (déclenchement à l'état bas, alarme DS3231) et EXT1 sur
 *          BROCHE_BP1 (réveil par bouton poussoir).
 */
void BoitierPilotageArrosage::configurerSourcesReveil()
{
   esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); // purge toutes les sources (dont le timer résiduel du light sleep)

    esp_sleep_enable_ext0_wakeup((gpio_num_t)BROCHE_IRQ_RTC, LOW);

    uint64_t masqueBP1 = (1ULL << BROCHE_BP1);
    esp_sleep_enable_ext1_wakeup(masqueBP1, ESP_EXT1_WAKEUP_ALL_LOW); 
}

// ============================================================
// Utilitaires
// ============================================================

/**
 * @brief Retrouve le pointeur de vanne correspondant à un identifiant.
 * @param _idVanne Identifiant de vanne attendu, de 1 à 4.
 * @return Pointeur vers la Vanne correspondante, ou nullptr si
 *         l'identifiant est hors de la plage [1, 4].
 */
Vanne *BoitierPilotageArrosage::vanneParId(int _idVanne) const
{
    Vanne *vanne = nullptr;

    if (_idVanne >= 1 && _idVanne <= 4)
        vanne = vannes[_idVanne - 1];

    return vanne;
}

/**
 * @brief Indique si les 4 vannes sont fermées et sans impulsion en cours.
 * @return true si aucune des 4 vannes n'est ouverte ni en cours de
 *         manœuvre (estOuverte() et estEnCours() faux pour toutes) ;
 *         false dès qu'une vanne est ouverte ou en cours de manœuvre.
 */
bool BoitierPilotageArrosage::toutesVannesFermees() const
{
    bool resultat = true;

    uint8_t idVanne = 0;
    while (idVanne < 4 && resultat)
    {
        if (vannes[idVanne]->estOuverte() || vannes[idVanne]->estEnCours())
            resultat = false;

        idVanne++;
    }

    return resultat;
}

/**
 * @brief Rafraîchit l'affichage OLED avec l'état réel courant.
 * @details Relit l'état physique (estOuverte()) des 4 vannes ainsi que
 *          l'heure système, les transmet à l'AfficheurOLED puis déclenche
 *          le rafraîchissement effectif de l'écran.
 */
void BoitierPilotageArrosage::mettreAJourAffichageVannes()
{
    bool etats[4];
    uint8_t idVanne = 0;

    while (idVanne < 4)
    {
        etats[idVanne] = vannes[idVanne]->estOuverte();
        idVanne++;
    }

    afficheur->setEtatVannes(etats, 4);
    afficheur->setHeureSysteme(gestionnaireTemps->lireHeureSysteme());
    afficheur->rafraichir();
}

/**
 * @brief Rafraîchit l'affichage OLED en forçant l'état affiché d'une vanne
 *        à une valeur cible.
 * @details Utilisée juste après le déclenchement d'une impulsion manuelle
 *          (ouverture/fermeture), pendant que l'état physique réel
 *          (Vanne::estOuverte()) n'a pas encore basculé (temps de
 *          l'impulsion bistable) : affiche directement l'état visé pour la
 *          vanne actionnée, et l'état réel courant pour les 3 autres.
 * @param _idVanneCible Identifiant (1 à 4) de la vanne dont l'état affiché
 *                       est forcé.
 * @param _etatCible    État à afficher pour cette vanne (true = ouverte,
 *                       false = fermée).
 */
void BoitierPilotageArrosage::mettreAJourAffichageVannesAvecCible(
    uint8_t _idVanneCible, bool _etatCible)
{
    bool etats[4];
    uint8_t idVanne = 0;

    while (idVanne < 4)
    {
        if (idVanne == _idVanneCible - 1)
            etats[idVanne] = _etatCible; // état cible pour la vanne actionnée
        else
            etats[idVanne] = vannes[idVanne]->estOuverte(); // état réel pour les autres

        idVanne++;
    }

    afficheur->setEtatVannes(etats, 4);
    afficheur->setHeureSysteme(gestionnaireTemps->lireHeureSysteme());
    afficheur->rafraichir();
}

/**
 * @brief Persiste en NVS l'ouverture d'une vanne.
 * @details Lit l'heure système courante et délègue la sauvegarde (état +
 *          horodatage d'ouverture) à StockageEtatVannes::sauvegarderOuverture().
 * @param _idVanne Identifiant de la vanne concernée (1 à 4).
 */
void BoitierPilotageArrosage::sauvegarderOuvertureVanne(uint8_t _idVanne)
{
    String heureActuelle = gestionnaireTemps->lireHeureSysteme();
    stockageEtat->sauvegarderOuverture(_idVanne, heureActuelle);
    DEBUG_VAL("NVS ouverture sauvegardee vanne ", _idVanne);
}

/**
 * @brief Persiste en NVS la fermeture d'une vanne.
 * @details Délègue à StockageEtatVannes::sauvegarderFermeture().
 * @param _idVanne Identifiant de la vanne concernée (1 à 4).
 */
void BoitierPilotageArrosage::sauvegarderFermetureVanne(uint8_t _idVanne)
{
    stockageEtat->sauvegarderFermeture(_idVanne);
    DEBUG_VAL("NVS fermeture sauvegardee vanne ", _idVanne);
}

/**
 * @brief Applique la programmation horaire de chaque vanne en mode
 *        Programme.
 * @details Pour chacune des 4 vannes dont le mode est MODE_PROGRAMME,
 *          recalcule l'occurrence courante de la programmation périodique
 *          (GestionnaireTemps::calculerOccurrenceCourante(), basée sur
 *          l'heure de début d'origine et la fréquence, et non l'heure de
 *          début brute qui ne correspond qu'à la toute première occurrence)
 *          pour obtenir la fenêtre d'arrosage [occurrence courante,
 *          occurrence courante + durée[ du jour, puis compare l'heure
 *          système courante :
 *          - si l'heure système est dans la fenêtre et que la vanne est
 *            fermée et disponible (pas d'impulsion en cours), déclenche
 *            l'ouverture, attend activement la fin de l'impulsion physique
 *            (timeout de sécurité 2 s) avant de persister le nouvel état en
 *            NVS et de notifier le client WebSocket connecté (le cas
 *            échéant) ;
 *          - si l'heure système est hors fenêtre et que la vanne est
 *            encore ouverte et disponible, déclenche symétriquement la
 *            fermeture, l'attente active, la persistance et la
 *            notification.
 *
 *          Appelée à la fois depuis traiterArrosage() (après un réveil par
 *          alarme RTC) et depuis traiterDialogue() (throttlée), afin que le
 *          déclenchement programmé fonctionne que l'application soit
 *          connectée ou non.
 */
void BoitierPilotageArrosage::verifierProgrammations()
{
    String heureSysteme = gestionnaireTemps->lireHeureSysteme();

    uint8_t idVanne = 0;
    while (idVanne < 4)
    {
        ProgrammationVanne prog = stockage->lireProgrammation(idVanne + 1);

        if (prog.mode == ProtocoleArrosageServeur::MODE_PROGRAMME)
        {
            String heureOuverture = gestionnaireTemps->calculerOccurrenceCourante(
                prog.mode, prog.heure, prog.frequence);
            String heureFermeture = gestionnaireTemps->ajouterMinutes(
                heureOuverture, prog.duree);

            if (heureOuverture.length() > 0 &&
                heureSysteme >= heureOuverture && heureSysteme < heureFermeture)
            {
                if (!vannes[idVanne]->estOuverte() && !vannes[idVanne]->estEnCours())
                {
                    DEBUG_VAL("Ouverture programmee vanne ", idVanne + 1);
                    vannes[idVanne]->ouvrir();

                    uint32_t debutImpulsion = millis();
                    while (vannes[idVanne]->estEnCours() &&
                           millis() - debutImpulsion < 2000)
                    {
                        vannes[idVanne]->update();
                        delay(10);
                    }

                    sauvegarderOuvertureVanne(idVanne + 1);
                    serveurWebSocket->envoyerMessage(
                        protocole->creerNotifEtatVanne(
                            ProtocoleArrosageServeur::CMD_OUVRIR, idVanne + 1));
                }
            }
            else
            {
                if (vannes[idVanne]->estOuverte() && !vannes[idVanne]->estEnCours())
                {
                    DEBUG_VAL("Fermeture programmee vanne ", idVanne + 1);
                    vannes[idVanne]->fermer();

                    uint32_t debutImpulsion = millis();
                    while (vannes[idVanne]->estEnCours() &&
                           millis() - debutImpulsion < 2000)
                    {
                        vannes[idVanne]->update();
                        delay(10);
                    }

                    sauvegarderFermetureVanne(idVanne + 1);
                    serveurWebSocket->envoyerMessage(
                        protocole->creerNotifEtatVanne(
                            ProtocoleArrosageServeur::CMD_FERMER, idVanne + 1));
                }
            }
        }

        idVanne++;
    }
}

// ============================================================
// Affichage batterie (BP2)
// ============================================================

/**
 * @brief Gère l'affichage ponctuel de l'état de la batterie déclenché par
 *        BP2.
 * @details Sur un front d'appui de BP2, affiche un relevé frais du
 *          capteur INA219 (ou un message d'indisponibilité si le capteur
 *          n'a pas été détecté à l'initialisation) et arme le délai
 *          DUREE_AFFICHAGE_BATTERIE_MS. Une fois ce délai écoulé sans
 *          nouvel appui, restaure l'affichage habituel. N'agit que sur le
 *          contenu de l'écran : ne modifie ni l'état de l'automate ni le
 *          pilotage des vannes, et n'interfère donc pas avec un arrosage
 *          ou un dialogue WebSocket en cours.
 */
void BoitierPilotageArrosage::verifierAffichageBatterie()
{
    bp2->update();

    if (bp2->frontAppui())
    {
        MesureBatterie mesure = mesureBatteries->lireMesure();
        afficheur->afficherMesureBatterie(mesure, mesureBatteries->disponible());
        affichageBatterieActif = true;
        finAffichageBatterieMs = millis() + DUREE_AFFICHAGE_BATTERIE_MS;
        DEBUG("BP2 : affichage batterie");
    }
    else if (affichageBatterieActif && millis() > finAffichageBatterieMs)
    {
        affichageBatterieActif = false;
        mettreAJourAffichageVannes();
        DEBUG("Fin affichage batterie → ecran habituel restaure");
    }
}
