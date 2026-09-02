cmake_minimum_required(VERSION 3.20)
project(BTCPurchaseTracker VERSION 1.0.0 LANGUAGES CXX)

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
        src/csvutils.cpp
        src/csvutils.h
        src/language.h
        src/currency.h
    )
    target_include_directories(btc-purchase-tracker-tests PRIVATE src)
    target_link_libraries(btc-purchase-tracker-tests PRIVATE Qt6::Core Qt6::Sql)
    add_test(NAME btc-purchase-tracker-tests COMMAND btc-purchase-tracker-tests)
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

install(TARGETS btc-purchase-tracker RUNTIME DESTINATION bin)
install(FILES appimage/btc-purchase-tracker.desktop DESTINATION share/applications)
install(FILES assets/btc_purchase_tracker_icon.png
        DESTINATION share/icons/hicolor/512x512/apps)
