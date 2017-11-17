include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(kwebkit_LIB_SRCS
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
)

ki18n_wrap_ui(kwebkit_LIB_SRCS
                  ui/searchbar.ui
)

ecm_qt_declare_logging_category(kwebkit_LIB_SRCS
    HEADER kwebkitpart_debug.h
    IDENTIFIER KWEBKITPART_LOG
    CATEGORY_NAME org.kde.kwebkitpart
)

########################### kwebkitpart #############################
add_library(kwebkitpart ${kwebkit_LIB_SRCS})

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
########################### kwebkitpart #############################

install(TARGETS kwebkitpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES kwebkitpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES kwebkitpart.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kwebkitpart)

install(FILES error.html DESTINATION ${KDE_INSTALL_DATADIR}/kwebkitpart)
