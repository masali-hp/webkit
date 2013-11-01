if (CMAKE_SYSTEM_NAME MATCHES WindowsCE)
    # NOMINMAX is already defined in c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\ce\include\altcecrt.h
    add_definitions(-D_HAS_EXCEPTIONS=0 -DUNICODE)
    set(WINOS "CE")
else ()
    add_definitions(-D_HAS_EXCEPTIONS=0 -DNOMINMAX -DUNICODE)
    set(WINOS "XP")
endif ()

# Windows XP SP2
# With these macros our binaries should run on XP SP2 and above, even when built on Win7.
add_definitions(-DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x502)

if (MSVC)
    add_definitions(/WX
        /wd4018 /wd4065 /wd4068 /wd4099 /wd4100 /wd4127 /wd4138 /wd4180 /wd4189 /wd4201 /wd4244 /wd4251 /wd4275 /wd4288 /wd4291
        /wd4305 /wd4344 /wd4355 /wd4389 /wd4396 /wd4503 /wd4505 /wd4510 /wd4512 /wd4610 /wd4706 /wd4800 /wd4951 /wd4952 /wd4996)

    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable RTTI

    if (NOT MSVC_VERSION LESS 1500)
        set(CMAKE_C_FLAGS "/MP ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")
    endif ()

    # Build without framepointers, if requested.
    # http://msdn.microsoft.com/en-us/library/2kxx5t2c(v=vs.90).aspx
    # Frame pointer omission makes stack walking too slow for memory leak tools.
    if (NOT USE_FPO)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oy-")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Oy-")
    endif ()

    foreach (flag_var CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
        # Disable the import/export warning, generate .map files
        set(${flag_var} "${${flag_var}} /ignore:4049 /MAP")
    endforeach (flag_var)
endif ()
