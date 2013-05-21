

if (WTF_USE_ICU_UNICODE)
    list(APPEND WTF_HEADERS
        unicode/icu/UnicodeIcu.h
    )
    list(APPEND WTF_SOURCES
        unicode/icu/CollatorICU.cpp
    )
    list(APPEND WTF_LIBRARIES
        libicuin
        libicuuc
    )
elseif (WTF_USE_WCHAR_UNICODE)
    list(APPEND WTF_HEADERS
        unicode/wchar/UnicodeWchar.h
    )
    list(APPEND WTF_SOURCES
        unicode/CollatorDefault.cpp
        unicode/wchar/UnicodeWchar.cpp
    )
endif ()

list(APPEND WTF_SOURCES
    NullPtr.cpp
    OSAllocatorWin.cpp
    ThreadingWin.cpp
    ThreadSpecificWin.cpp

    threads/win/BinarySemaphoreWin.cpp

    win/MainThreadWin.cpp
    win/OwnPtrWin.cpp
)

list(APPEND WTF_LIBRARIES
    winmm
)

if (WTF_USE_PTHREADS)
    include_directories(${WEBKIT_LIBRARIES_DIR}/win/include/pthreads)
    list(APPEND WTF_LIBRARIES
        pthreadVC2
    )
endif ()
