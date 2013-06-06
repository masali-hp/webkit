if (WINOS MATCHES XP)
    list(APPEND JSC_LIBRARIES winmm)
elseif (WINOS MATCHES CE)
    # WinMain implements WinMain and invokes main.
    LIST(APPEND JSC_SOURCES
        ../os-win32/WinMain.cpp
    )
endif ()
