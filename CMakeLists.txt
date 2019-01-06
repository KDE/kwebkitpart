cmake_minimum_required(VERSION 3.0)
project(kwebkitpart)

set(QT_MIN_VERSION "5.6.0")
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
    PrintSupport
)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    KIO
    IconThemes
    Parts
    Sonnet
    WebKit
    I18n
)

add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050600
    -DQT_USE_FAST_OPERATOR_PLUS
    -DQT_NO_CAST_TO_ASCII
#     -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_SIGNALS_SLOTS_KEYWORDS
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

add_definitions(-DTRANSLATION_DOMAIN=\"kwebkitpart\")

add_subdirectory(icons)
add_subdirectory(src)
add_subdirectory(kdelauncher)

install(FILES kwebkitpart.categories DESTINATION ${KDE_INSTALL_CONFDIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
