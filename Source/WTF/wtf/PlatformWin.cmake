

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

include_directories(${WEBKIT_LIBRARIES_DIR}/win/include/pthreads)
list(APPEND WTF_LIBRARIES
    pthreadVC2
)
