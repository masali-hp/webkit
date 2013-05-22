list(APPEND JavaScriptCore_SOURCES
    jit/ExecutableAllocator.cpp
    API/JSStringRefBSTR.cpp
)

if(SHARED_CORE)
    list(APPEND JavaScriptCore_SOURCES
        JavaScriptCore.vcproj/JavaScriptCore/JavaScriptCore.def
    )
endif()

list(APPEND JavaScriptCore_SOURCES
    API/JSStringRefCF.cpp
)
list(APPEND JavaScriptCore_LIBRARIES
    CFLite
)
