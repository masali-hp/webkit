list(APPEND WebKit_INCLUDE_DIRECTORIES
    win
    win/WebCoreSupport
    # For resource.h:
    win/WebKit.vcproj

    # Forwarding headers:
    "${DERIVED_SOURCES_WEBKIT_DIR}/include"
    "${CMAKE_SOURCE_DIR}/Source"
    "${DERIVED_SOURCES_WEBKIT_DIR}/include/WebCore"

    # Other webcore:
    "${DERIVED_SOURCES_DIR}"
    "${WEBCORE_DIR}/platform/graphics/win"
    "${WEBCORE_DIR}/platform/win"

    "${WEBCORE_DIR}/modules/websockets"

    # WebKit.h (from WebKit.idl):
    "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces"
)

list(APPEND WebKit_INCLUDES
    win/AboutData.h
    win/CodeAnalysisConfig.h
    win/COMEnumVariant.h
    win/COMPropertyBag.h
    win/COMVariantSetter.h
    win/DefaultDownloadDelegate.h
    win/DefaultPolicyDelegate.h
    win/DOMCoreClasses.h
    win/DOMCSSClasses.h
    win/DOMEventsClasses.h
    win/DOMHTMLClasses.h
    win/ForEachCoClass.h
    win/FullscreenVideoController.h
    win/MarshallingHelpers.h
    win/MemoryStream.h
    win/ProgIDMacros.h
    win/WebActionPropertyBag.h
    win/WebBackForwardList.h
    win/WebCache.h
    win/WebCachedFramePlatformData.h
    win/WebCookieManager.h
    win/WebCoreStatistics.h
    win/WebDatabaseManager.h
    win/WebDataSource.h
    win/WebDocumentLoader.h
    win/WebDownload.h
    win/WebDropSource.h
    win/WebElementPropertyBag.h
    win/WebError.h
    win/WebFrame.h
    win/WebFramePolicyListener.h
    win/WebHistory.h
    win/WebHistoryItem.h
    win/WebHTMLRepresentation.h
    win/WebIconDatabase.h
    win/WebJavaScriptCollector.h
    win/WebKitClassFactory.h
    win/WebKitCOMAPI.h
    win/WebScriptObject.h
    win/WebScriptWorld.h
    win/WebScrollBar.h
    win/WebSecurityOrigin.h
    win/WebSerializedJSValue.h
    win/WebTextRenderer.h
    win/WebKitDLL.h
    win/WebKitLogging.h
    win/WebKitStatistics.h
    win/WebKitStatisticsPrivate.h
    win/WebLocalizableStrings.h
    win/WebMutableURLRequest.h
    win/WebNavigationData.h
    win/WebNotification.h
    win/WebNotificationCenter.h
    win/WebPreferenceKeysPrivate.h
    win/WebPreferences.h
    win/WebResource.h
    win/WebURLAuthenticationChallenge.h
    win/WebURLAuthenticationChallengeSender.h
    win/WebURLCredential.h
    win/WebURLProtectionSpace.h
    win/WebURLResponse.h
    win/WebUserContentURLPattern.h
    win/WebView.h
    win/WebWorkersPrivate.h
)

list(APPEND WebKit_SOURCES_API
    win/WebKitCOMAPI.cpp
)

