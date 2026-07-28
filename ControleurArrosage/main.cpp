/**
 * @file    main.cpp
 * @brief   Point d'entrée de l'application Qt/QML "Contrôleur d'Arrosage".
 *
 * @details Crée l'unique instance de ControleurArrosage (orchestrateur
 *          central, exposé à QML sous le nom de contexte "controleurArrosage"),
 *          l'expose au moteur QML, puis charge et démarre l'interface
 *          graphique (module QML "ControleurArrosage", composant "Main").
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>

#include <QFile>
#include <QDebug>
#include <QThread>
#include "controleurarrosage.h"

/**
 * @defgroup cpp_classes Classes C++
 * @brief Classes du backend Qt/C++ : modèle de données, transport
 *        WebSocket, protocole applicatif.
 */

/**
 * @defgroup qml_pages Pages QML
 * @brief Pages complètes poussées sur le StackView de navigation (une par
 *        écran de l'application).
 */

/**
 * @defgroup qml_components Composants QML
 * @brief Composants QML réutilisables (dossier Component/), utilisés par
 *        plusieurs pages.
 */



/**
 * @brief Démarre l'application : instancie l'orchestrateur, l'expose à QML
 *        et charge l'interface.
 * @param argc Nombre d'arguments de la ligne de commande (transmis à Qt).
 * @param argv Arguments de la ligne de commande (transmis à Qt).
 * @return Code de retour de la boucle d'événements Qt (QGuiApplication::exec()).
 */
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    ControleurArrosage controleur;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controleurArrosage",&controleur);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("ControleurArrosage", "Main");
    return app.exec();
}
