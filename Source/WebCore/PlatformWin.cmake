list(REMOVE_ITEM WebCore_SOURCES
    platform/network/DNSResolveQueue.cpp
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/accessibility/win"
    "${WEBCORE_DIR}/plugins/win"
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBKIT_LIBRARIES_DIR}/win/include/SQLite"
    "${WEBKIT_LIBRARIES_DIR}/win/include/zlib"
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/loader/archive/cf"
    "${WEBCORE_DIR}/platform/cf"
)

list(APPEND WebCore_SOURCES
    accessibility/win/AccessibilityObjectWin.cpp
    accessibility/win/AXObjectCacheWin.cpp

    page/win/DragControllerWin.cpp
    page/win/EventHandlerWin.cpp

    plugins/PluginDatabase.cpp

    plugins/win/PluginDatabaseWin.cpp

    platform/Cursor.cpp
    platform/LocalizedStrings.cpp
    platform/PlatformStrategies.cpp

    platform/graphics/FontPlatformData.cpp

    platform/graphics/opentype/OpenTypeUtilities.cpp

    platform/graphics/win/FullScreenController.cpp
    platform/graphics/win/IconWin.cpp
    platform/graphics/win/ImageWin.cpp
    platform/graphics/win/IntPointWin.cpp
    platform/graphics/win/IntRectWin.cpp
    platform/graphics/win/IntSizeWin.cpp
    platform/graphics/win/TransformationMatrixWin.cpp

    platform/network/win/NetworkStateNotifierWin.cpp
    #platform/text/win/LocaleWin.cpp

    platform/win/BString.cpp
    platform/win/BitmapInfo.cpp
    platform/win/ClipboardUtilitiesWin.cpp
    platform/win/ClipboardWin.cpp
    platform/win/ContextMenuItemWin.cpp
    platform/win/ContextMenuWin.cpp
    platform/win/CursorWin.cpp
    platform/win/DragDataWin.cpp
    platform/win/DragImageWin.cpp
    platform/win/EditorWin.cpp
    platform/win/EventLoopWin.cpp
    platform/win/FileSystemWin.cpp
    platform/win/KeyEventWin.cpp
    platform/win/LanguageWin.cpp
    platform/win/LocalizedStringsWin.cpp
    platform/win/LoggingWin.cpp
    platform/win/MIMETypeRegistryWin.cpp
    platform/win/PasteboardWin.cpp
    platform/win/PathWalker.cpp
    platform/win/PopupMenuWin.cpp
    platform/win/PlatformMouseEventWin.cpp
    platform/win/PlatformScreenWin.cpp
    platform/win/RunLoopWin.cpp
    platform/win/SSLKeyGeneratorWin.cpp
    platform/win/ScrollbarThemeWin.cpp
    platform/win/SearchPopupMenuWin.cpp
    platform/win/SharedBufferWin.cpp
    platform/win/SharedTimerWin.cpp
    platform/win/SoundWin.cpp
    platform/win/SystemInfo.cpp
    platform/win/WCDataObject.cpp
    platform/win/WebCoreInstanceHandle.cpp
    platform/win/WebCoreTextRenderer.cpp
    platform/win/WidgetWin.cpp
    platform/win/WindowMessageBroadcaster.cpp
    platform/win/WheelEventWin.cpp

    platform/text/LocaleNone.cpp
    platform/text/TextEncodingDetectorNone.cpp
)

list(APPEND WebCore_SOURCES
    editing/SmartReplaceCF.cpp

    history/cf/HistoryPropertyList.cpp

    loader/archive/cf/LegacyWebArchive.cpp

    platform/cf/BinaryPropertyList.cpp
    platform/cf/KURLCFNet.cpp
    platform/cf/SharedBufferCF.cpp

    platform/cf/win/CertificateCFWin.cpp

    platform/network/win/DownloadBundleWin.cpp

    platform/text/cf/AtomicStringCF.cpp
    platform/text/cf/HyphenationCF.cpp
    platform/text/cf/StringCF.cpp
    platform/text/cf/StringImplCF.cpp
)

if (WTF_PLATFORM_WIN_CAIRO)
    list(APPEND WebCore_SOURCES
        page/win/FrameCairoWin.cpp

        platform/graphics/cairo/BitmapImageCairo.cpp
        platform/graphics/cairo/CairoUtilities.cpp
        platform/graphics/cairo/FontCairo.cpp
        platform/graphics/cairo/GradientCairo.cpp
        platform/graphics/cairo/GraphicsContextCairo.cpp
        platform/graphics/cairo/ImageBufferCairo.cpp
        platform/graphics/cairo/ImageCairo.cpp
        platform/graphics/cairo/OwnPtrCairo.cpp
        platform/graphics/cairo/PathCairo.cpp
        platform/graphics/cairo/PatternCairo.cpp
        platform/graphics/cairo/PlatformContextCairo.cpp
        platform/graphics/cairo/PlatformPathCairo.cpp
        platform/graphics/cairo/RefPtrCairo.cpp
        platform/graphics/cairo/TransformationMatrixCairo.cpp

        platform/image-decoders/cairo/ImageDecoderCairo.cpp

        platform/graphics/win/FontCustomPlatformDataCairo.cpp
        platform/graphics/win/FontPlatformDataCairoWin.cpp
        platform/graphics/win/GlyphPageTreeNodeCairoWin.cpp
        platform/graphics/win/GraphicsContextCairoWin.cpp
        platform/graphics/win/ImageCairoWin.cpp
        platform/graphics/win/SimpleFontDataCairoWin.cpp

        platform/win/DragImageCairoWin.cpp
    )
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/cairo"
    )

    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBKIT_LIBRARIES_DIR}/win/include/cairo"
    )

    list(APPEND WebCore_LIBRARIES
        cairo
    )

