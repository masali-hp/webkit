SET(WinLauncherLauncher_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/win"
    "${WEBCORE_DIR}"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/text"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/wtf"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces"
    "${CMAKE_BINARY_DIR}"
)

INCLUDE_DIRECTORIES(${WinLauncherLauncher_INCLUDE_DIRECTORIES})
ADD_EXECUTABLE (WinLauncherLauncher WIN32 DLLLauncherMain.cpp)
ADD_DEPENDENCIES(WinLauncherLauncher WinLauncher)
TARGET_LINK_LIBRARIES(WinLauncherLauncher WinLauncher)
SET_TARGET_PROPERTIES(WinLauncherLauncher PROPERTIES OUTPUT_NAME "WinLauncher")


