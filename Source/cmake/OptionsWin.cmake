
include(OptionsWindows)

# Defaults for features for all ports are listed in WebKitFeatures.cmake
if (PORT_FLAVOR)
    # Port specific deviations set here
    include("Options${PORT_FLAVOR}")
endif ()

if (NOT WEBKIT_LIBRARIES_DIR)
    set(WEBKIT_LIBRARIES_DIR ${CMAKE_SOURCE_DIR}/WebKitLibraries)
endif()
include_directories(${WEBKIT_LIBRARIES_DIR}/win/include)
link_directories(${WEBKIT_LIBRARIES_DIR}/win/lib)

if (NOT SHARED_CORE)
  add_definitions(-DJS_NO_EXPORT)
endif ()

# Windows XP SP2
add_definitions(-DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x502)

WEBKIT_DEFINE_IF_SET(WTF_PLATFORM_WIN_CAIRO)
WEBKIT_DEFINE_IF_SET(WTF_USE_CURL)
WEBKIT_DEFINE_IF_SET(WTF_USE_WININET)