list(APPEND WebKit_SOURCES_Classes
    win/DefaultDownloadDelegate.cpp
    win/DefaultPolicyDelegate.cpp
    win/DOMCoreClasses.cpp
    win/DOMCSSClasses.cpp
    win/DOMEventsClasses.cpp
    win/DOMHTMLClasses.cpp
    win/ForEachCoClass.cpp
    win/FullscreenVideoController.cpp
    win/MemoryStream.cpp
    win/WebActionPropertyBag.cpp
    win/WebBackForwardList.cpp
    win/WebCache.cpp
    win/WebCookieManager.cpp
    win/WebCoreStatistics.cpp
    win/WebDatabaseManager.cpp
    win/WebDataSource.cpp
    win/WebDocumentLoader.cpp
    win/WebDownload.cpp
    win/WebDropSource.cpp
    win/WebElementPropertyBag.cpp
    win/WebError.cpp
    win/WebFrame.cpp
    win/WebFramePolicyListener.cpp
    win/WebHTMLRepresentation.cpp
    win/WebHistory.cpp
    win/WebHistoryItem.cpp
    win/WebIconDatabase.cpp
    win/WebJavaScriptCollector.cpp
    win/WebKitLogging.cpp
    win/WebKitStatistics.cpp
    win/WebMutableURLRequest.cpp
    win/WebNavigationData.cpp
    win/WebNetworkConfiguration.cpp
    win/WebNotification.cpp
    win/WebNotificationCenter.cpp
    win/WebPreferences.cpp
    win/WebResource.cpp
    win/WebScriptObject.cpp
    win/WebScriptWorld.cpp
    win/WebScrollBar.cpp
    win/WebSecurityOrigin.cpp
    win/WebSerializedJSValue.cpp
    win/WebTextRenderer.cpp
    win/WebURLAuthenticationChallenge.cpp
    win/WebURLAuthenticationChallengeSender.cpp
    win/WebURLCredential.cpp
    win/WebURLProtectionSpace.cpp
    win/WebURLResponse.cpp
    win/WebUserContentURLPattern.cpp
    win/WebView.cpp
    win/WebWorkersPrivate.cpp
)

list(APPEND WebKit_SOURCES_WebCoreSupport
    win/WebCoreSupport/EmbeddedWidget.cpp
    win/WebCoreSupport/EmbeddedWidget.h
    win/WebCoreSupport/WebChromeClient.cpp
    win/WebCoreSupport/WebChromeClient.h
    win/WebCoreSupport/WebDesktopNotificationsDelegate.cpp
    win/WebCoreSupport/WebDesktopNotificationsDelegate.h
    win/WebCoreSupport/WebDragClient.cpp
    win/WebCoreSupport/WebDragClient.h
    win/WebCoreSupport/WebEditorClient.cpp
    win/WebCoreSupport/WebEditorClient.h
    win/WebCoreSupport/WebFrameLoaderClient.cpp
    win/WebCoreSupport/WebFrameLoaderClient.h
    win/WebCoreSupport/WebFrameNetworkingContext.cpp
    win/WebCoreSupport/WebFrameNetworkingContext.h
    win/WebCoreSupport/WebPlatformStrategies.cpp
    win/WebCoreSupport/WebPlatformStrategies.h
)

list(APPEND WebKit_SOURCES
    win/AboutData.cpp
    win/MarshallingHelpers.cpp
    win/WebKitClassFactory.cpp
    win/WebKitDLL.cpp
)

if (ENABLE_ACCESSIBILITY)
    list(APPEND WebKit_INCLUDES
        win/AccessibleBase.h
        win/AccessibleDocument.h
        win/AccessibleImage.h
    )
    list(APPEND WebKit_SOURCES_Classes
        win/AccessibleBase.cpp
        win/AccessibleDocument.cpp
        win/AccessibleImage.cpp
    )
endif ()

if (ENABLE_CONTEXT_MENUS)
    list(APPEND WebKit_INCLUDES
        win/WebCoreSupport/WebContextMenuClient.h
    )
    list(APPEND WebKit_SOURCES_WebCoreSupport
        win/WebCoreSupport/WebContextMenuClient.cpp
    )
endif ()

if (WTF_PLATFORM_WIN_CAIRO)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/cairo"
    )
    if (WEBKIT_LIBRARIES_DIR)
        list(APPEND WebKit_INCLUDE_DIRECTORIES
            "${WEBKIT_LIBRARIES_DIR}/win/include/cairo"
        )
    elseif (3RDPARTY_DIR)
        list(APPEND WebKit_INCLUDE_DIRECTORIES
            "${3RDPARTY_DIR}/cairo/src"
        )
    endif ()
elseif (WTF_USE_CG)
    list(APPEND WebKit_SOURCES_API
        win/WebKitGraphics.cpp
    )
elseif (WINOS MATCHES CE)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/wince"
    )
endif ()

