cmake_minimum_required(VERSION 3.20)
project(BTCPurchaseTracker VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
include(CTest)

add_executable(btc-purchase-tracker
    src/main.cpp
    src/diagnosticlog.cpp
    src/diagnosticlog.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/language.h
    src/currency.h
    src/purchasedialog.cpp
    src/purchasedialog.h
    src/database.cpp
    src/database.h
    src/monthlystats.cpp
    src/monthlystats.h
    src/searchfilter.cpp
    src/searchfilter.h
    src/csvutils.cpp
    src/csvutils.h
    resources/resources.qrc
)

target_link_libraries(btc-purchase-tracker PRIVATE Qt6::Widgets Qt6::Sql)

if(BUILD_TESTING)
    add_executable(btc-purchase-tracker-tests
        tests/tests.cpp
        src/database.cpp
        src/database.h
        src/monthlystats.cpp
        src/monthlystats.h
        src/searchfilter.cpp
        src/searchfilter.h
        src/csvutils.cpp
        src/csvutils.h
        src/language.h
        src/currency.h
    )
    target_include_directories(btc-purchase-tracker-tests PRIVATE src)
    target_link_libraries(btc-purchase-tracker-tests PRIVATE Qt6::Core Qt6::Sql)
    add_test(NAME btc-purchase-tracker-tests COMMAND btc-purchase-tracker-tests)

    add_executable(btc-purchase-dialog-tests
        tests/purchasedialog_tests.cpp
        src/purchasedialog.cpp
        src/purchasedialog.h
        src/csvutils.cpp
        src/database.cpp
    )
    target_include_directories(btc-purchase-dialog-tests PRIVATE src)
    target_link_libraries(btc-purchase-dialog-tests PRIVATE Qt6::Widgets Qt6::Sql)
    add_test(NAME btc-purchase-dialog-tests COMMAND btc-purchase-dialog-tests)
    set_tests_properties(btc-purchase-dialog-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 20
    )
endif()

# Windows-specific packaging details only.
# The application sources remain shared with Linux/macOS.
if(WIN32)
    set_target_properties(btc-purchase-tracker PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(btc-purchase-tracker PRIVATE
        windows/app.rc
    )
endif()

if(APPLE)
    set_target_properties(btc-purchase-tracker PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "BTC Purchase Tracker"
        MACOSX_BUNDLE_BUNDLE_NAME "BTC Purchase Tracker"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.darkvectordev.btc-purchase-tracker"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "btc-purchase-tracker.icns"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    )
    set_source_files_properties(macos/btc-purchase-tracker.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    target_sources(btc-purchase-tracker PRIVATE macos/btc-purchase-tracker.icns)
    install(TARGETS btc-purchase-tracker BUNDLE DESTINATION .)
else()
    install(TARGETS btc-purchase-tracker RUNTIME DESTINATION bin)
    install(FILES appimage/btc-purchase-tracker.desktop DESTINATION share/applications)
    install(FILES assets/btc_purchase_tracker_icon.png
            DESTINATION share/icons/hicolor/512x512/apps)
endif()
