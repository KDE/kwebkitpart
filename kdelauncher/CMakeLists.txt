link_directories(${CMAKE_BINARY_DIR}/../src)
include_directories(${CMAKE_SOURCE_DIR}/../src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(kdelauncher main.cpp)
target_link_libraries(kdelauncher
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::WebKitWidgets
    Qt5::UiTools
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::KDELibs4Support
    KF5::WebKit
)
set_target_properties(kdelauncher PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
