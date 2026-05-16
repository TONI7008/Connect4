cmake_minimum_required(VERSION 3.16)

project(connect4 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt modules ────────────────────────────────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# ── Sources ───────────────────────────────────────────────────────────────────
set(PROJECT_SOURCES
    main.cpp

    # Game logic
    connect_4.cpp       connect_4.h
    piece.cpp           piece.h
    ai.cpp              ai.h
    aihelper.cpp        aihelper.h

    # UI widgets
    circularwidget.cpp  circularwidget.h
    twidget.cpp         twidget.h
    tframe.cpp          tframe.h
    togglebutton.cpp    togglebutton.h
    tstackedwidget.cpp  tstackedwidget.h
    widgetfloater.cpp   widgetfloater.h
    winneroverlay.cpp   winneroverlay.h

    # Game engine (main window)
    gameengine.cpp      gameengine.h
    gameengine.ui

    # Auto-updater
    appupdater.cpp      appupdater.h
)

# ── Executable ────────────────────────────────────────────────────────────────
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(connect4
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(connect4 SHARED ${PROJECT_SOURCES})
    else()
        add_executable(connect4 ${PROJECT_SOURCES})
    endif()
endif()

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(connect4 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(connect4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Platform-specific properties ──────────────────────────────────────────────

# macOS bundle
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME    "Connect4")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING
        "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    # set(MACOSX_BUNDLE_ICON_FILE AppIcon.icns)   # uncomment when you have an icon
    # target_sources(connect4 PRIVATE resources/AppIcon.icns)
    # set_source_files_properties(resources/AppIcon.icns
    #     PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

# Windows app-icon / subsystem
if(WIN32)
    # target_sources(connect4 PRIVATE resources/connect4.rc)   # uncomment for icon
endif()

set_target_properties(connect4 PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE    # No console window on Windows
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING
        "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

# Minimum macOS version (Qt6 requires 10.14+, Qt5 works on 10.13+)
if(APPLE)
    set_target_properties(connect4 PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.14"
    )
endif()

# ── iOS / Android bundle id (Qt ≥ 6.1) ───────────────────────────────────────
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION
        MACOSX_BUNDLE_GUI_IDENTIFIER com.example.connect4)
endif()

# ── Install rules ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS connect4
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Finalise (Qt6 only) ───────────────────────────────────────────────────────
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(connect4)
endif()

# ── CPack (optional — produces platform installers via `cmake --build . --target package`) ──
# Uncomment the block below when you are ready to publish releases.
#
# set(CPACK_PACKAGE_NAME            "Connect4")
# set(CPACK_PACKAGE_VENDOR          "YourName")
# set(CPACK_PACKAGE_VERSION         ${PROJECT_VERSION})
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Connect 4 game")
# set(CPACK_RESOURCE_FILE_LICENSE   "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
#
# if(WIN32)
#     set(CPACK_GENERATOR "NSIS;ZIP")
# elseif(APPLE)
#     set(CPACK_GENERATOR "DragNDrop")
# else()
#     set(CPACK_GENERATOR "DEB;RPM;TGZ")
#     set(CPACK_DEBIAN_PACKAGE_MAINTAINER "YourName <you@example.com>")
# endif()
#
# include(CPack)
