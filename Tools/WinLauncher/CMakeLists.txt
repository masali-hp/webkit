SET(WinLauncher_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/win"
    "${WEBCORE_DIR}"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/text"
    "${JAVASCRIPTCORE_DIR}"
    "${WTF_DIR}"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces"
    "${CMAKE_BINARY_DIR}"
)

SET (WinLauncher_INCLUDES
    DOMDefaultImpl.h
    PrintWebUIDelegate.h
    resource.h
    stdafx.h
    WinLauncher.h
    WinLauncherLauncherResource.h
)

SET (WinLauncher_SOURCES
    PrintWebUIDelegate.cpp
    stdafx.cpp
    WinLauncher.cpp
)

SET (WinLauncher_RESOURCES
    small.ico
    WinLauncher.ico
    WinLauncher.rc
)
SOURCE_GROUP("Resource Files" FILES ${WinLauncher_RESOURCES})

SET(WinLauncher_LIBRARIES
    WebKitGUID
    WebKit
)

IF (WINOS MATCHES XP)
    LIST(APPEND WinLauncher_LIBRARIES
        comctl32
        shlwapi
        ws2_32
    )
ELSE ()
    LIST(APPEND WinLauncher_LIBRARIES
        ws2
    )
ENDIF ()

INCLUDE_DIRECTORIES(${WinLauncher_INCLUDE_DIRECTORIES})

# When building for CE, we need the webkit libraries to come before
# the system default libraries.  To do this we add system default libs
# as webkit libs and zero out system default libs.
STRING(REPLACE " " "\;" CXX_LIBS ${CMAKE_CXX_STANDARD_LIBRARIES})
LIST(APPEND WinLauncher_LIBRARIES ${CXX_LIBS})
SET(CMAKE_CXX_STANDARD_LIBRARIES "")

# If you want to build WinLauncher as a standalone EXE, then set DONT_USE_DLLLAUNCHERS.
# Otherwise it is built as a DLL and WinLauncherLauncher is expected to start it.
IF (DONT_USE_DLLLAUNCHERS)
    ADD_DEFINITIONS(-DBUILD_WINLAUNCHER_EXE)
    ADD_EXECUTABLE (WinLauncher WIN32 ${WinLauncher_INCLUDES} ${WinLauncher_SOURCES} ${WinLauncher_RESOURCES})
ELSE()
    ADD_LIBRARY (WinLauncher SHARED ${WinLauncher_INCLUDES} ${WinLauncher_SOURCES} ${WinLauncher_RESOURCES})
ENDIF()
set_target_properties(WinLauncher PROPERTIES FOLDER "Tools")

ADD_DEPENDENCIES(WinLauncher WebKit)
TARGET_LINK_LIBRARIES(WinLauncher ${WinLauncher_LIBRARIES})

if (NOT DONT_USE_DLLLAUNCHERS)
    add_executable (WinLauncherLauncher WIN32 ${TOOLS_DIR}/win/DLLLauncher/DLLLauncherMain.cpp WinLauncher.vcxproj/WinLauncher.rc)
    target_link_libraries(WinLauncherLauncher shlwapi)
    set_target_properties(WinLauncherLauncher PROPERTIES FOLDER "Tools")
    set_target_properties(WinLauncherLauncher PROPERTIES OUTPUT_NAME "WinLauncher")
endif ()

# When building with corefoundation (and hence depending on the webkitlibraries
# bundle), the libraries in the bundle need to be copied to the build bin
# directory or webkit won't load (can't find CFLite.dll).
# This may only be true for WinCairo (with CF dependencies) builds.
# WinApple may get by with using just the .dlls found in the safari installation
# folder.
if (WEBKIT_LIBRARIES_DIR)
    file(GLOB WebKitLibraries ${WEBKIT_LIBRARIES_DIR}/win/bin/*.dll)
    foreach(WebKitLibrary ${WebKitLibraries})
        add_custom_command(TARGET WinLauncher POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E
                       copy ${WebKitLibrary} $<TARGET_FILE_DIR:WinLauncher>)
    endforeach()
endif ()
