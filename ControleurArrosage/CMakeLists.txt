cmake_minimum_required(VERSION 3.16)

project(ControleurArrosage VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS WebSockets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appControleurArrosage
    main.cpp
)

set_source_files_properties(Constantes.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appControleurArrosage
    URI ControleurArrosage
    VERSION 1.0
    QML_FILES
        Main.qml
        Constantes.qml
        PagePrincipale.qml
        PageVanne.qml
        PageConfiguration.qml
        Component/AppButton.qml
        Component/AppTextField.qml
        Component/DatePicker.qml
        Component/EntetePage.qml
        Component/Retour.qml
        Component/RoueChiffres.qml
        Component/SelecteurTumbler.qml
        Component/Titre.qml
        js/DatePicker.js
        js/PageConfiguration.js
        js/PagePrincipale.js
        js/PageVanne.js

    SOURCES
        controleurarrosage.cpp controleurarrosage.h
        vanne.cpp vanne.h
        communicationesp32.cpp communicationesp32.h
        protocolearrosageclient.h protocolearrosageclient.cpp

    RESOURCES
        images/imageArrosage.png
        images/settings.svg
)

target_link_libraries(appControleurArrosage
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Core
    PRIVATE Qt6::WebSockets
)

include(GNUInstallDirs)
install(TARGETS appControleurArrosage
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