if (WTF_USE_CF)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/loader/archive/cf"
        "${WEBCORE_DIR}/platform/cf"
        "${WEBCORE_DIR}/loader/archive"
    )
    list(APPEND WebKit_INCLUDES
        win/WebArchive.h
        win/WebKitGraphics.h
        win/WebKitSystemBits.h
        win/CFDictionaryPropertyBag.h
    )
    list(APPEND WebKit_SOURCES_API
        win/WebLocalizableStrings.cpp
    )
    list(APPEND WebKit_SOURCES_Classes
        win/CFDictionaryPropertyBag.cpp
        win/WebArchive.cpp
    )
    list(APPEND WebKit_SOURCES
        win/WebKitSystemBits.cpp
    )
endif ()

if (WTF_USE_CF_NETWORK)
    list(APPEND WebKit_SOURCES_Classes
        win/WebCookieManagerCFNet.cpp
        win/WebDownloadCFNet.cpp
        win/WebURLAuthenticationChallengeSenderCFNet.cpp
    )
elseif (WTF_USE_CURL)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/network/curl"
    )
    if (3RDPARTY_DIR)
        list(APPEND WebKit_INCLUDE_DIRECTORIES
            "${3RDPARTY_DIR}/libcurl/include"
        )
    endif ()
    list(APPEND WebKit_SOURCES_Classes
        win/WebCookieManagerCurl.cpp
        win/WebDownloadCurl.cpp
        win/WebURLAuthenticationChallengeSenderCurl.cpp
    )
elseif (WTF_USE_WININET)
    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/network/win"
    )
    # There is no curl implementation in these files but they satisfy
    # linking requirements.
    list(APPEND WebKit_SOURCES_Classes
        win/WebCookieManagerCurl.cpp
        win/WebDownloadCurl.cpp
        win/WebURLAuthenticationChallengeSenderCurl.cpp
    )
endif ()

if (ENABLE_GEOLOCATION)
    list(APPEND WebKit_INCLUDES
        win/WebGeolocationPolicyListener.h
        win/WebGeolocationPosition.h
    )
    list(APPEND WebKit_SOURCES_Classes
        win/WebGeolocationPolicyListener.cpp
        win/WebGeolocationPosition.cpp
    )
    list(APPEND WebKit_SOURCES_WebCoreSupport
        win/WebCoreSupport/WebGeolocationClient.cpp
        win/WebCoreSupport/WebGeolocationClient.h
    )
endif ()

if (ENABLE_INSPECTOR)
    if (WTF_USE_CF)
        list(APPEND WebKit_SOURCES_WebCoreSupport cf/WebCoreSupport/WebInspectorClientCF.cpp)
    else ()
        list(APPEND WebKit_SOURCES_WebCoreSupport win/WebCoreSupport/WebInspectorClientNoCF.cpp)
    endif ()

    list(APPEND WebKit_INCLUDES
        win/WebInspector.h
    )
    list(APPEND WebKit_SOURCES_Classes
        win/WebInspector.cpp
    )
    list(APPEND WebKit_SOURCES_WebCoreSupport
        win/WebCoreSupport/WebInspectorClient.cpp
        win/WebCoreSupport/WebInspectorClient.h
        win/WebCoreSupport/WebInspectorDelegate.cpp
        win/WebCoreSupport/WebInspectorDelegate.h
    )

