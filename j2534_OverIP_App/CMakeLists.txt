

qt_add_executable(j2534_OverIP_App WIN32
    main.cpp)

qt_add_qml_module(j2534_OverIP_App
    URI de.xplatforms.j2534
    VERSION 1.0
    QML_FILES
        Main.qml
        MainPage.qml
        InterfacePage.qml
        InterfaceRow.qml
        ExIconButton.qml
        J2534Emulator.qml
        J2534Wrapper.qml
        WrapperMainView.qml
        WrapperSettings.qml
        WrapperRXView.qml
        WrapperTXView.qml
        WrapperRXTXQueuesView.qml
        J2534Cannelloni.qml
        CannelloniMainView.qml
        CannelloniSettings.qml
        CannelloniRXView.qml
        CannelloniRXTXQueuesView.qml
        CannelloniTXView.qml
    SOURCES
        defs.h
        types.h
        exj2534cannelloni.cpp exj2534cannelloni.h
        exj2534emulator.cpp exj2534emulator.h
        exj2534readmsgslist.cpp exj2534readmsgslist.h
        exj2534wrapper.cpp exj2534wrapper.h
        exj2534writemsgslist.cpp exj2534writemsgslist.h
        exmessagepreprocessor.cpp exmessagepreprocessor.h
        expipeserver.cpp expipeserver.h
        msgstableviewmodel.cpp msgstableviewmodel.h
        pipecapture.cpp pipecapture.h
        passthruregistry.cpp passthruregistry.h
        interfaceconfig.cpp interfaceconfig.h
        vehicleserver.cpp vehicleserver.h)

# Icons are referenced as qrc:/assets/... from QML, so keep them at the root prefix.
qt_add_resources(j2534_OverIP_App "assets"
    PREFIX /
    FILES
        assets/arrow_left.png
        assets/arrow_left.svg
        assets/arrow_right.png
        assets/arrow_right.svg
        assets/arrow2_left.png
        assets/arrow2_right.png
        assets/arrows.png
        assets/arrows.svg
        assets/garage.png
        assets/garage.svg
        assets/settings_icon.png)

# Only PassThruStruct.h is shared with the DLL; the App does not link it.
target_include_directories(j2534_OverIP_App PRIVATE ${PROJECT_SOURCE_DIR}/j2534_OverIP)
target_compile_definitions(j2534_OverIP_App PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x060000)
target_link_libraries(j2534_OverIP_App PRIVATE
    j2534_pipe_qt
    shell32
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Network Qt6::WebSockets)

include(GNUInstallDirs)
install(TARGETS j2534_OverIP_App
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_qml_app_script(
    TARGET j2534_OverIP_App
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS)
install(SCRIPT ${deploy_script})
