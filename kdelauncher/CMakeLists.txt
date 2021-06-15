add_executable(kdelauncher)

target_sources(kdelauncher PRIVATE
    main.cpp
)

ecm_qt_declare_logging_category(kdelauncher
    HEADER kwebkitpart_demo_debug.h
    IDENTIFIER KWEBKITPART_DEMO_LOG
    CATEGORY_NAME org.kde.kwebkitpart.demo
)

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