if (ENABLE_INSPECTOR_SERVER)
    list(APPEND WebKit_SOURCES_WebCoreSupport
        win/WebCoreSupport/InspectorServer/WebInspectorServer.cpp
        win/WebCoreSupport/InspectorServer/TcpServerWin.cpp
        win/WebCoreSupport/InspectorServer/WebSocketServer.cpp
        win/WebCoreSupport/InspectorServer/WebSocketServerWin.cpp
        win/WebCoreSupport/InspectorServer/WebSocketServerConnection.cpp
        win/WebCoreSupport/InspectorServer/WebInspectorServerWin.cpp
        win/WebCoreSupport/InspectorServer/HTTPRequest.cpp
    )
	# Generate inspectorPageIndex.h
    ADD_CUSTOM_COMMAND(
        OUTPUT "${DERIVED_SOURCES_WEBKIT_DIR}/include/inspectorPageIndex.h"
        MAIN_DEPENDENCY win/WebCoreSupport/InspectorServer/inspectorPageIndex.html
        COMMAND ${PERL_EXECUTABLE} ${WEBCORE_DIR}/inspector/xxd.pl inspectorPageIndex_html ${WEBKIT_DIR}/win/WebCoreSupport/InspectorServer/inspectorPageIndex.html "${DERIVED_SOURCES_WEBKIT_DIR}/include/inspectorPageIndex.h"
        VERBATIM)
        SET_SOURCE_FILES_PROPERTIES("${DERIVED_SOURCES_WEBKIT_DIR}/include/inspectorPageIndex.h" PROPERTIES GENERATED TRUE)
        LIST(APPEND WebKit_SOURCES_WebCoreSupport "${DERIVED_SOURCES_WEBKIT_DIR}/include/inspectorPageIndex.h")
endif()

    if (WINOS MATCHES XP)
        list(APPEND WebKit_INCLUDES
            win/WebNodeHighlight.h
        )
        list(APPEND WebKit_SOURCES
            win/WebNodeHighlight.cpp
        )
    endif ()
endif ()

if (ENABLE_DRAG_SUPPORT)
    list(APPEND WebKit_SOURCES_WebCoreSupport
        win/WebCoreSupport/WebDragClient.cpp
        win/WebCoreSupport/WebDragClient.h
    )
    if (WINOS MATCHES CE OR PLATFORM MATCHES HP)
        list(APPEND WebKit_SOURCES
            win/wince/DragDropManager.cpp
        )
    endif ()
endif ()

if (ENABLE_GESTURE_EVENTS)
    list(APPEND WebKit_INCLUDES
        win/wince/PlatformGestureSupport.h
    )
    list(APPEND WebKit_SOURCES
        win/wince/DefWindowProc.cpp
    )
endif ()

# Generate autoversion.h
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBKIT_DIR}/autoversion.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/win/cmake-autoversion.h ${DERIVED_SOURCES_WEBKIT_DIR}/autoversion.h
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/win/cmake-autoversion.h)
list(APPEND WebKit_INCLUDES ${DERIVED_SOURCES_WEBKIT_DIR}/autoversion.h)
set_source_files_properties(${DERIVED_SOURCES_WEBKIT_DIR}/autoversion.h PROPERTIES GENERATED TRUE)

list(APPEND WebKit_SOURCES ${WebKit_INCLUDES} ${WebKit_SOURCES_API} ${WebKit_SOURCES_Classes} ${WebKit_SOURCES_WebCoreSupport})

source_group(Includes FILES ${WebKit_INCLUDES})
source_group(API FILES ${WebKit_SOURCES_API})
source_group(Classes FILES ${WebKit_SOURCES_Classes})
source_group(WebCoreSupport FILES ${WebKit_SOURCES_WebCoreSupport})