elseif (WTF_USE_CG)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/cg"
    )
    list(APPEND WebCore_SOURCES
        platform/graphics/win/FontCGWin.cpp
        platform/graphics/win/FontCustomPlatformData.cpp
        platform/graphics/win/FontPlatformDataCGWin.cpp
        platform/graphics/win/GlyphPageTreeNodeCGWin.cpp
        platform/graphics/win/GraphicsContextCGWin.cpp
        platform/graphics/win/ImageCGWin.cpp
        platform/graphics/win/SimpleFontDataCGWin.cpp
        platform/win/DragImageCGWin.cpp
    )
endif ()

list(APPEND WebCore_SOURCES
    platform/text/TextBreakIteratorICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
    platform/text/win/TextBreakIteratorInternalICUWin.cpp
)

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebCore_SOURCES
        plugins/win/PluginMessageThrottlerWin.cpp
        plugins/win/PluginPackageWin.cpp
        plugins/win/PluginViewWin.cpp
        plugins/PluginPackage.cpp
        plugins/PluginView.cpp
    )
else ()
    list(APPEND WebCore_SOURCES
        plugins/PluginPackage.cpp
        plugins/PluginPackageNone.cpp
        plugins/PluginViewNone.cpp
    )
endif ()

if (WTF_USE_CFNETWORK)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/network/cf"
        "${WEBCORE_DIR}/platform/network/win"
    )
    list(APPEND WebCore_SOURCES
        platform/network/cf/AuthenticationCF.cpp
        platform/network/cf/CookieJarCFNet.cpp
        platform/network/cf/CookieStorageCFNet.cpp
        platform/network/cf/CredentialStorageCFNet.cpp
        platform/network/cf/DNSCFNet.cpp
        platform/network/cf/FormDataStreamCFNet.cpp
        platform/network/cf/LoaderRunLoopCF.cpp
        platform/network/cf/NetworkStorageSessionCFNet.cpp
        platform/network/cf/ProxyServerCFNet.cpp
        platform/network/cf/ResourceErrorCF.cpp
        platform/network/cf/ResourceHandleCFNet.cpp
        platform/network/cf/ResourceRequestCFNet.cpp
        platform/network/cf/ResourceResponseCFNet.cpp
        platform/network/cf/SocketStreamHandleCFNet.cpp
    )
elseif (WTF_USE_CURL)
    list(APPEND WebCore_LIBRARIES
        libcurl_imp
    )
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/network/curl"
    )
    list(APPEND WebCore_SOURCES
        platform/network/NetworkStorageSessionStub.cpp
        platform/network/curl/CookieJarCurl.cpp
        platform/network/curl/CredentialStorageCurl.cpp
        platform/network/curl/DNSCurl.cpp
        platform/network/curl/FormDataStreamCurl.cpp
        platform/network/curl/ProxyServerCurl.cpp
        platform/network/curl/ResourceHandleCurl.cpp
        platform/network/curl/ResourceHandleManager.cpp
        platform/network/curl/SocketStreamHandleCurl.cpp
    )
elseif (WTF_USE_WININET)
    list(APPEND WebCore_INCLUDE_DIRECTORIES "${WEBCORE_DIR}/platform/network/win")
    list(APPEND WebCore_SOURCES
        platform/network/win/CredentialStorageWin.cpp
        platform/network/win/CookieJarWin.cpp
        platform/network/win/KURLWin.cpp
        platform/network/win/ProxyServerWin.cpp
        platform/network/win/ResourceHandleWin.cpp
        platform/network/win/SocketStreamHandleWin.cpp
    )
    list(APPEND WebCore_LIBRARIES wininet)
endif ()

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/themeWin.css
    ${WEBCORE_DIR}/css/themeWinQuirks.css
)

list(APPEND WebCore_LIBRARIES
    libjpeg
    libpng
    libxml2
    libxslt
    crypt32
    iphlpapi
)

list(APPEND WebCore_LIBRARIES
    SQLite3
    zdll
)

if (NOT SHARED_CORE)
    # If we don't use this flag, we sometimes see  NK1106: invalid file or disk full: cannot seek to ....
    # See https://connect.microsoft.com/VisualStudio/feedback/details/521439/link-failure-lnk1106-on-ltcg-for-a-static-library
    # Apparently this has been fixed in VS 2010.
    list(APPEND WebCore_LINK_FLAGS "/expectedoutputsize:200000000")
endif ()

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/page/win"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/win"
    "${WEBCORE_DIR}/platform/text/win"
    "${WEBCORE_DIR}/platform/win"
)

list(APPEND WebCore_SOURCES
    html/HTMLSelectElementWin.cpp

    rendering/RenderThemeWin.cpp

    page/win/FrameWin.cpp

    platform/win/GDIObjectCounter.cpp

    platform/graphics/win/DIBPixelData.cpp
    platform/graphics/win/FontCacheWin.cpp
    platform/graphics/win/FontPlatformDataWin.cpp
    platform/graphics/win/FontWin.cpp
    platform/graphics/win/GraphicsContextWin.cpp
    platform/graphics/win/SimpleFontDataWin.cpp
    platform/graphics/win/UniscribeController.cpp
)

list(APPEND WebCore_LIBRARIES
    shlwapi
    version
    usp10
    winmm
    ws2_32
)
