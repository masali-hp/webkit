list(APPEND JavaScriptCore_SOURCES
    jit/ExecutableAllocator.cpp
    API/JSStringRefBSTR.cpp
)

if(SHARED_CORE)
    list(APPEND JavaScriptCore_SOURCES
        JavaScriptCore.vcproj/JavaScriptCore/JavaScriptCore.def
    )
endif()

if (WTF_USE_CF)
    list(APPEND JavaScriptCore_SOURCES
        API/JSStringRefCF.cpp
    )
    list(APPEND JavaScriptCore_LIBRARIES
        CFLite
    )
endif ()

if (ENABLE_JIT AND WTF_CPU_ARM)
    if (ARM_THUMB2)
        set(MAKE_JIT_STUBS_PREFIX MSVC_THUMB2)
    else ()
        set(MAKE_JIT_STUBS_PREFIX MSVC_ARM)
    endif ()
    add_custom_command(
        OUTPUT ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.asm
        MAIN_DEPENDENCY ${JAVASCRIPTCORE_DIR}/create_jit_stubs
        DEPENDS ${JAVASCRIPTCORE_DIR}/jit/JITStubs.cpp
        COMMAND ${PERL_EXECUTABLE} ${JAVASCRIPTCORE_DIR}/create_jit_stubs --prefix=${MAKE_JIT_STUBS_PREFIX} ${JAVASCRIPTCORE_DIR}/jit/JITStubs.cpp > ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.asm
        VERBATIM)

    add_custom_command(
        OUTPUT ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.obj
        MAIN_DEPENDENCY ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.asm
        COMMAND armasm -nologo ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.asm ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.obj
        VERBATIM)

    list(APPEND JavaScriptCore_SOURCES ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/GeneratedJITStubs.obj)
endif ()