# Build the COM interface:
macro (GENERATE_INTERFACE _infile _defines _depends)
    get_filename_component(_filewe ${_infile} NAME_WE)
    add_custom_command(
        OUTPUT  ${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/${_filewe}.h
        MAIN_DEPENDENCY ${_infile}
        DEPENDS ${_depends}
        COMMAND midl.exe /I "${CMAKE_CURRENT_SOURCE_DIR}/win/Interfaces" /I "${DERIVED_SOURCES_WEBKIT_DIR}" /WX /char signed /env win32 /tlb "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_filewe}.tlb" /out "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces" /h "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/${_filewe}.h" /iid "${_filewe}_i.c" ${_defines} "${CMAKE_CURRENT_SOURCE_DIR}/${_infile}"
        VERBATIM)
    set_source_files_properties(${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/${_filewe}.h PROPERTIES GENERATED TRUE)
endmacro ()

if (WINOS MATCHES CE)
  set(MIDL_DEFINES /D\ \"_WIN32_WCE=0x600\"\ /D\ \"UNDER_CE\"\ /D\ \"WINCE\"\ /D\ \"STANDARDSHELL_UI_MODEL\"\ /D\ \"__PRODUCTION__=01\")
else ()
  set(MIDL_DEFINES /D\ \"__PRODUCTION__=01\")
endif ()

set(WEBKIT_IDL_DEPENDENCIES
    win/Interfaces/AccessibleComparable.idl
    win/Interfaces/DOMCore.idl
    win/Interfaces/DOMCSS.idl
    win/Interfaces/DOMEvents.idl
    win/Interfaces/DOMExtensions.idl
    win/Interfaces/DOMHTML.idl
    win/Interfaces/DOMPrivate.idl
    win/Interfaces/DOMRange.idl
    win/Interfaces/DOMWindow.idl
    win/Interfaces/IGEN_DOMObject.idl
    win/Interfaces/IWebArchive.idl
    win/Interfaces/IWebBackForwardList.idl
    win/Interfaces/IWebBackForwardListPrivate.idl
    win/Interfaces/IWebCache.idl
    win/Interfaces/IWebCookieManager.idl
    win/Interfaces/IWebCoreStatistics.idl
    win/Interfaces/IWebDatabaseManager.idl
    win/Interfaces/IWebDataSource.idl
    win/Interfaces/IWebDesktopNotificationsDelegate.idl
    win/Interfaces/IWebDocument.idl
    win/Interfaces/IWebDownload.idl
    win/Interfaces/IWebEditingDelegate.idl
    win/Interfaces/IWebEmbeddedView.idl
    win/Interfaces/IWebError.idl
    win/Interfaces/IWebErrorPrivate.idl
    win/Interfaces/IWebFormDelegate.idl
    win/Interfaces/IWebFrame.idl
    win/Interfaces/IWebFrameLoadDelegate.idl
    win/Interfaces/IWebFrameLoadDelegatePrivate.idl
    win/Interfaces/IWebFrameLoadDelegatePrivate2.idl
    win/Interfaces/IWebFramePrivate.idl
    win/Interfaces/IWebFrameView.idl
    win/Interfaces/IWebGeolocationPolicyListener.idl
    win/Interfaces/IWebGeolocationPosition.idl
    win/Interfaces/IWebGeolocationProvider.idl
    win/Interfaces/IWebHistory.idl
    win/Interfaces/IWebHistoryDelegate.idl
    win/Interfaces/IWebHistoryItem.idl
    win/Interfaces/IWebHistoryItemPrivate.idl
    win/Interfaces/IWebHistoryPrivate.idl
    win/Interfaces/IWebHTMLRepresentation.idl
    win/Interfaces/IWebHTTPURLResponse.idl
    win/Interfaces/IWebIconDatabase.idl
    win/Interfaces/IWebInspector.idl
    win/Interfaces/IWebInspectorPrivate.idl
    win/Interfaces/IWebJavaScriptCollector.idl
    win/Interfaces/IWebKitStatistics.idl
    win/Interfaces/IWebMutableURLRequest.idl
    win/Interfaces/IWebMutableURLRequestPrivate.idl
    win/Interfaces/IWebNavigationData.idl
    win/Interfaces/IWebNetworkConfiguration.idl
    win/Interfaces/IWebNotification.idl
    win/Interfaces/IWebNotificationCenter.idl
    win/Interfaces/IWebNotificationObserver.idl
    win/Interfaces/IWebPolicyDelegate.idl
    win/Interfaces/IWebPolicyDelegatePrivate.idl
    win/Interfaces/IWebPreferences.idl
    win/Interfaces/IWebPreferencesPrivate.idl
    win/Interfaces/IWebResource.idl
    win/Interfaces/IWebResourceLoadDelegate.idl
    win/Interfaces/IWebResourceLoadDelegatePrivate.idl
    win/Interfaces/IWebResourceLoadDelegatePrivate2.idl
    win/Interfaces/IWebScriptObject.idl
    win/Interfaces/IWebScriptWorld.idl
    win/Interfaces/IWebScrollBarDelegatePrivate.idl
    win/Interfaces/IWebScrollBarPrivate.idl
    win/Interfaces/IWebSecurityOrigin.idl
    win/Interfaces/IWebSerializedJSValue.idl
    win/Interfaces/IWebSerializedJSValuePrivate.idl
    win/Interfaces/IWebTextRenderer.idl
    win/Interfaces/IWebUIDelegate.idl
    win/Interfaces/IWebUIDelegate2.idl
    win/Interfaces/IWebUIDelegatePrivate.idl
    win/Interfaces/IWebUndoManager.idl
    win/Interfaces/IWebUndoTarget.idl
    win/Interfaces/IWebURLAuthenticationChallenge.idl
    win/Interfaces/IWebURLRequest.idl
    win/Interfaces/IWebURLResponse.idl
    win/Interfaces/IWebURLResponsePrivate.idl
    win/Interfaces/IWebUserContentURLPattern.idl
    win/Interfaces/IWebView.idl
    win/Interfaces/IWebViewPrivate.idl
    win/Interfaces/IWebWorkersPrivate.idl
    win/Interfaces/JavaScriptCoreAPITypes.idl
    win/Interfaces/WebKit.idl
    win/Interfaces/WebScrollbarTypes.idl
)

GENERATE_INTERFACE(win/Interfaces/WebKit.idl ${MIDL_DEFINES} "${WEBKIT_IDL_DEPENDENCIES};${DERIVED_SOURCES_WEBKIT_DIR}/autoversion.h")

set_source_files_properties("${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/WebKit_i.c" PROPERTIES GENERATED TRUE)
add_library(WebKitGUID STATIC ${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/WebKit.h "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/WebKit_i.c")
set_target_properties(WebKitGUID PROPERTIES FOLDER "WebKit")
list(APPEND WebKit_LIBRARIES WebKitGUID)
# If this directory isn't created before midl runs and attempts to output WebKit.tlb,
# It fails with an unusual error - midl failed - failed to save all changes
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Forwarding headers:
# We use a perl script that lives in the WebKit2 platform
# to build our forwarding headers.
add_custom_target(forwarding-headerWin COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_DIR} ${DERIVED_SOURCES_WEBKIT_DIR}/include win --leaveExistingHeaders)
set_target_properties(forwarding-headerWin PROPERTIES FOLDER "WebKit")
set(ForwardingHeaders_NAME forwarding-headerWin)

if (WTF_USE_CF)
    add_custom_target(forwarding-headerCF COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_DIR} ${DERIVED_SOURCES_WEBKIT_DIR}/include cf --leaveExistingHeaders)
    set_target_properties(forwarding-headerCF PROPERTIES FOLDER "WebKit")
    set(ForwardingFoundationHeaders_NAME forwarding-headerCF)
endif ()
if (WTF_USE_CURL)
    add_custom_target(forwarding-headerCurl COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_DIR} ${DERIVED_SOURCES_WEBKIT_DIR}/include curl)
    set_target_properties(forwarding-headerCurl PROPERTIES FOLDER "WebKit")
    set(ForwardingNetworkHeaders_NAME forwarding-headerCurl)
endif ()

# WebKit export file (WebKitExports.def):
# Use c preprocessor to transform the file, then use perl script to remove blank lines.
set(WEBKIT_EXPORTS_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/win/WebKit.vcproj/WebKitExports.def.in)
set(WEBKIT_EXPORTS_INT ${CMAKE_BINARY_DIR}/Source/WebKit/WebKitExports.def.i)
set(WEBKIT_EXPORTS_OUTPUT ${DERIVED_SOURCES_WEBKIT_DIR}/WebKitExports.def)
# This part is ugly.  Because we use the c preprocessor on WebKitExports.def.in, Visual
# Studio thinks it is a source file and wants to link WebKitExports.def.in.obj.
# There seems to be no way to have the preprocessor run on the file but not pass it
# to the list of files to link.  We will make an empty source file which will be compiled
# into WebKitExports.def.obj so that linking succeeds.
set(WEBKIT_EXPORTS_BLANK ${DERIVED_SOURCES_WEBKIT_DIR}/blank.cpp)
if (NOT EXISTS ${WEBKIT_EXPORTS_BLANK})
    file(WRITE ${WEBKIT_EXPORTS_BLANK} "")
endif ()
#  "/FoWebKit.dir\\${CMAKE_CFG_INTDIR}\\WebKitExports.def.obj"
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/Source/WebKit/WebKit.dir/${CMAKE_CFG_INTDIR}/blank
    MAIN_DEPENDENCY ${WEBKIT_EXPORTS_BLANK}
    COMMAND ${CMAKE_C_COMPILER} /c /FoWebKit.dir\\${CMAKE_CFG_INTDIR}\\WebKitExports.def.obj ${WEBKIT_EXPORTS_BLANK}
)
if (CMAKE_SYSTEM_PROCESSOR MATCHES ARM)
    set(EXPORTS_ARCH --architecture=ARM)
else ()
    set(EXPORTS_ARCH --architecture=x86)
endif ()
add_custom_command(
    OUTPUT ${WEBKIT_EXPORTS_OUTPUT}
    MAIN_DEPENDENCY ${WEBKIT_EXPORTS_INT}
    DEPENDS ${WEBKIT_EXPORTS_INPUT} ${WEBKIT_EXPORTS_INT}
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT_DIR}/Scripts/trim-win-exports.pl --input ${WEBKIT_EXPORTS_INT} ${EXPORTS_ARCH} > ${WEBKIT_EXPORTS_OUTPUT}
)
set_source_files_properties(${WEBKIT_EXPORTS_INPUT} PROPERTIES COMPILE_FLAGS "/EP /P")
set_source_files_properties(${WEBKIT_EXPORTS_INT} PROPERTIES GENERATED TRUE)
set_source_files_properties(${WEBKIT_EXPORTS_OUTPUT} PROPERTIES GENERATED TRUE)
list(APPEND WebKit_SOURCES
    ${WEBKIT_EXPORTS_INPUT}
    ${WEBKIT_EXPORTS_INT}
    ${WEBKIT_EXPORTS_OUTPUT}
    ${WEBKIT_EXPORTS_BLANK}
)
source_group(Exports FILES ${WEBKIT_EXPORTS_INPUT} ${WEBKIT_EXPORTS_INT} ${WEBKIT_EXPORTS_OUTPUT} ${WEBKIT_EXPORTS_BLANK})

