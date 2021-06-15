add_library(kwebkitpart MODULE)

target_sources(kwebkitpart PRIVATE
    kwebkitpart.cpp
    kwebkitpart_ext.cpp
    kwebkitpartfactory.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    networkaccessmanager.cpp
    webpluginfactory.cpp
    settings/webkitsettings.cpp
    settings/webkit_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
    ui/featurepermissionbar.cpp

    kwebkitpart.qrc
)

ki18n_wrap_ui(kwebkitpart
    ui/searchbar.ui
)

ecm_qt_declare_logging_category(kwebkitpart
    HEADER kwebkitpart_debug.h
    IDENTIFIER KWEBKITPART_LOG
    EXPORT kwebkitpartlogging
    DESCRIPTION KWebKitPart
    CATEGORY_NAME org.kde.kwebkitpart
)

kcoreaddons_desktop_to_json(kwebkitpart kwebkitpart.desktop)

target_link_libraries(kwebkitpart
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Qt5::Widgets
    Qt5::WebKit
    Qt5::WebKitWidgets
    Qt5::PrintSupport
    KF5::WebKit
    KF5::Parts
    KF5::IconThemes
    KF5::SonnetCore
    KF5::KIOCore)

install(TARGETS kwebkitpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES kwebkitpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES error.html DESTINATION ${KDE_INSTALL_DATADIR}/kwebkitpart)
