link_directories(${CMAKE_BINARY_DIR}/../src)
include_directories(${CMAKE_SOURCE_DIR}/../src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(kwebkit_DEMO_SRCS
    main.cpp
)

ecm_qt_declare_logging_category(kwebkit_DEMO_SRCS
    HEADER kwebkitpart_demo_debug.h
    IDENTIFIER KWEBKITPART_DEMO_LOG
    CATEGORY_NAME org.kde.kwebkitpart.demo
)

add_executable(kdelauncher ${kwebkit_DEMO_SRCS})
target_link_libraries(kdelauncher
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::WebKitWidgets
    Qt5::PrintSupport
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::WebKit
)
set_target_properties(kdelauncher PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