# Add library dependencies for WebKit:
if (WINOS MATCHES XP)
    list(APPEND WebKit_LIBRARIES
        shlwapi
        usp10
        comctl32
        rpcrt4
        comsupp
    )
endif ()

# We need the webkit libraries to come before the system default libraries.
# To do this we add system default libs as webkit libs and zero out system
# default libs.
string(REPLACE " " "\;" CXX_LIBS ${CMAKE_CXX_STANDARD_LIBRARIES})
# wtf.lib is an inherited dependency for webkit that needs to come before
# system libraries for ARM builds when we override new/delete.
list(APPEND WebKit_LIBRARIES WTF)
list(APPEND WebKit_LIBRARIES ${CXX_LIBS})
set(CMAKE_CXX_STANDARD_LIBRARIES "")

set(WebKit_LIBRARY_TYPE SHARED)


set(WebKit_INSTALL_HEADERS
    "${DERIVED_SOURCES_WEBKIT_DIR}/Interfaces/WebKit.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/win/WebKitCOMAPI.h"
)

foreach(_infile ${WebKit_INSTALL_HEADERS})
    get_filename_component(_filewe ${_infile} NAME)
    set(outf ${DERIVED_SOURCES_WEBKIT_DIR}/${_filewe})
    file(TO_NATIVE_PATH ${_infile} _infile)
    file(TO_NATIVE_PATH ${outf} outf)
    add_custom_command(OUTPUT ${outf}
                     COMMAND COPY ${_infile} ${outf}
                     MAIN_DEPENDENCY ${_infile})
endforeach()

if (WINOS MATCHES XP)
    list(APPEND WebKit_LIBRARIES
        winmm
    )
endif ()

# incremental linking fails for debug builds
SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
