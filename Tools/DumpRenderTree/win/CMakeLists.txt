set(DumpRenderTree_SOURCES
    ${TOOLS_DIR}/DumpRenderTree/CyclicRedundancyCheck.cpp
    ${TOOLS_DIR}/DumpRenderTree/DumpRenderTreeCommon.cpp
    ${TOOLS_DIR}/DumpRenderTree/GCController.cpp
    ${TOOLS_DIR}/DumpRenderTree/JavaScriptThreading.cpp
    ${TOOLS_DIR}/DumpRenderTree/PixelDumpSupport.cpp
    ${TOOLS_DIR}/DumpRenderTree/TestRunner.cpp
    ${TOOLS_DIR}/DumpRenderTree/WorkQueue.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/DRTDataObject.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/DRTDesktopNotificationPresenter.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/DRTDropSource.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/DumpRenderTree.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/EditingDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/EventSender.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/FrameLoadDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/GCControllerWin.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/HistoryDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/MD5.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/PixelDumpSupportWin.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/PolicyDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/ResourceLoadDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/TestRunnerWin.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/TextInputController.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/TextInputControllerWin.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/UIDelegate.cpp
    ${TOOLS_DIR}/DumpRenderTree/win/WorkQueueItemWin.cpp
)

if (HAVE_ACCESSIBILITY)
    list(APPEND DumpRenderTree_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/AccessibilityController.cpp
        ${TOOLS_DIR}/DumpRenderTree/AccessibilityTextMarker.cpp
        ${TOOLS_DIR}/DumpRenderTree/AccessibilityUIElement.cpp
        ${TOOLS_DIR}/DumpRenderTree/win/AccessibilityControllerWin.cpp
        ${TOOLS_DIR}/DumpRenderTree/win/AccessibilityUIElementWin.cpp
    )
endif ()

if (NOT DONT_USE_DLLLAUNCHERS)
    set(ImageDiff_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/win/ImageDiffWin.cpp
    )
endif ()

set(DumpRenderTree_LIBRARIES
    WebKit
    WebCoreTestSupport
    WebKitGUID
    Oleacc
    comsuppw
)

if (WTF_USE_PTHREADS)
    list(APPEND DumpRenderTree_LIBRARIES
        pthreadVC2
    )
endif ()

set(DumpRenderTree_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebKit"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebKit2"
    ${WEBCORE_DIR}/testing/js
    ${TOOLS_DIR}/DumpRenderTree
    ${TOOLS_DIR}/DumpRenderTree/cg
    ${TOOLS_DIR}/DumpRenderTree/win
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/JavaScriptCore
    ${CMAKE_SOURCE_DIR}/Source/WTF
    ${CMAKE_BINARY_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
)

add_definitions(/wd4146)

if (WTF_PLATFORM_WIN_CAIRO)
    list(APPEND DumpRenderTree_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/cairo/PixelDumpSupportCairo.cpp
    )
    list(APPEND ImageDiff_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/win/ImageDiffCairo.cpp
    )
    list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
        ${TOOLS_DIR}/DumpRenderTree/cairo
    )
    if (WEBKIT_LIBRARIES_DIR)
        list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
            "${WEBKIT_LIBRARIES_DIR}/win/include/cairo"
            "${WEBKIT_LIBRARIES_DIR}/win/include"
        )
    elseif (3RDPARTY_DIR)
        list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
            "${3RDPARTY_DIR}/cairo/src"
            "${3RDPARTY_DIR}/libpng"
        )
    endif ()
    list(APPEND DumpRenderTree_LIBRARIES
        cairo
        libpng
        shlwapi
    )
elseif (WTF_USE_CG)
    list(APPEND DumpRenderTree_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/cg/PixelDumpSupportCG.cpp
    )
    list(APPEND ImageDiff_SOURCES
        ${TOOLS_DIR}/DumpRenderTree/cg/ImageDiffCG.cpp
    )
    list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
        ${TOOLS_DIR}/DumpRenderTree/cg
    )
endif ()

include_directories(${DumpRenderTree_INCLUDE_DIRECTORIES})

add_definitions(-D_UNICODE)

if (DONT_USE_DLLLAUNCHERS)
    add_definitions(-DUSE_CONSOLE_ENTRY_POINT=1)
    add_definitions(-DNO_DLL_LAUNCHER)
    add_executable(DumpRenderTree ${DumpRenderTree_SOURCES})
    add_executable(ImageDiff ${ImageDiff_SOURCES})
else ()
    add_library(DumpRenderTree SHARED ${DumpRenderTree_SOURCES})
    add_library(ImageDiff SHARED ${ImageDiff_SOURCES})
endif ()

target_link_libraries(DumpRenderTree ${DumpRenderTree_LIBRARIES})
set_target_properties(DumpRenderTree PROPERTIES FOLDER "Tools")

target_link_libraries(ImageDiff ${DumpRenderTree_LIBRARIES})
set_target_properties(ImageDiff PROPERTIES FOLDER "Tools")
