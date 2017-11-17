cmake_minimum_required(VERSION 2.8.12)
project(kwebkitpart)

set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.14.0")

# search packages used by the project
find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Gui
    Widgets
    WebKit
    WebKitWidgets
    UiTools
    PrintSupport
)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    DBusAddons
    KIO
    IconThemes
    Parts
    Sonnet
    WebKit
    I18n
)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DTRANSLATION_DOMAIN=\"kwebkitpart\")

add_subdirectory(icons)
add_subdirectory(src)
add_subdirectory(kdelauncher)

install(FILES kwebkitpart.categories DESTINATION ${KDE_INSTALL_CONFDIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
