include(OptionsWindows)

# Defaults for features for all ports are listed in WebKitFeatures.cmake
if (PORT_FLAVOR)
    # Port specific deviations set here
    include("Options${PORT_FLAVOR}")
endif ()

if (${PLATFORM} MATCHES "HP")
  INCLUDE_DIRECTORIES(${3RDPARTY_DIR}/../HP)
  ADD_SUBDIRECTORY(${3RDPARTY_DIR}/../HP/HPCommonSystemOS "${CMAKE_CURRENT_BINARY_DIR}/HP")
endif()

if (PLATFORM)
  ADD_DEFINITIONS(-DWTF_PLATFORM_${PLATFORM}=1)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES ARM)
    if (ARM_ARCH_VERSION)
        add_definitions(-D__ARM_ARCH_${ARM_ARCH_VERSION}__)
    endif ()
    if (ARM_THUMB2)
        add_definitions(-DWTF_CPU_ARM_THUMB2)
    else ()
        add_definitions(-DWTF_CPU_ARM_TRADITIONAL)
    endif ()
endif ()

if (WTF_USE_CF)
    if (NOT WEBKIT_LIBRARIES_DIR)
        set(WEBKIT_LIBRARIES_DIR ${CMAKE_SOURCE_DIR}/WebKitLibraries)
    endif()
    include_directories(${WEBKIT_LIBRARIES_DIR}/win/include)
    link_directories(${WEBKIT_LIBRARIES_DIR}/win/lib)
elseif (3RDPARTY_DIR)
    add_subdirectory(${3RDPARTY_DIR} "${CMAKE_CURRENT_BINARY_DIR}/3rdparty")
endif ()

include_directories(${JAVASCRIPTCORE_DIR}/os-win32)

if (WTF_USE_WCHAR_UNICODE)
  include_directories(${WTF_DIR}/wtf/unicode/wchar)
endif()

if (NOT SHARED_CORE)
  add_definitions(-DJS_NO_EXPORT)
endif ()

WEBKIT_DEFINE_IF_SET(WTF_USE_ICU_UNICODE)
WEBKIT_DEFINE_IF_SET(WTF_USE_WCHAR_UNICODE)
WEBKIT_DEFINE_IF_SET(WTF_USE_CF)
WEBKIT_DEFINE_IF_SET(WTF_PLATFORM_WIN_CAIRO)
WEBKIT_DEFINE_IF_SET(WTF_USE_CURL)
WEBKIT_DEFINE_IF_SET(WTF_USE_WININET)

if (NOT ENABLE_ACCESSIBILITY)
    add_definitions(-DHAVE_ACCESSIBILITY=0)
endif ()

if (WTF_USE_CURL AND WINOS MATCHES CE AND _CE_VERSION VERSION_LESS 800)
  include_directories(${3RDPARTY_DIR}/wcelibcex-1.0/src)
endif ()
