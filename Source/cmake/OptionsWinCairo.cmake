WEBKIT_OPTION_BEGIN()
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ACCESSIBILITY ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_FILTERS ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DATALIST_ELEMENT OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DRAG_SUPPORT ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FILTERS ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GEOLOCATION ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIGH_DPI_CANVAS ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_IMAGE_DECODER_DOWN_SAMPLING ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_METER_ELEMENT OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SHARED_WORKERS ON)
WEBKIT_OPTION_END()

# Using ICU unicode and Core Foundation are optional with the WinCairo CMake
# port, but the default is to build with CF and ICU
if (NOT DEFINED WTF_USE_ICU_UNICODE)
    set(WTF_USE_ICU_UNICODE 1)
endif ()
if (NOT DEFINED WTF_USE_CF)
    set(WTF_USE_CF 1)
endif ()

if (NOT WTF_USE_ICU_UNICODE)
    set(WTF_USE_WCHAR_UNICODE 1)
endif ()

set(WTF_PLATFORM_WIN_CAIRO 1)
set(WTF_USE_CURL 1)

if (NOT USE_SYSTEM_MALLOC)
    # FastMalloc.cpp uses pthreads when not using system malloc.
    set(WTF_USE_PTHREADS 1)
endif ()

# Using DLL launchers requires Safari to be installed.
set(DONT_USE_DLLLAUNCHERS 1)
