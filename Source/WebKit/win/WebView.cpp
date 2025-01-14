/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple, Inc.  All rights reserved.
 * Copyright (C) 2009, 2010, 2011 Appcelerator, Inc. All rights reserved.
 * Copyright (C) 2011 Brent Fulgham. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebView.h"

#include "CFDictionaryPropertyBag.h"
#include "DOMCoreClasses.h"
#include "FullscreenVideoController.h"
#include "MarshallingHelpers.h"
#include "SoftLinking.h"
#include "TextIterator.h"
#include "WebBackForwardList.h"
#include "WebChromeClient.h"
#include "WebContextMenuClient.h"
#include "WebCoreTextRenderer.h"
#include "WebDatabaseManager.h"
#include "WebDocumentLoader.h"
#include "WebDownload.h"
#include "WebDragClient.h"
#include "WebEditorClient.h"
#include "WebElementPropertyBag.h"
#include "WebFrame.h"
#include "WebFrameNetworkingContext.h"
#include "WebGeolocationClient.h"
#include "WebGeolocationPosition.h"
#include "WebIconDatabase.h"
#include "WebInspector.h"
#include "WebInspectorClient.h"
#include "WebKit.h"
#include "WebKitDLL.h"
#include "WebKitLogging.h"
#include "WebKitStatisticsPrivate.h"
#include "WebKitSystemBits.h"
#include "WebMutableURLRequest.h"
#include "WebNotificationCenter.h"
#include "WebPlatformStrategies.h"
#include "WebPreferences.h"
#include "WebScriptWorld.h"
#include "resource.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/JSCJSValue.h>
#include <JavaScriptCore/JSLock.h>
#include <WebCore/AXObjectCache.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/BString.h>
#include <WebCore/BackForwardListImpl.h>
#include <WebCore/BitmapInfo.h>
#include <WebCore/Chrome.h>
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/Cursor.h>
#include <WebCore/DatabaseManager.h>
#include <WebCore/Document.h>
#include <WebCore/DocumentMarkerController.h>
#include <WebCore/DragController.h>
#include <WebCore/DragData.h>
#include <WebCore/DragSession.h>
#include <WebCore/Editor.h>
#include <WebCore/EventHandler.h>
#include <WebCore/EventNames.h>
#include <WebCore/FileSystem.h>
#include <WebCore/FloatQuad.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameSelection.h>
#include <WebCore/FrameTree.h>
#include <WebCore/FrameView.h>
#include <WebCore/FrameWin.h>
#include <WebCore/GDIObjectCounter.h>
#include <WebCore/GeolocationController.h>
#include <WebCore/GeolocationError.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/HTMLMediaElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HWndDC.h>
#include <WebCore/HistoryController.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/HitTestRequest.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/InitializeLogging.h>
#include <WebCore/IntRect.h>
#include <WebCore/JSElement.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Language.h>
#include <WebCore/MIMETypeRegistry.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/Page.h>
#include <WebCore/PageCache.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/PlatformMouseEvent.h>
#include <WebCore/PlatformWheelEvent.h>
#include <WebCore/PluginData.h>
#include <WebCore/PluginDatabase.h>
#include <WebCore/PluginView.h>
#include <WebCore/PopupMenu.h>
#include <WebCore/PopupMenuWin.h>
#include <WebCore/ProgressTracker.h>
#include <WebCore/RenderLayer.h>
#include <WebCore/RenderTheme.h>
#include <WebCore/RenderTreeAsText.h>
#include <WebCore/RenderView.h>
#include <WebCore/RenderWidget.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceHandleClient.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/RuntimeEnabledFeatures.h>
#include <WebCore/SchemeRegistry.h>
#include <WebCore/ScriptController.h>
#include <WebCore/ScriptValue.h>
#include <WebCore/Scrollbar.h>
#include <WebCore/ScrollbarTheme.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityPolicy.h>
#include <WebCore/Settings.h>
#include <WebCore/SimpleFontData.h>
#include <WebCore/SystemInfo.h>
#include <WebCore/WindowMessageBroadcaster.h>
#include <WebCore/WindowsTouch.h>
#include <wtf/MainThread.h>
#include <wtf/PerformanceTrace.h>
#include <JavaScriptCore/JSLock.h>

#if OS(WINCE)
#define SetWindowLongPtrW(x, y, z) SetWindowLong(x, y, z)
#define SetWindowLongPtr(x, y, z) SetWindowLong(x, y, z)
#define GetWindowLongPtr(x, y) GetWindowLong(x, y)
#define LONG_PTR LONG
#endif

#include "NativeScrollParameters.h"
#include <WebCore/ActivePlatformGestureAnimation.h>
#include <WebCore/TouchpadFlingPlatformGestureCurve.h>
#include <WebCore/ScrollbarThemeOpus.h>

#if USE(CG)
#include <CoreGraphics/CGContext.h>
#endif

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#endif

#if USE(CFNETWORK)
#include <CFNetwork/CFURLCachePriv.h>
#include <CFNetwork/CFURLProtocolPriv.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h> 
#endif

#if USE(ACCELERATED_COMPOSITING)
#include <WebCore/CACFLayerTreeHost.h>
#include <WebCore/PlatformCALayer.h>
#endif

#if ENABLE(FULLSCREEN_API)
#include <WebCore/FullScreenController.h>
#endif

#if ENABLE(INSPECTOR)
#include "InspectorController.h"
#endif

#if ENABLE(INSPECTOR_SERVER)
#include "InspectorServer/WebInspectorServer.h"
#endif

#include <ShlObj.h>
#include <comutil.h>
#include <dimm.h>
#include <oleacc.h>
#include <wchar.h>
#include <windowsx.h>
#include <wtf/HashSet.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

#if ENABLE(DRAG_SUPPORT) && (OS(WINCE) || PLATFORM(HP))
#include <wince/DragDropManager.h>
#endif

#if PLATFORM(HP)
#include <wtf/StringExtras.h>
#endif

#if USE(SIMULATED_GESTURES)
#include <wince/PlatformGestureSupport.h>
#else
// Soft link functions for gestures and panning feedback
SOFT_LINK_LIBRARY(USER32);
SOFT_LINK_OPTIONAL(USER32, GetGestureInfo, BOOL, WINAPI, (HGESTUREINFO, PGESTUREINFO));
SOFT_LINK_OPTIONAL(USER32, SetGestureConfig, BOOL, WINAPI, (HWND, DWORD, UINT, PGESTURECONFIG, UINT));
SOFT_LINK_OPTIONAL(USER32, CloseGestureInfoHandle, BOOL, WINAPI, (HGESTUREINFO));
SOFT_LINK_LIBRARY(Uxtheme);
SOFT_LINK_OPTIONAL(Uxtheme, BeginPanningFeedback, BOOL, WINAPI, (HWND));
SOFT_LINK_OPTIONAL(Uxtheme, EndPanningFeedback, BOOL, WINAPI, (HWND, BOOL));
SOFT_LINK_OPTIONAL(Uxtheme, UpdatePanningFeedback, BOOL, WINAPI, (HWND, LONG, LONG, BOOL));
#endif

using namespace WebCore;
using namespace std;
using JSC::JSLock;

static HMODULE accessibilityLib;
static HashSet<WebView*> *pendingDeleteBackingStoreSet = NULL;

static String webKitVersionString();

template <class T>
static T snapToRange(T value, T min, T max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    return value;
}

WebView* kit(Page* page)
{
    if (!page)
        return 0;
    
    ChromeClient* chromeClient = page->chrome().client();
    if (chromeClient->isEmptyChromeClient())
        return 0;
    
    return static_cast<WebChromeClient*>(chromeClient)->webView();
}

static inline AtomicString toAtomicString(BSTR bstr)
{
    return AtomicString(bstr, SysStringLen(bstr));
}

static inline String toString(BSTR bstr)
{
    return String(bstr, SysStringLen(bstr));
}

static inline KURL toKURL(BSTR bstr)
{
    return KURL(KURL(), toString(bstr));
}

class PreferencesChangedOrRemovedObserver : public IWebNotificationObserver {
public:
    static PreferencesChangedOrRemovedObserver* sharedInstance();

private:
    PreferencesChangedOrRemovedObserver() {}
    ~PreferencesChangedOrRemovedObserver() {}

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) { return E_FAIL; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return 0; }

public:
    // IWebNotificationObserver
    virtual HRESULT STDMETHODCALLTYPE onNotify( 
        /* [in] */ IWebNotification* notification);

private:
    HRESULT notifyPreferencesChanged(WebCacheModel);
    HRESULT notifyPreferencesRemoved(WebCacheModel);
};

PreferencesChangedOrRemovedObserver* PreferencesChangedOrRemovedObserver::sharedInstance()
{
    static PreferencesChangedOrRemovedObserver* shared = new PreferencesChangedOrRemovedObserver;
    return shared;
}

HRESULT PreferencesChangedOrRemovedObserver::onNotify(IWebNotification* notification)
{
    HRESULT hr = S_OK;

    COMPtr<IUnknown> unkPrefs;
    hr = notification->getObject(&unkPrefs);
    if (FAILED(hr))
        return hr;

    COMPtr<IWebPreferences> preferences(Query, unkPrefs);
    if (!preferences)
        return E_NOINTERFACE;

    WebCacheModel cacheModel;
    hr = preferences->cacheModel(&cacheModel);
    if (FAILED(hr))
        return hr;

    BString name;
    hr = notification->name(&name);
    if (FAILED(hr))
        return hr;

    if (wcscmp(name, WebPreferences::webPreferencesChangedNotification()) == 0)
        return notifyPreferencesChanged(cacheModel);

    if (wcscmp(name, WebPreferences::webPreferencesRemovedNotification()) == 0)
        return notifyPreferencesRemoved(cacheModel);

    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT PreferencesChangedOrRemovedObserver::notifyPreferencesChanged(WebCacheModel cacheModel)
{
    HRESULT hr = S_OK;

    if (!WebView::didSetCacheModel() || cacheModel > WebView::cacheModel())
        WebView::setCacheModel(cacheModel);
    else if (cacheModel < WebView::cacheModel()) {
        WebCacheModel sharedPreferencesCacheModel;
        hr = WebPreferences::sharedStandardPreferences()->cacheModel(&sharedPreferencesCacheModel);
        if (FAILED(hr))
            return hr;
        WebView::setCacheModel(max(sharedPreferencesCacheModel, WebView::maxCacheModelInAnyInstance()));
    }

    return hr;
}

HRESULT PreferencesChangedOrRemovedObserver::notifyPreferencesRemoved(WebCacheModel cacheModel)
{
    HRESULT hr = S_OK;

    if (cacheModel == WebView::cacheModel()) {
        WebCacheModel sharedPreferencesCacheModel;
        hr = WebPreferences::sharedStandardPreferences()->cacheModel(&sharedPreferencesCacheModel);
        if (FAILED(hr))
            return hr;
        WebView::setCacheModel(max(sharedPreferencesCacheModel, WebView::maxCacheModelInAnyInstance()));
    }

    return hr;
}


const LPCWSTR kWebViewWindowClassName = L"WebViewWindowClass";

const int WM_XP_THEMECHANGED = 0x031A;
const int WM_VISTA_MOUSEHWHEEL = 0x020E;

static const int maxToolTipWidth = 250;

static const int delayBeforeDeletingBackingStoreMsec = 5000;

static ATOM registerWebView();

static void initializeStaticObservers();

static HRESULT updateSharedSettingsFromPreferencesIfNeeded(IWebPreferences*);

HRESULT createMatchEnumerator(Vector<IntRect>* rects, IEnumTextMatches** matches);

static bool continuousSpellCheckingEnabled;
static bool grammarCheckingEnabled;

static bool s_didSetCacheModel;
static WebCacheModel s_cacheModel = WebCacheModelDocumentViewer;

enum {
    UpdateActiveStateTimer = 1,
    DeleteBackingStoreTimer = 2,
};

// WebView ----------------------------------------------------------------

bool WebView::s_allowSiteSpecificHacks = false;

WebView::WebView()
    : m_refCount(0)
    , m_shouldInvertColors(false)
#if !ASSERT_DISABLED
    , m_deletionHasBegun(false)
#endif
    , m_hostWindow(0)
    , m_viewWindow(0)
    , m_mainFrame(0)
    , m_page(0)
#if ENABLE(INSPECTOR)
    , m_inspectorClient(0)
#endif // ENABLE(INSPECTOR)
#if ENABLE(DRAG_SUPPORT)
    , m_hasCustomDropTarget(false)
    , m_dragData(0)
#endif
    , m_useBackForwardList(true)
    , m_userAgentOverridden(false)
    , m_zoomMultiplier(1.0f)
    , m_zoomsTextOnly(false)
    , m_mouseActivated(false)
    , m_currentCharacterCode(0)
    , m_isBeingDestroyed(false)
    , m_paintCount(0)
    , m_hasSpellCheckerDocumentTag(false)
    , m_didClose(false)
    , m_inIMEComposition(0)
    , m_toolTipHwnd(0)
    , m_closeWindowTimer(0)
#if !OS(WINCE)
    , m_topLevelParent(0)
    , m_transparent(false)
    , m_usesLayeredWindow(false)
#endif
    , m_deleteBackingStoreTimerActive(false)
    , m_lastPanX(0)
    , m_lastPanY(0)
    , m_gestureStartTime(0)
    , m_gestureInProgress(false)
    , m_mouseEventHandled(false)
    , m_animationTimer(this, &WebView::animationTimerFired)
    , m_xOverpan(0)
    , m_yOverpan(0)
    , m_xOverpanLast(0)
    , m_yOverpanLast(0)
    , m_fadingScrollbar(0)
#if USE(ACCELERATED_COMPOSITING)
    , m_isAcceleratedCompositing(false)
#endif
    , m_nextDisplayIsSynchronous(false)
    , m_lastSetCursor(0)
    , m_paintPending(false)
    , m_mouseProcessMode(WebKitMouseMove_Normal)
{
    if (!pendingDeleteBackingStoreSet)
        pendingDeleteBackingStoreSet = new HashSet<WebView*>();
    JSC::initializeThreading();
    WTF::initializeMainThread();

    m_backingStoreSize.cx = m_backingStoreSize.cy = 0;

#if ENABLE(DRAG_SUPPORT) && !OS(WINCE) && !PLATFORM(HP)
    CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper,(void**)&m_dropTargetHelper);
#endif

    initializeStaticObservers();

    WebPreferences* sharedPreferences = WebPreferences::sharedStandardPreferences();
    BOOL enabled;
    if (SUCCEEDED(sharedPreferences->continuousSpellCheckingEnabled(&enabled)))
        continuousSpellCheckingEnabled = !!enabled;
    if (SUCCEEDED(sharedPreferences->grammarCheckingEnabled(&enabled)))
        grammarCheckingEnabled = !!enabled;

    WebViewCount++;
    gClassCount++;
    gClassNameCount.add("WebView");

#if PLATFORM(HP)
    HPRegisterMemoryDebug(OutputLiveWebViewInfo);
#endif

    // In http://trac.webkit.org/changeset/140321, when emulate touch events is enabled,
    // mouse move events are not longer sent if the cursor is not held down.
    // This prevents m_lastSetCursor from ever being initialized.
    m_lastSetCursor = ::LoadCursor(0, IDC_ARROW);
}

WebView::~WebView()
{
    deleteBackingStore();

    // the tooltip window needs to be explicitly destroyed since it isn't a WS_CHILD
    if (::IsWindow(m_toolTipHwnd))
        ::DestroyWindow(m_toolTipHwnd);

    ASSERT(!m_page);
    ASSERT(!m_preferences);
    ASSERT(!m_viewWindow);

#if USE(ACCELERATED_COMPOSITING)
    ASSERT(!m_layerTreeHost);
    ASSERT(!m_backingLayer);
#endif

    WebViewCount--;
    gClassCount--;
    gClassNameCount.remove("WebView");
}

WebView* WebView::createInstance()
{
    WebView* instance = new WebView();
    instance->AddRef();
    return instance;
}

void initializeStaticObservers()
{
    static bool initialized;
    if (initialized)
        return;
    initialized = true;

    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->addObserver(PreferencesChangedOrRemovedObserver::sharedInstance(), WebPreferences::webPreferencesChangedNotification(), 0);
    notifyCenter->addObserver(PreferencesChangedOrRemovedObserver::sharedInstance(), WebPreferences::webPreferencesRemovedNotification(), 0);
}

static HashSet<WebView*>& allWebViewsSet()
{
    static HashSet<WebView*> allWebViewsSet;
    return allWebViewsSet;
}

void WebView::addToAllWebViewsSet()
{
    allWebViewsSet().add(this);
}

void WebView::removeFromAllWebViewsSet()
{
    allWebViewsSet().remove(this);
}

void WebView::setCacheModel(WebCacheModel cacheModel)
{
#if USE(CFNETWORK)
    if (s_didSetCacheModel && cacheModel == s_cacheModel)
        return;

    RetainPtr<CFURLCacheRef> cfurlCache = adoptCF(CFURLCacheCopySharedURLCache());
    RetainPtr<CFStringRef> cfurlCacheDirectory = adoptCF(wkCopyFoundationCacheDirectory(0));
    if (!cfurlCacheDirectory)
        cfurlCacheDirectory = WebCore::localUserSpecificStorageDirectory().createCFString();

    // As a fudge factor, use 1000 instead of 1024, in case the reported byte 
    // count doesn't align exactly to a megabyte boundary.
    unsigned long long memSize = WebMemorySize() / 1024 / 1000;
    unsigned long long diskFreeSize = WebVolumeFreeSize(cfurlCacheDirectory.get()) / 1024 / 1000;

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;

    unsigned pageCacheCapacity = 0;

    CFIndex cfurlCacheMemoryCapacity = 0;
    CFIndex cfurlCacheDiskCapacity = 0;

    switch (cacheModel) {
    case WebCacheModelDocumentViewer: {
        // Page cache capacity (in pages)
        pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024; 

        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;

        // Foundation memory cache capacity (in bytes)
        cfurlCacheMemoryCapacity = 0;

        // Foundation disk cache capacity (in bytes)
        cfurlCacheDiskCapacity = CFURLCacheDiskCapacity(cfurlCache.get());

        break;
    }
    case WebCacheModelDocumentBrowser: {
        // Page cache capacity (in pages)
        if (memSize >= 1024)
            pageCacheCapacity = 3;
        else if (memSize >= 512)
            pageCacheCapacity = 2;
        else if (memSize >= 256)
            pageCacheCapacity = 1;
        else
            pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024; 

        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;

        // Foundation memory cache capacity (in bytes)
        if (memSize >= 2048)
            cfurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 1024)
            cfurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 512)
            cfurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            cfurlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            cfurlCacheDiskCapacity = 50 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            cfurlCacheDiskCapacity = 40 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            cfurlCacheDiskCapacity = 30 * 1024 * 1024;
        else
            cfurlCacheDiskCapacity = 20 * 1024 * 1024;

        break;
    }
    case WebCacheModelPrimaryWebBrowser: {
        // Page cache capacity (in pages)
        // (Research indicates that value / page drops substantially after 3 pages.)
        if (memSize >= 2048)
            pageCacheCapacity = 5;
        else if (memSize >= 1024)
            pageCacheCapacity = 4;
        else if (memSize >= 512)
            pageCacheCapacity = 3;
        else if (memSize >= 256)
            pageCacheCapacity = 2;
        else
            pageCacheCapacity = 1;

        // Object cache capacities (in bytes)
        // (Testing indicates that value / MB depends heavily on content and
        // browsing pattern. Even growth above 128MB can have substantial 
        // value / MB for some content / browsing patterns.)
        if (memSize >= 2048)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 32 * 1024 * 1024; 

        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;

        // This code is here to avoid a PLT regression. We can remove it if we
        // can prove that the overall system gain would justify the regression.
        cacheMaxDeadCapacity = max(24u, cacheMaxDeadCapacity);

        deadDecodedDataDeletionInterval = 60;

        // Foundation memory cache capacity (in bytes)
        // (These values are small because WebCore does most caching itself.)
        if (memSize >= 1024)
            cfurlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memSize >= 512)
            cfurlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memSize >= 256)
            cfurlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            cfurlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            cfurlCacheDiskCapacity = 175 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            cfurlCacheDiskCapacity = 150 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            cfurlCacheDiskCapacity = 125 * 1024 * 1024;
        else if (diskFreeSize >= 2048)
            cfurlCacheDiskCapacity = 100 * 1024 * 1024;
        else if (diskFreeSize >= 1024)
            cfurlCacheDiskCapacity = 75 * 1024 * 1024;
        else
            cfurlCacheDiskCapacity = 50 * 1024 * 1024;

        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }

    // Don't shrink a big disk cache, since that would cause churn.
    cfurlCacheDiskCapacity = max(cfurlCacheDiskCapacity, CFURLCacheDiskCapacity(cfurlCache.get()));

    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);

    CFURLCacheSetMemoryCapacity(cfurlCache.get(), cfurlCacheMemoryCapacity);
    CFURLCacheSetDiskCapacity(cfurlCache.get(), cfurlCacheDiskCapacity);

    s_didSetCacheModel = true;
    s_cacheModel = cacheModel;
    return;
#endif
}

WebCacheModel WebView::cacheModel()
{
    return s_cacheModel;
}

bool WebView::didSetCacheModel()
{
    return s_didSetCacheModel;
}

WebCacheModel WebView::maxCacheModelInAnyInstance()
{
    WebCacheModel cacheModel = WebCacheModelDocumentViewer;

    HashSet<WebView*>::iterator end = allWebViewsSet().end();
    for (HashSet<WebView*>::iterator it = allWebViewsSet().begin(); it != end; ++it) {
        COMPtr<IWebPreferences> pref;
        if (FAILED((*it)->preferences(&pref)))
            continue;
        WebCacheModel prefCacheModel = WebCacheModelDocumentViewer;
        if (FAILED(pref->cacheModel(&prefCacheModel)))
            continue;

        cacheModel = max(cacheModel, prefCacheModel);
    }

    return cacheModel;
}

HRESULT STDMETHODCALLTYPE WebView::close()
{
    if (m_didClose)
        return S_OK;

    m_didClose = true;

#if USE(ACCELERATED_COMPOSITING)
    setAcceleratedCompositing(false);
#endif

    WebNotificationCenter::defaultCenterInternal()->postNotificationName(BString(WebViewWillCloseNotification).release(), static_cast<IWebView*>(this), 0);

    if (m_uiDelegatePrivate)
        m_uiDelegatePrivate->webViewClosing(this);

    removeFromAllWebViewsSet();

    if (m_page) {
        if (Frame* frame = m_page->mainFrame()) {
            frame->loader()->detachFromParent();
            // If we are closing the last webview, let's release executable memory now.  This
            // will reduce the appearance of memory leaks.  (JSC bytecode, cached regular expressions...)
            if (allWebViewsSet().size() == 0) {
                JSC::ExecState * exec = frame->script()->globalObject(mainThreadNormalWorld())->globalExec();
                JSC::JSLockHolder lock(exec);
                exec->vm().releaseExecutableMemory();
            }
        }
    }

#if !OS(WINCE)
    if (m_mouseOutTracker) {
        m_mouseOutTracker->dwFlags = TME_CANCEL;
        ::TrackMouseEvent(m_mouseOutTracker.get());
        m_mouseOutTracker.clear();
    }
#endif

#if ENABLE(DRAG_SUPPORT)
    revokeDragDrop();
#endif

    if (m_viewWindow) {
#if USE(CE_GESTURE_MODEL)
        DisableGesturesPtr()(m_viewWindow, TGF_GID_PAN | TGF_GID_SCROLL, TGF_SCOPE_WINDOW);
#endif
        // We can't check IsWindow(m_viewWindow) here, because that will return true even while
        // we're already handling WM_DESTROY. So we check !isBeingDestroyed() instead.
        if (!isBeingDestroyed())
            DestroyWindow(m_viewWindow);
        // Either we just destroyed m_viewWindow, or it's in the process of being destroyed. Either
        // way, we clear it out to make sure we don't try to use it later.
        m_viewWindow = 0;
    }

    setHostWindow(0);

    setDownloadDelegate(0);
    setEditingDelegate(0);
    setFrameLoadDelegate(0);
    setFrameLoadDelegatePrivate(0);
    setHistoryDelegate(0);
    setPolicyDelegate(0);
    setResourceLoadDelegate(0);
    setUIDelegate(0);
    setFormDelegate(0);

#if ENABLE(INSPECTOR)
    m_inspectorClient = 0;
    if (m_webInspector)
        m_webInspector->webViewClosed();
#endif // ENABLE(INSPECTOR)

    delete m_page;
    m_page = 0;

    registerForIconNotification(false);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->removeObserver(this, WebPreferences::webPreferencesChangedNotification(), static_cast<IWebPreferences*>(m_preferences.get()));

    if (COMPtr<WebPreferences> preferences = m_preferences) {
        BString identifier;
        preferences->identifier(&identifier);

        m_preferences = 0;
        preferences->didRemoveFromWebView();
        // Make sure we release the reference, since WebPreferences::removeReferenceForIdentifier will check for last reference to WebPreferences
        preferences = 0;
        if (identifier)
            WebPreferences::removeReferenceForIdentifier(identifier);
    }

    deleteBackingStore();
    return S_OK;
}

void WebView::repaint(const WebCore::IntRect& windowRect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
#if USE(ACCELERATED_COMPOSITING)
    if (isAcceleratedCompositing()) {
        // The contentChanged, immediate, and repaintContentOnly parameters are all based on a non-
        // compositing painting/scrolling model.
        addToDirtyRegion(windowRect);
        return;
    }
#endif
    m_paintPending = true;

    if (!repaintContentOnly) {
        RECT rect = windowRect;
        ::InvalidateRect(m_viewWindow, &rect, false);
    }
    if (contentChanged)
        addToDirtyRegion(windowRect);
    if (immediate) {
        if (repaintContentOnly)
            updateBackingStore(core(topLevelFrame())->view());
        else
            ::UpdateWindow(m_viewWindow);
    }
}

void WebView::deleteBackingStore()
{
    pendingDeleteBackingStoreSet->remove(this);

    if (m_deleteBackingStoreTimerActive) {
        KillTimer(m_viewWindow, DeleteBackingStoreTimer);
        m_deleteBackingStoreTimerActive = false;
    }
    m_backingStoreBitmap.clear();
    m_backingStoreDirtyRegion.clear();
    m_backingStoreSize.cx = m_backingStoreSize.cy = 0;
}

bool WebView::ensureBackingStore()
{
    RECT windowRect;
    ::GetClientRect(m_viewWindow, &windowRect);
    LONG width = windowRect.right - windowRect.left;
    LONG height = windowRect.bottom - windowRect.top;
    if (width > 0 && height > 0 && (width != m_backingStoreSize.cx || height != m_backingStoreSize.cy)) {
        deleteBackingStore();

        m_backingStoreSize.cx = width;
        m_backingStoreSize.cy = height;
        BitmapInfo bitmapInfo = BitmapInfo::createBottomUp(IntSize(m_backingStoreSize));

        void* pixels = NULL;
        m_backingStoreBitmap = RefCountedHBITMAP::create(::CreateDIBSection(0, &bitmapInfo, DIB_RGB_COLORS, &pixels, 0, 0));
        return true;
    }

    return false;
}

void WebView::addToDirtyRegion(const IntRect& dirtyRect)
{
    // FIXME: We want an assert here saying that the dirtyRect is inside the clienRect,
    // but it was being hit during our layout tests, and is being investigated in
    // http://webkit.org/b/29350.

#if USE(ACCELERATED_COMPOSITING)
    if (isAcceleratedCompositing()) {
        m_backingLayer->setNeedsDisplayInRect(dirtyRect);
        return;
    }
#endif

    HRGN newRegion = ::CreateRectRgn(dirtyRect.x(), dirtyRect.y(),
                                     dirtyRect.maxX(), dirtyRect.maxY());
    addToDirtyRegion(newRegion);
}

void WebView::addToDirtyRegion(HRGN newRegion)
{
#if USE(ACCELERATED_COMPOSITING)
    ASSERT(!isAcceleratedCompositing());
#endif

    LOCAL_GDI_COUNTER(0, __FUNCTION__);

    if (m_backingStoreDirtyRegion) {
        HRGN combinedRegion = ::CreateRectRgn(0,0,0,0);
        ::CombineRgn(combinedRegion, m_backingStoreDirtyRegion->handle(), newRegion, RGN_OR);
        ::DeleteObject(newRegion);
        m_backingStoreDirtyRegion = RefCountedHRGN::create(combinedRegion);
    } else
        m_backingStoreDirtyRegion = RefCountedHRGN::create(newRegion);

    if (m_uiDelegatePrivate)
        m_uiDelegatePrivate->webViewDidInvalidate(this);
}

void WebView::scrollBackingStore(FrameView* frameView, int dx, int dy, const IntRect& scrollViewRect, const IntRect& clipRect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (isAcceleratedCompositing()) {
        // FIXME: We should be doing something smarter here, like moving tiles around and painting
        // any newly-exposed tiles. <http://webkit.org/b/52714>
        m_backingLayer->setNeedsDisplayInRect(scrollViewRect);
        return;
    }
#endif

    LOCAL_GDI_COUNTER(0, __FUNCTION__);

    // If there's no backing store we don't need to update it
    if (!m_backingStoreBitmap) {
        if (m_uiDelegatePrivate)
            m_uiDelegatePrivate->webViewScrolled(this);

        return;
    }

    // Make a region to hold the invalidated scroll area.
    HRGN updateRegion = ::CreateRectRgn(0, 0, 0, 0);

    // Collect our device context info and select the bitmap to scroll.
    HWndDC windowDC(m_viewWindow);
    HDC bitmapDC = ::CreateCompatibleDC(windowDC);
    HGDIOBJ oldBitmap = ::SelectObject(bitmapDC, m_backingStoreBitmap->handle());
    
    // Scroll the bitmap.
    RECT scrollRectWin(scrollViewRect);
    RECT clipRectWin(clipRect);
    ::ScrollDC(bitmapDC, dx, dy, &scrollRectWin, &clipRectWin, updateRegion, 0);
    RECT regionBox;
    ::GetRgnBox(updateRegion, &regionBox);

#if !OS(WINCE)
    // Flush.
    GdiFlush();
#endif

    // Add the dirty region to the backing store's dirty region.
    addToDirtyRegion(updateRegion);

    if (m_uiDelegatePrivate)
        m_uiDelegatePrivate->webViewScrolled(this);

    // Update the backing store.
    updateBackingStore(frameView, bitmapDC, false);

    // Clean up.
    ::SelectObject(bitmapDC, oldBitmap);
    ::DeleteDC(bitmapDC);
}

void WebView::sizeChanged(const IntSize& newSize)
{
    deleteBackingStore();

    if (Frame* coreFrame = core(topLevelFrame()))
        coreFrame->view()->resize(newSize);

#if USE(ACCELERATED_COMPOSITING)
    if (m_layerTreeHost)
        m_layerTreeHost->resize();
    if (m_backingLayer) {
        m_backingLayer->setSize(newSize);
        m_backingLayer->setNeedsDisplay();
    }
#endif
}

// This emulates the Mac smarts for painting rects intelligently.  This is very
// important for us, since we double buffer based off dirty rects.
static void getUpdateRects(HRGN region, const IntRect& dirtyRect, Vector<IntRect>& rects)
{
    ASSERT_ARG(region, region);

    const int cRectThreshold = 10;
    const float cWastedSpaceThreshold = 0.75f;

    rects.clear();

    DWORD regionDataSize = GetRegionData(region, sizeof(RGNDATA), NULL);
    if (!regionDataSize) {
        rects.append(dirtyRect);
        return;
    }

    Vector<unsigned char> buffer(regionDataSize);
    RGNDATA* regionData = reinterpret_cast<RGNDATA*>(buffer.data());
    GetRegionData(region, regionDataSize, regionData);
    if (regionData->rdh.nCount > cRectThreshold) {
        rects.append(dirtyRect);
        return;
    }

    double singlePixels = 0.0;
    unsigned i;
    RECT* rect;
    for (i = 0, rect = reinterpret_cast<RECT*>(regionData->Buffer); i < regionData->rdh.nCount; i++, rect++)
        singlePixels += (rect->right - rect->left) * (rect->bottom - rect->top);

    double unionPixels = dirtyRect.width() * dirtyRect.height();
    double wastedSpace = 1.0 - (singlePixels / unionPixels);
    if (wastedSpace <= cWastedSpaceThreshold) {
        rects.append(dirtyRect);
        return;
    }

    for (i = 0, rect = reinterpret_cast<RECT*>(regionData->Buffer); i < regionData->rdh.nCount; i++, rect++)
        rects.append(*rect);
}

void WebView::updateBackingStore(FrameView* frameView, HDC dc, bool backingStoreCompletelyDirty, WindowsToPaint windowsToPaint)
{
#if USE(ACCELERATED_COMPOSITING)
    ASSERT(!isAcceleratedCompositing());
#endif

    LOCAL_GDI_COUNTER(0, __FUNCTION__);

    HDC bitmapDC = dc;
    HGDIOBJ oldBitmap = 0;
    if (!dc) {
        HWndDC windowDC(m_viewWindow);
        bitmapDC = ::CreateCompatibleDC(windowDC);
        oldBitmap = ::SelectObject(bitmapDC, m_backingStoreBitmap->handle());
    }

    if (m_backingStoreBitmap && (m_backingStoreDirtyRegion || backingStoreCompletelyDirty)) {
        // Do a layout first so that everything we render to the backing store is always current.
        if (Frame* coreFrame = core(m_mainFrame))
            if (FrameView* view = coreFrame->view())
                view->updateLayoutAndStyleIfNeededRecursive();

        Vector<IntRect> paintRects;
        if (!backingStoreCompletelyDirty && m_backingStoreDirtyRegion) {
            RECT regionBox;
            ::GetRgnBox(m_backingStoreDirtyRegion->handle(), &regionBox);
            getUpdateRects(m_backingStoreDirtyRegion->handle(), regionBox, paintRects);
        } else {
            RECT clientRect;
            ::GetClientRect(m_viewWindow, &clientRect);
            paintRects.append(clientRect);
        }

        for (unsigned i = 0; i < paintRects.size(); ++i)
            paintIntoBackingStore(frameView, bitmapDC, paintRects[i], windowsToPaint);

        if (m_uiDelegatePrivate)
            m_uiDelegatePrivate->webViewPainted(this);

        m_backingStoreDirtyRegion.clear();
    }

    if (!dc) {
        ::SelectObject(bitmapDC, oldBitmap);
        ::DeleteDC(bitmapDC);
    }

#if !OS(WINCE)
    GdiFlush();
#endif
}

#if !OS(WINCE)
void WebView::performLayeredWindowUpdate()
{
    // The backing store may have been destroyed if the window rect was set to zero height or zero width.
    if (!m_backingStoreBitmap)
        return;

    HWndDC hdcScreen(m_viewWindow);
    OwnPtr<HDC> hdcMem = adoptPtr(::CreateCompatibleDC(hdcScreen));
    HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdcMem.get(), m_backingStoreBitmap->handle()));

    BITMAP bmpInfo;
    ::GetObject(m_backingStoreBitmap->handle(), sizeof(bmpInfo), &bmpInfo);
    SIZE windowSize = { bmpInfo.bmWidth, bmpInfo.bmHeight };

    BLENDFUNCTION blendFunction;
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.BlendFlags = 0;
    blendFunction.SourceConstantAlpha = 0xFF;
    blendFunction.AlphaFormat = AC_SRC_ALPHA;

    POINT layerPos = { 0, 0 };
    ::UpdateLayeredWindow(m_viewWindow, hdcScreen, 0, &windowSize, hdcMem.get(), &layerPos, 0, &blendFunction, ULW_ALPHA);

    ::SelectObject(hdcMem.get(), hbmOld);
}
#endif

void WebView::paint(HDC dc, LPARAM options)
{
    LOCAL_GDI_COUNTER(0, __FUNCTION__);

#if USE(ACCELERATED_COMPOSITING)
    if (isAcceleratedCompositing() && !usesLayeredWindow()) {
        m_layerTreeHost->flushPendingLayerChangesNow();
        // Flushing might have taken us out of compositing mode.
        if (isAcceleratedCompositing()) {
            // FIXME: We need to paint into dc (if provided). <http://webkit.org/b/52578>
            m_layerTreeHost->paint();
            ::ValidateRect(m_viewWindow, 0);
            return;
        }
    }
#endif

    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return;
    FrameView* frameView = coreFrame->view();

    RECT rcPaint;
    HDC hdc;
    OwnPtr<HRGN> region;
    int regionType = NULLREGION;
    PAINTSTRUCT ps;
    WindowsToPaint windowsToPaint;
    if (!dc) {
        region = adoptPtr(CreateRectRgn(0,0,0,0));
        regionType = GetUpdateRgn(m_viewWindow, region.get(), false);
        hdc = BeginPaint(m_viewWindow, &ps);
        rcPaint = ps.rcPaint;
        // We're painting to the screen, and our child windows can handle
        // painting themselves to the screen.
        windowsToPaint = PaintWebViewOnly;
    } else {
        hdc = dc;
        ::GetClientRect(m_viewWindow, &rcPaint);
#if !OS(WINCE)
        if (options & PRF_ERASEBKGND)
            ::FillRect(hdc, &rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));
#endif
        // Since we aren't painting to the screen, we want to paint all our
        // children into the HDC.
        windowsToPaint = PaintWebViewAndChildren;
    }

    bool backingStoreCompletelyDirty = ensureBackingStore();
    if (!m_backingStoreBitmap) {
        if (!dc)
            EndPaint(m_viewWindow, &ps);
        return;
    }

    m_paintCount++;

    HDC bitmapDC = ::CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap = ::SelectObject(bitmapDC, m_backingStoreBitmap->handle());

    // Update our backing store if needed.
    updateBackingStore(frameView, bitmapDC, backingStoreCompletelyDirty, windowsToPaint);

    // Now we blit the updated backing store
    IntRect windowDirtyRect = rcPaint;
    
    // Apply the same heuristic for this update region too.
    Vector<IntRect> blitRects;
    if (region && regionType == COMPLEXREGION)
        getUpdateRects(region.get(), windowDirtyRect, blitRects);
    else
        blitRects.append(windowDirtyRect);

    for (unsigned i = 0; i < blitRects.size(); ++i)
        paintIntoWindow(bitmapDC, hdc, blitRects[i]);

    ::SelectObject(bitmapDC, oldBitmap);
    ::DeleteDC(bitmapDC);

    if (!dc)
        EndPaint(m_viewWindow, &ps);

    m_paintCount--;

#if !PLATFORM(HP)
    // Don't delete the backing store... we want to be able to get screen dumps even if the web view isn't presently visible.
    // We may have to revisit this as it will use more memory.
    // Unfortunately, CE doesn't know about the WM_PRINTCLIENT message, which is how DRT does this.
    if (active())
        cancelDeleteBackingStoreSoon();
    else
        deleteBackingStoreSoon();
#endif

    m_paintPending = false;
}

void WebView::paintIntoBackingStore(FrameView* frameView, HDC bitmapDC, const IntRect& dirtyRect, WindowsToPaint windowsToPaint)
{
    // FIXME: This function should never be called in accelerated compositing mode, and we should
    // assert as such. But currently it *is* sometimes called, so we can't assert yet. See
    // <http://webkit.org/b/58539>.

    LOCAL_GDI_COUNTER(0, __FUNCTION__);

    // FIXME: We want an assert here saying that the dirtyRect is inside the clienRect,
    // but it was being hit during our layout tests, and is being investigated in
    // http://webkit.org/b/29350.

    RECT rect = dirtyRect;

#if FLASH_BACKING_STORE_REDRAW
    {
        HWndDC dc(m_viewWindow);
        OwnPtr<HBRUSH> yellowBrush(CreateSolidBrush(RGB(255, 255, 0)));
        FillRect(dc, &rect, yellowBrush.get());
        GdiFlush();
        Sleep(50);
        paintIntoWindow(bitmapDC, dc, dirtyRect);
    }
#endif

#if OS(WINCE)
    GraphicsContext gc(bitmapDC);
#else
    GraphicsContext gc(bitmapDC, m_transparent);
    gc.setShouldIncludeChildWindows(windowsToPaint == PaintWebViewAndChildren);
#endif
    gc.save();
#if !OS(WINCE)
    if (m_transparent)
        gc.clearRect(dirtyRect);
#endif
    // I think we end up painting the whole view area multiple times, this
    // doesn't seem to be necessary.
    //else
    //    FillRect(bitmapDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

    COMPtr<IWebUIDelegatePrivate2> uiPrivate(Query, m_uiDelegate);
    if (uiPrivate)
        uiPrivate->drawBackground(this, reinterpret_cast<OLE_HANDLE>(bitmapDC), &rect);

    if (frameView && frameView->frame() && frameView->frame()->contentRenderer()) {
        gc.clip(dirtyRect);
        frameView->paint(&gc, dirtyRect);
        if (m_shouldInvertColors)
            gc.fillRect(dirtyRect, Color::white, ColorSpaceDeviceRGB, CompositeDifference);

        paintOverpanFeedback(&gc, dirtyRect);

#if ENABLE(DRAG_SUPPORT) && (OS(WINCE) || PLATFORM(HP))
        WinCE_PaintDragImage(bitmapDC, this, &rect);
#endif

#if OS(WINCE) && ENABLE(INSPECTOR)
        // WinCE does not support layered/transparent windows,
        // so we just draw the highlighted node as part of the
        // WebView paint.
        // Normally this is drawing is down with a separate transparent
        // layered child window in WebNodeHighlight.cpp.
        if (frameView->frame()->page()->inspectorController()->highlightedNode()) {
            frameView->frame()->page()->inspectorController()->drawHighlight(gc);
        }
#endif
    }
    gc.restore();
}

void WebView::paintIntoWindow(HDC bitmapDC, HDC windowDC, const IntRect& dirtyRect)
{
    // FIXME: This function should never be called in accelerated compositing mode, and we should
    // assert as such. But currently it *is* sometimes called, so we can't assert yet. See
    // <http://webkit.org/b/58539>.

    LOCAL_GDI_COUNTER(0, __FUNCTION__);
#if FLASH_WINDOW_REDRAW
    OwnPtr<HBRUSH> greenBrush = CreateSolidBrush(RGB(0, 255, 0));
    RECT rect = dirtyRect;
    FillRect(windowDC, &rect, greenBrush.get());
    GdiFlush();
    Sleep(50);
#endif

    // Blit the dirty rect from the backing store into the same position
    // in the destination DC.
    BitBlt(windowDC, dirtyRect.x(), dirtyRect.y(), dirtyRect.width(), dirtyRect.height(), bitmapDC,
           dirtyRect.x(), dirtyRect.y(), SRCCOPY);
}

void WebView::frameRect(RECT* rect)
{
    ::GetWindowRect(m_viewWindow, rect);
}

class WindowCloseTimer : public WebCore::SuspendableTimer {
public:
    static WindowCloseTimer* create(WebView*);

private:
    WindowCloseTimer(ScriptExecutionContext*, WebView*);
    virtual void contextDestroyed();
    virtual void fired();

    WebView* m_webView;
};

WindowCloseTimer* WindowCloseTimer::create(WebView* webView)
{
    ASSERT_ARG(webView, webView);
    Frame* frame = core(webView->topLevelFrame());
    ASSERT(frame);
    if (!frame)
        return 0;

    Document* document = frame->document();
    ASSERT(document);
    if (!document)
        return 0;

    return new WindowCloseTimer(document, webView);
}

WindowCloseTimer::WindowCloseTimer(ScriptExecutionContext* context, WebView* webView)
    : SuspendableTimer(context)
    , m_webView(webView)
{
    ASSERT_ARG(context, context);
    ASSERT_ARG(webView, webView);
}

void WindowCloseTimer::contextDestroyed()
{
    SuspendableTimer::contextDestroyed();
    delete this;
}

void WindowCloseTimer::fired()
{
    m_webView->closeWindowTimerFired();
}

void WebView::closeWindowSoon()
{
    if (m_closeWindowTimer)
        return;

    m_closeWindowTimer = WindowCloseTimer::create(this);
    if (!m_closeWindowTimer)
        return;
    m_closeWindowTimer->startOneShot(0);

    AddRef();
}

void WebView::closeWindowTimerFired()
{
    closeWindow();
    Release();
}

void WebView::closeWindow()
{
    if (m_hasSpellCheckerDocumentTag) {
        if (m_editingDelegate)
            m_editingDelegate->closeSpellDocument(this);
        m_hasSpellCheckerDocumentTag = false;
    }

    COMPtr<IWebUIDelegate> ui;
    if (SUCCEEDED(uiDelegate(&ui)))
        ui->webViewClose(this);
}

bool WebView::canHandleRequest(const WebCore::ResourceRequest& request)
{
    // On the mac there's an about url protocol implementation but CFNetwork doesn't have that.
    if (equalIgnoringCase(String(request.url().protocol()), "about"))
        return true;

#if USE(CFNETWORK)
    if (CFURLProtocolCanHandleRequest(request.cfURLRequest(UpdateHTTPBody)))
        return true;

    // FIXME: Mac WebKit calls _representationExistsForURLScheme here
    return false;
#else
    return true;
#endif
}

String WebView::standardUserAgentWithApplicationName(const String& applicationName)
{
    DEFINE_STATIC_LOCAL(String, osVersion, (windowsVersionForUAString()));
    DEFINE_STATIC_LOCAL(String, webKitVersion, (webKitVersionString()));

    return makeString("Mozilla/5.0 (", osVersion, ") AppleWebKit/", webKitVersion, " (KHTML, like Gecko)", applicationName.isEmpty() ? "" : " ", applicationName);
}

Page* WebView::page()
{
    return m_page;
}

#if ENABLE(CONTEXT_MENUS)
bool WebView::handleContextMenuEvent(WPARAM wParam, LPARAM lParam)
{
    // Translate the screen coordinates into window coordinates
    POINT coords = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    if (coords.x == -1 || coords.y == -1) {
        // The contextMenuController() holds onto the last context menu that was popped up on the
        // page until a new one is created. We need to clear this menu before propagating the event
        // through the DOM so that we can detect if we create a new menu for this event, since we
        // won't create a new menu if the DOM swallows the event and the defaultEventHandler does
        // not run.
        m_page->contextMenuController()->clearContextMenu();

        Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
        return focusedFrame->eventHandler()->sendContextMenuEventForKey();

    } else {
        if (!::ScreenToClient(m_viewWindow, &coords))
            return false;
    }

    lParam = MAKELPARAM(coords.x, coords.y);

    m_page->contextMenuController()->clearContextMenu();

    IntPoint documentPoint(m_page->mainFrame()->view()->windowToContents(coords));
    HitTestResult result = m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(documentPoint);
    Frame* targetFrame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document()->frame() : m_page->focusController()->focusedOrMainFrame();

    targetFrame->view()->setCursor(pointerCursor());
    PlatformMouseEvent mouseEvent(m_viewWindow, WM_RBUTTONUP, wParam, lParam);
    bool handledEvent = targetFrame->eventHandler()->sendContextMenuEvent(mouseEvent);
    if (!handledEvent)
        return false;

    ContextMenuController* contextMenuController = m_page->contextMenuController();

    // Show the menu
    ContextMenu* coreMenu = contextMenuController->contextMenu();
    if (!coreMenu)
        return false;

    Frame* frame = contextMenuController->hitTestResult().innerNodeFrame();
    if (!frame)
        return false;

    FrameView* view = frame->view();
    if (!view)
        return false;

    POINT point(view->contentsToWindow(contextMenuController->hitTestResult().roundedPointInInnerNodeFrame()));

    // Translate the point to screen coordinates
    if (!::ClientToScreen(m_viewWindow, &point))
        return false;

    BOOL hasCustomMenus = false;
    if (m_uiDelegate)
        m_uiDelegate->hasCustomMenuImplementation(&hasCustomMenus);

    if (hasCustomMenus)
        m_uiDelegate->trackCustomPopupMenu((IWebView*)this, (OLE_HANDLE)(ULONG64)coreMenu->platformContextMenu(), &point);
    else {
#if OS(WINCE)
        UINT flags = TPM_TOPALIGN | TPM_HORIZONTAL | TPM_LEFTALIGN;
#else
        // Surprisingly, TPM_RIGHTBUTTON means that items are selectable with either the right OR left mouse button
        UINT flags = TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_HORIZONTAL
            | TPM_LEFTALIGN | TPM_HORPOSANIMATION;
#endif
        ::TrackPopupMenuEx(coreMenu->platformContextMenu(), flags, point.x, point.y, m_viewWindow, 0);
    }

    return true;
}

bool WebView::onMeasureItem(WPARAM /*wParam*/, LPARAM lParam)
{
    if (!m_uiDelegate)
        return false;

    BOOL hasCustomMenus = false;
    m_uiDelegate->hasCustomMenuImplementation(&hasCustomMenus);
    if (!hasCustomMenus)
        return false;

    m_uiDelegate->measureCustomMenuItem((IWebView*)this, (void*)lParam);
    return true;
}

bool WebView::onDrawItem(WPARAM /*wParam*/, LPARAM lParam)
{
    if (!m_uiDelegate)
        return false;

    BOOL hasCustomMenus = false;
    m_uiDelegate->hasCustomMenuImplementation(&hasCustomMenus);
    if (!hasCustomMenus)
        return false;

    m_uiDelegate->drawCustomMenuItem((IWebView*)this, (void*)lParam);
    return true;
}

bool WebView::onInitMenuPopup(WPARAM wParam, LPARAM /*lParam*/)
{
    if (!m_uiDelegate)
        return false;

    HMENU menu = (HMENU)wParam;
    if (!menu)
        return false;

    BOOL hasCustomMenus = false;
    m_uiDelegate->hasCustomMenuImplementation(&hasCustomMenus);
    if (!hasCustomMenus)
        return false;

    m_uiDelegate->addCustomMenuDrawingData((IWebView*)this, (OLE_HANDLE)(ULONG64)menu);
    return true;
}

bool WebView::onUninitMenuPopup(WPARAM wParam, LPARAM /*lParam*/)
{
    if (!m_uiDelegate)
        return false;

    HMENU menu = (HMENU)wParam;
    if (!menu)
        return false;

    BOOL hasCustomMenus = false;
    m_uiDelegate->hasCustomMenuImplementation(&hasCustomMenus);
    if (!hasCustomMenus)
        return false;

    m_uiDelegate->cleanUpCustomMenuDrawingData((IWebView*)this, (OLE_HANDLE)(ULONG64)menu);
    return true;
}

void WebView::performContextMenuAction(WPARAM wParam, LPARAM lParam, bool byPosition)
{
    ContextMenu* menu = m_page->contextMenuController()->contextMenu();
    ASSERT(menu);

    ContextMenuItem* item = byPosition ? menu->itemAtIndex((unsigned)wParam) : menu->itemWithAction((ContextMenuAction)wParam);
    if (!item)
        return;
    m_page->contextMenuController()->contextMenuItemSelected(item);
}
#endif

bool WebView::handleMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG globalClickCount;
    static IntPoint globalPrevPoint;
    static MouseButton globalPrevButton;
#if !OS(WINCE)
    static LONG globalPrevMouseDownTime;
#endif

    if (message == WM_CANCELMODE) {
        m_page->mainFrame()->eventHandler()->lostMouseCapture();
        return true;
    }

    // Drop mouse move events if either:
    // 1) A gesture (native scrolling) is in progress
    // 2) A gesture could begin but has not yet and the time
    //    since the gesture started is under HOLD_THRESHOLD.
    if (message == WM_MOUSEMOVE && (m_gestureInProgress ||
        (m_gestureTargetNode && m_gestureStartTime && WTF::monotonicallyIncreasingTime() - m_gestureStartTime < HOLD_THRESHOLD / 1000.0)))
        return false;

    // Create our event.
    // On WM_MOUSELEAVE we need to create a mouseout event, so we force the position
    // of the event to be at (MINSHORT, MINSHORT).
#if OS(WINCE)
    LPARAM position = lParam;
#else
    LPARAM position = (message == WM_MOUSELEAVE) ? ((MINSHORT << 16) | MINSHORT) : lParam;
#endif
    PlatformMouseEvent mouseEvent(m_viewWindow, message, wParam, position, m_mouseActivated);

#if !OS(WINCE)
    setMouseActivated(false);
#endif

    bool insideThreshold = abs(globalPrevPoint.x() - mouseEvent.position().x()) < ::GetSystemMetrics(SM_CXDOUBLECLK) &&
                           abs(globalPrevPoint.y() - mouseEvent.position().y()) < ::GetSystemMetrics(SM_CYDOUBLECLK);
#if !OS(WINCE)
    // GetMessageTime is not implemented on WINCE.  If double click is needed for CE, this
    // will need to be implemented differently.
    LONG messageTime = ::GetMessageTime();
#endif

    bool handled = false;

    if (message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_RBUTTONDOWN) {
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, "WebView: Mouse Down");
        // FIXME: I'm not sure if this is the "right" way to do this
        // but without this call, we never become focused since we don't allow
        // the default handling of mouse events.
        SetFocus(m_viewWindow);

        // Always start capturing events when the mouse goes down in our HWND.
        ::SetCapture(m_viewWindow);

#if !OS(WINCE)
        if (((messageTime - globalPrevMouseDownTime) < (LONG)::GetDoubleClickTime()) && 
            insideThreshold &&
            mouseEvent.button() == globalPrevButton)
            globalClickCount++;
        else
#endif
            // Reset the click count.
            globalClickCount = 1;
#if !OS(WINCE)
        globalPrevMouseDownTime = messageTime;
#endif
        globalPrevButton = mouseEvent.button();
        globalPrevPoint = mouseEvent.position();
        
        mouseEvent.setClickCount(globalClickCount);
        handled = m_page->mainFrame()->eventHandler()->handleMousePressEvent(mouseEvent);
    } else if (message == WM_LBUTTONDBLCLK || message == WM_MBUTTONDBLCLK || message == WM_RBUTTONDBLCLK) {
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, "WebView: Double Click");
        globalClickCount++;
        mouseEvent.setClickCount(globalClickCount);
        handled = m_page->mainFrame()->eventHandler()->handleMousePressEvent(mouseEvent);
    } else if (message == WM_LBUTTONUP || message == WM_MBUTTONUP || message == WM_RBUTTONUP) {
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, "WebView: Mouse Up");
        // Record the global position and the button of the up.
        globalPrevButton = mouseEvent.button();
        globalPrevPoint = mouseEvent.position();
        mouseEvent.setClickCount(globalClickCount);
        m_page->mainFrame()->eventHandler()->handleMouseReleaseEvent(mouseEvent);
        ::ReleaseCapture();
#if !OS(WINCE)
    } else if (message == WM_MOUSELEAVE && m_mouseOutTracker) {
        // Once WM_MOUSELEAVE is fired windows clears this tracker
        // so there is no need to disable it ourselves.
        m_mouseOutTracker.clear();
        m_page->mainFrame()->eventHandler()->mouseMoved(mouseEvent);
        handled = true;
#endif
    } else if (message == WM_MOUSEMOVE) {
        WebKitMouseMoveProcessingMode mode = m_mouseProcessMode;

        if (mode == WebKitMouseMove_SkipWhenPaintPending) {
            BOOL webViewInvalidated = GetUpdateRect(m_viewWindow, NULL, FALSE);
            if (!webViewInvalidated && !m_paintPending)
                mode = WebKitMouseMove_Normal;
        }

        if (mode == WebKitMouseMove_Normal) {
            char buff[64];
            _snprintf(buff, 64, "WebView: Mouse Move (%d, %d)", mouseEvent.position().x(), mouseEvent.position().y());
            PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
            if (!insideThreshold)
                globalClickCount = 0;
            mouseEvent.setClickCount(globalClickCount);
            handled = m_page->mainFrame()->eventHandler()->mouseMoved(mouseEvent);
#if !OS(WINCE)
            if (!m_mouseOutTracker) {
                m_mouseOutTracker = adoptPtr(new TRACKMOUSEEVENT);
                m_mouseOutTracker->cbSize = sizeof(TRACKMOUSEEVENT);
                m_mouseOutTracker->dwFlags = TME_LEAVE;
                m_mouseOutTracker->hwndTrack = m_viewWindow;
                ::TrackMouseEvent(m_mouseOutTracker.get());
            }
#endif
        }
        else {//if (mode == WebKitMouseMove_SkipWhenPaintPending) {
            PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, "WebView: Skip WM_MOUSEMOVE");
            handled = true;
        }
    }
    PERFORMANCE_END(WTF::PerformanceTrace::InputEvent);
    return handled;
}

static WebCore::Node* enclosingLayerNode(WebCore::RenderLayer* layer)
{
    for (RenderObject* r = layer->renderer(); r; r = r->parent()) {
        if (WebCore::Node* e = r->node())
            return e;
    }
    return 0;
}

static ScrollableArea* findFirstScrollableArea(PassRefPtr<WebCore::Node> node, WebCore::Node ** scrollableElement = NULL)
{
    // We could scroll the parent frame.
    // We could take direction into account - if we are the bottom of a layer,
    // bubble up to its parent.
    if (!node)
        return NULL;

    RenderLayer * layer = node->renderer()->enclosingLayer();
    while (layer) {
        RenderBox* box = layer->renderBox();
        if (!box)
            return NULL;
        if (box->canBeScrolledAndHasScrollableArea()) {
            // If this layer is a FrameView, return as a FrameView.
            // For whateve reason the FrameView is a 'scrollable' scroll area
            // but the RenderLayer is not.
            RenderObject * renderer = layer->renderer();
            if (scrollableElement)
                *scrollableElement = enclosingLayerNode(layer);
            if (renderer->isRenderView()) {
                FrameView* view = renderer->view()->frameView();
                if (view) {
                    bool canBeScrolled = false;
                    if (view->canHaveScrollbars()) {
                        IntSize visibleSize = view->visibleContentRect().size();
                        IntSize contentsSize = view->contentsSize();
                        canBeScrolled = contentsSize.height() > visibleSize.height() || contentsSize.width() > visibleSize.width();
                    }
                    return canBeScrolled ? view : NULL;
                }
            }
            else if (layer->horizontalScrollbar() || layer->verticalScrollbar()) {
                return layer;
            }
        }
        layer = layer->parent();
    }
    return NULL;
}

RefPtr<WebCore::Node> WebView::gestureInfoAtPoint(POINTS beginPoint, bool & hitScrollbar, bool & canBeScrolled)
{
    hitScrollbar = false;
    canBeScrolled = false;

    RefPtr<WebCore::Node> node;
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return node;

    ScrollView* view = coreFrame->view();
    if (!view)
        return node;

    POINT gestureBeginPoint = {beginPoint.x, beginPoint.y};
    HitTestRequest request(HitTestRequest::ReadOnly);
    for (Frame* childFrame = m_page->mainFrame(); childFrame; childFrame = EventHandler::subframeForTargetNode(node.get())) {
        FrameView* frameView = childFrame->view();
        if (!frameView)
            break;
        RenderView* renderView = childFrame->document()->renderView();
        if (!renderView)
            break;
        RenderLayer* layer = renderView->layer();
        if (!layer)
            break;

        HitTestResult result(frameView->screenToContents(gestureBeginPoint));
        layer->hitTest(request, result);
        node = result.innerNode();

        if (!hitScrollbar)
            hitScrollbar = result.scrollbar();
    }

    if (!hitScrollbar) {
        // The hit testing above won't detect if we've hit the main frame's vertical scrollbar. Check that manually now.
        RECT webViewRect;
        GetWindowRect(m_viewWindow, &webViewRect);
        hitScrollbar = view->verticalScrollbar() && (gestureBeginPoint.x > (webViewRect.right - view->verticalScrollbar()->theme()->scrollbarThickness()));  
    }

    if (node) {
        canBeScrolled = findFirstScrollableArea(node) != NULL;
    }

    return node;
}

#if USE(XP_GESTURE_MODEL)
bool WebView::gestureNotify(WPARAM wParam, LPARAM lParam)
{
    GESTURENOTIFYSTRUCT* gn = reinterpret_cast<GESTURENOTIFYSTRUCT*>(lParam);
    PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, "WebView: GestureNotify");

    // If we don't have this function, we shouldn't be receiving this message
    ASSERT(SetGestureConfigPtr());

    bool hitScrollbar = false;
    bool canBeScrolled = false;
    m_gestureTargetNode = gestureInfoAtPoint(gn->ptsLocation, hitScrollbar, canBeScrolled);

    // We always allow two-fingered panning with inertia and a gutter (which limits movement to one
    // direction in most cases).
    DWORD dwPanWant = GC_PAN | GC_PAN_WITH_INERTIA | GC_PAN_WITH_GUTTER;
    // We never allow single-fingered horizontal panning. That gesture is reserved for creating text
    // selections. This matches IE.
    DWORD dwPanBlock = GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;

    if (hitScrollbar || !canBeScrolled) {
        // The part of the page under the gesture can't be scrolled, or the gesture is on a scrollbar.
        // Disallow single-fingered vertical panning in this case, too, so we'll fall back to the default
        // behavior (which allows the scrollbar thumb to be dragged, text selections to be made, etc.).
        dwPanBlock |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
        m_gestureTargetNode = 0;
    } else {
        // The part of the page the gesture is under can be scrolled, and we're not under a scrollbar.
        // Allow single-fingered vertical panning in this case, so the user will be able to pan the page
        // with one or two fingers.
        dwPanWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
    }

    GESTURECONFIG gc = { GID_PAN, dwPanWant, dwPanBlock };
    BOOL result = SetGestureConfigPtr()(m_viewWindow, 0, 1, &gc, sizeof(GESTURECONFIG));
    PERFORMANCE_END(WTF::PerformanceTrace::InputEvent);
    return result;
}
#endif

void WebView::cancelGestureAnimation()
{
    m_gestureAnimation.clear();
    m_gestureStartTime = 0;
    m_gestureTargetNode = 0;
}

void WebView::animationTimerFired(Timer<WebView>*)
{
    const double kFrameRate = 60;
    const double kMinimumTimerInterval = .001;
    const double kMaximumTimerInterval = 1.0 / kFrameRate;

    double currentTime = WTF::monotonicallyIncreasingTime();
    double deltaToNextFrame = ceil((currentTime - m_gestureStartTime) * kFrameRate) / kFrameRate - (currentTime - m_gestureStartTime);
    currentTime += deltaToNextFrame;

    bool continueTimer = false;
    if (m_gestureAnimation) {
        if (m_gestureAnimation->animate(currentTime))
            continueTimer = true;
        else
            cancelGestureAnimation();
    }

    if (m_overpanElement) {
        continueTimer |= updateOverpanFeedback(currentTime);
    }

    if (m_fadingScrollbar) {
        continueTimer |= updateFadingScrollbar(currentTime);
    }

    if (continueTimer)
        m_animationTimer.startOneShot(snapToRange(deltaToNextFrame, kMinimumTimerInterval, kMaximumTimerInterval));
}

WebCore::IntRect WebView::determineOverpanRect(PassRefPtr<WebCore::Node> node, double percentComplete)
{
    FrameView* frameView = 0;
    if (Frame* coreFrame = core(m_mainFrame))
        frameView = coreFrame->view();

    if (!frameView)
        return IntRect();

    RenderObject * renderObject = node->renderer();
    if (!renderObject || !renderObject->isBox())
        return IntRect();

    RenderBox * boxObject = toRenderBox(renderObject);
    WebCore::LayoutRect r = boxObject->visualOverflowRect();
    bool fixed = boxObject->isRenderView();
    boxObject->computeAbsoluteRepaintRect(r, fixed);
    IntRect rect = IntRect(r);

    // If node is part of an iframe, adjust now for the parent
    // frame(s).
    WebCore::HTMLFrameOwnerElement * elt = boxObject->document()->ownerElement();
    if (elt) {
        boxObject = elt->renderBox();
        // adjust for scroll position
        rect = elt->contentFrame()->view()->contentsToRootView(rect);
        r = LayoutRect(rect);

        // clip to the iframe
        IntRect clipRect = elt->contentFrame()->view()->windowClipRect();
        LayoutRect c = LayoutRect(clipRect);
        r = intersection(r, c);
        rect = IntRect(r);
    }
    else {
        rect = frameView->contentsToWindow(rect);
    }

    if (m_yOverpan) {
        int overpan = m_yOverpan * (1.0 - percentComplete * OVERPAN_REDUCTION);
        if (OVERPAN_MAX_SIZE && abs(overpan) > OVERPAN_MAX_SIZE) {
            overpan = overpan > 0 ? OVERPAN_MAX_SIZE : -OVERPAN_MAX_SIZE;
        }
        if (overpan > 0) {
            rect.setY( rect.y() + rect.height() - overpan );
        }
        rect.setHeight(abs(overpan));
    }
    else if (m_xOverpan) {
        int overpan = m_xOverpan * (1.0 - percentComplete * OVERPAN_REDUCTION);
        if (OVERPAN_MAX_SIZE && abs(overpan) > OVERPAN_MAX_SIZE) {
            overpan = overpan > 0 ? OVERPAN_MAX_SIZE : -OVERPAN_MAX_SIZE;
        }
        if (overpan > 0) {
            rect.setX( rect.x() + rect.width() - overpan );
        }
        rect.setWidth(abs(overpan));
    }
    return rect;
}

bool WebView::updateOverpanFeedback(double ctime)
{
    if (!m_overpanElement)
        return false;

    IntRect dirtyRect = m_overpanRect;

    if (m_overpanStartTime == 0) {
        m_overpanStartTime = ctime;
        m_overpanPercentComplete = 0;
        m_overpanRect = determineOverpanRect(m_overpanElement, m_overpanPercentComplete);
    }
    else if (ctime - m_overpanStartTime > OVERPAN_START) {
        m_overpanPercentComplete = (ctime - (m_overpanStartTime + OVERPAN_START)) / OVERPAN_DURATION;
        m_overpanRect = determineOverpanRect(m_overpanElement, m_overpanPercentComplete);
    }

    dirtyRect.unite(m_overpanRect);
    repaint(dirtyRect, true /*contentChanged*/, false /*immediate*/, false /*repaintContentOnly*/);

    // another option: tell the renderer to repaint a subrect (in coordinates relative to the renderer).
    //m_overpanElement->renderer()->repaintRectangle();

    if (m_overpanPercentComplete >= 1.0 || m_overpanRect.isEmpty()) {
        m_overpanElement = 0;
        m_overpanRect = IntRect();
        return false;
    }
    return true;
}

void WebView::paintOverpanFeedback(WebCore::GraphicsContext* context, const WebCore::IntRect& dirtyRect)
{
    if (!m_overpanElement)
        return;

    IntRect rect = dirtyRect;
    rect.intersect(m_overpanRect);
    if (rect.isEmpty() || (m_yOverpan == 0 && m_xOverpan == 0))
        return;

    FloatPoint p1, p2;
    if (abs(m_yOverpan) > abs(m_xOverpan)) {
        p1.setX(m_overpanRect.center().x());
        p2.setX(p1.x());
        if (m_yOverpan > 0) {
            p1.setY(m_overpanRect.y());
            p2.setY(m_overpanRect.maxY());
        }
        else {
            p1.setY(m_overpanRect.maxY());
            p2.setY(m_overpanRect.y());
        }
    }
    else {
        p1.setY(m_overpanRect.center().y());
        p2.setY(p1.y());
        if (m_xOverpan > 0) {
            p1.setX(m_overpanRect.x());
            p2.setX(m_overpanRect.maxX());
        }
        else {
            p1.setX(m_overpanRect.maxX());
            p2.setX(m_overpanRect.x());
        }
    }
    RefPtr<Gradient> gradient = Gradient::create(p1, p2);
    Color color = OVERPAN_COLOR;
    Color startColor(color.red(), color.green(), color.blue(), 0);
    Color endColor(color.red(), color.green(), color.blue(), (int)(255 - m_overpanPercentComplete * 255 * OVERPAN_OPACITY_REDUCTION));
    gradient->addColorStop(0.0f, startColor);
    gradient->addColorStop(1.0f, endColor);
    context->setFillGradient(gradient);
    context->fillRect(rect);
}

void WebView::showOverpanFeedback(int overpanX, int overpanY, PassRefPtr<WebCore::Node> layer, bool autoScroll)
{
    if (m_overpanElement != layer || overpanX * m_xOverpanLast < 0 || overpanY * m_yOverpanLast < 0) {
        m_xOverpan = 0;
        m_yOverpan = 0;
    }

    m_xOverpanLast = overpanX;
    m_yOverpanLast = overpanY;

    if (autoScroll) {
        m_xOverpan += overpanX;
        m_yOverpan += overpanY;
    }
    else {
        m_xOverpan += (int)(overpanX > 0 ? ceil((double)overpanX * OVERPAN_DRAG) : floor((double)overpanX * OVERPAN_DRAG));
        m_yOverpan += (int)(overpanY > 0 ? ceil((double)overpanY * OVERPAN_DRAG) : floor((double)overpanY * OVERPAN_DRAG));
    }

    m_overpanElement = layer;
    m_overpanStartTime = 0;
    if (!m_animationTimer.isActive())
        m_animationTimer.startOneShot(0);
}

void WebView::scrollbarWillUpdate(Scrollbar* scrollbar)
{
    if (!scrollbar)
        return;

    WebCore::ScrollbarTheme * theme = scrollbar->theme();
    if (!theme->usesOverlayScrollbars())
        return;

    // Hide any prior visible scrollbar:
    if (m_fadingScrollbar && m_fadingScrollbar != scrollbar)
        ScrollbarThemeOpus::setThumbOpacity(m_fadingScrollbar, 0);

    // Make the scrollbar visible:
    ScrollbarThemeOpus::setThumbOpacity(scrollbar, 255);
    m_fadingScrollbar = scrollbar;
}

bool WebView::updateFadingScrollbar(double ctime)
{
    // Don't start fading the scrollbar until the gesture has
    // ended and the scroll has stopped animating.
    if (m_gestureInProgress || m_gestureAnimation)
        return true;

    if (!m_scrollbarFadeStartTime) {
        m_scrollbarFadeStartTime = ctime;
    }

    double progress = (ctime - m_scrollbarFadeStartTime) / 0.35;
    int opacity = 255 * progress;
    if (opacity > 255)
        opacity = 255;
    opacity = 255 - opacity;
    bool continueAnimation = ScrollbarThemeOpus::setThumbOpacity(m_fadingScrollbar, opacity);

    if (continueAnimation) {
        m_fadingScrollbar->invalidate();
        continueAnimation = (opacity > 0);
    }

    if (!continueAnimation) {
        m_fadingScrollbar = 0;
    }

    return continueAnimation;
}

bool WebView::scrollNode(PassRefPtr<WebCore::Node> node, const WebCore::IntPoint & distance, bool autoScroll)
{
    WebCore::Node * layer = NULL;
    ScrollableArea * scroller = findFirstScrollableArea(node, &layer);
    if (!scroller)
        return false;

    IntPoint scrollOffset = scroller->scrollOrigin() + scroller->scrollPosition();
    IntPoint newScrollOffset = scrollOffset;

    int overpanX = 0;
    int overpanY = 0;

    // We negate here (scrollOffset.x() - distance.x()) since panning up moves the content up,
    // but moves the scrollbar down.
    if (distance.x()) {
        int maxX = scroller->scrollSize(HorizontalScrollbar);
        if (maxX > 0) {
            int newX = scrollOffset.x() - distance.x();
            if (newX < 0) {
                overpanX = newX;
                newX = 0;
            }
            else if (newX > maxX) {
                overpanX = newX - maxX;
                newX = maxX;
            }
            newScrollOffset.setX(newX);
        }
    }
    if (distance.y()) {
        int maxY = scroller->scrollSize(VerticalScrollbar);
        if (maxY > 0) {
            int newY = scrollOffset.y() - distance.y();
            if (newY < 0) {
                overpanY = newY;
                newY = 0;
            }
            else if (newY > maxY) {
                overpanY = newY - maxY;
                newY = maxY;
            }
            newScrollOffset.setY(newY);
        }
    }

    if (overpanX || overpanY) {
        // only show overpan for overlay scrollbars
        if (overpanX && (!scroller->horizontalScrollbar() || !scroller->horizontalScrollbar()->isOverlayScrollbar()))
            overpanX = 0;
        if (overpanY && (!scroller->verticalScrollbar() || !scroller->verticalScrollbar()->isOverlayScrollbar()))
            overpanY = 0;
        if (overpanX || overpanY) {
            RefPtr<WebCore::Node> layerNode(layer);
            showOverpanFeedback(overpanX, overpanY, layerNode, autoScroll);
        }
    }

    if (scrollOffset != newScrollOffset) {
        if (scrollOffset.y() != newScrollOffset.y())
            scrollbarWillUpdate(scroller->verticalScrollbar());
        else if (scrollOffset.x() != newScrollOffset.x())
            scrollbarWillUpdate(scroller->horizontalScrollbar());
        scroller->scrollToOffsetWithoutAnimation(newScrollOffset);
        return true;
    }
    return false;
}

void WebView::scrollBy(const WebCore::IntPoint & distance)
{
    if (!scrollNode(m_gestureTargetNode, distance, true))
        cancelGestureAnimation();
}

bool WebView::gesture(WPARAM wParam, LPARAM lParam) 
{
    // We want to bail out if we don't have either of these functions
    if (!GetGestureInfoPtr() || !CloseGestureInfoHandlePtr())
        return false;

    HGESTUREINFO gestureHandle = reinterpret_cast<HGESTUREINFO>(lParam);
    
    GESTUREINFO gi = {0};
    gi.cbSize = sizeof(GESTUREINFO);

    if (!GetGestureInfoPtr()(gestureHandle, reinterpret_cast<PGESTUREINFO>(&gi)))
        return false;

    bool result = false;
    char buff[64];
    switch (gi.dwID) {
    case GID_BEGIN:
        _snprintf(buff, 64, "WebView: Gesture GID_BEGIN (%d, %d)", gi.ptsLocation.x, gi.ptsLocation.y);
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
        cancelGestureAnimation();
#if USE(CE_GESTURE_MODEL)
        bool gestureHitScrollbar, gestureCanScroll;
        m_gestureTargetNode = gestureInfoAtPoint(gi.ptsLocation, gestureHitScrollbar, gestureCanScroll);
        if (gestureHitScrollbar || !gestureCanScroll)
            m_gestureTargetNode = 0;
#endif
        m_gestureStartTime = WTF::monotonicallyIncreasingTime();
        m_lastPanX = gi.ptsLocation.x;
        m_lastPanY = gi.ptsLocation.y;
        break;
    case GID_END:
        _snprintf(buff, 64, "WebView: Gesture GID_END (%d, %d)", gi.ptsLocation.x, gi.ptsLocation.y);
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
        m_gestureInProgress = false;
        m_mouseEventHandled = false;
        // Now that the gesture has ended the
        // fading scrollbar can start fading.
        if (m_fadingScrollbar) {
            m_scrollbarFadeStartTime = 0;
            if (!m_animationTimer.isActive())
                m_animationTimer.startOneShot(0);
        }
        break;
    case GID_PAN: {
        _snprintf(buff, 64, "WebView: Gesture GID_PAN (%d, %d)", gi.ptsLocation.x, gi.ptsLocation.y);
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
        if ((m_mouseEventHandled && !m_gestureInProgress) ||
            (!m_gestureTargetNode || !m_gestureTargetNode->renderer())) {
            break;
        }
        if (!m_gestureInProgress) {
            m_gestureInProgress = true;
            Frame* coreFrame = core(m_mainFrame);
            if (coreFrame && m_gestureInProgress) {
                Frame * frameToInvalidate = coreFrame;
                Document * doc = m_gestureTargetNode.get()->ownerDocument();
                if (doc && doc->frame())
                    frameToInvalidate = doc->frame();
                // Prevent a click from firing.  Calling invalidateClick()
                // would be lighter weight, but that method is private.
                frameToInvalidate->eventHandler()->clear();
            }
        }
        // Where are the fingers currently?
        long currentX = gi.ptsLocation.x;
        long currentY = gi.ptsLocation.y;
        // How far did we pan in each direction?
        long deltaX = currentX - m_lastPanX;
        long deltaY = currentY - m_lastPanY;

        // Update our class variables with updated values
        m_lastPanX = currentX;
        m_lastPanY = currentY;

        scrollNode(m_gestureTargetNode, IntPoint(deltaX, deltaY), false);
        result = true;
        break;
    }
#if USE(CE_GESTURE_MODEL) || (USE(XP_GESTURE_MODEL) && USE(SIMULATED_GESTURES))
    case GID_SCROLL: {
        _snprintf(buff, 64, "WebView: Gesture GID_SCROLL v=%d (%d, %d)", GID_SCROLL_VELOCITY(gi.ullArguments), gi.ptsLocation.x, gi.ptsLocation.y);
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
        FloatPoint fp;
        switch (GID_SCROLL_DIRECTION(gi.ullArguments)) {
            case ARG_SCROLL_DOWN:
                fp.setY(-GID_SCROLL_VELOCITY(gi.ullArguments));
                break;
            case ARG_SCROLL_UP:
                fp.setY(GID_SCROLL_VELOCITY(gi.ullArguments));
                break;
            case ARG_SCROLL_RIGHT:
                fp.setX(-GID_SCROLL_VELOCITY(gi.ullArguments));
                break;
            case ARG_SCROLL_LEFT:
                fp.setX(GID_SCROLL_VELOCITY(gi.ullArguments));
                break;
        }
        if (m_gestureAnimation)
            m_gestureAnimation.clear();
        if (fp.x() != 0 || fp.y() != 0) {
            m_gestureStartTime = WTF::monotonicallyIncreasingTime();
            m_gestureAnimation = ActivePlatformGestureAnimation::create(TouchpadFlingPlatformGestureCurve::create(fp), this);
            if (!m_animationTimer.isActive())
                m_animationTimer.startOneShot(0);
        }
        result = true;
        break;
    }
#endif
    default:
        _snprintf(buff, 64, "WebView: Gesture gi.dwID = %d", gi.dwID);
        PERFORMANCE_START(WTF::PerformanceTrace::InputEvent, buff);
        break;
    }
    PERFORMANCE_END(WTF::PerformanceTrace::InputEvent);
    CloseGestureInfoHandlePtr()(gestureHandle);
    return result;
}

bool WebView::mouseWheel(WPARAM wParam, LPARAM lParam, bool isMouseHWheel)
{
    // Ctrl+Mouse wheel doesn't ever go into WebCore.  It is used to
    // zoom instead (Mac zooms the whole Desktop, but Windows browsers trigger their
    // own local zoom modes for Ctrl+wheel).
    if (wParam & MK_CONTROL) {
        short delta = short(HIWORD(wParam));
        if (delta < 0)
            makeTextSmaller(0);
        else
            makeTextLarger(0);
        return true;
    }
    
    // FIXME: This doesn't fix https://bugs.webkit.org/show_bug.cgi?id=28217. This only fixes https://bugs.webkit.org/show_bug.cgi?id=28203.
    HWND focusedWindow = GetFocus();
    if (focusedWindow && focusedWindow != m_viewWindow) {
        // Our focus is on a different hwnd, see if it's a PopupMenu and if so, set the focus back on us (which will hide the popup).
        WCHAR className[256];

        // Make sure truncation won't affect the comparison.
        ASSERT(WTF_ARRAY_LENGTH(className) > wcslen(PopupMenuWin::popupClassName()));

        if (GetClassNameW(focusedWindow, className, WTF_ARRAY_LENGTH(className)) && !wcscmp(className, PopupMenuWin::popupClassName())) {
            // We don't let the WebView scroll here for two reasons - 1) To match Firefox behavior, 2) If we do scroll, we lose the
            // focus ring around the select menu.
            SetFocus(m_viewWindow);
            return true;
        }
    }

    PlatformWheelEvent wheelEvent(m_viewWindow, wParam, lParam, isMouseHWheel);
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return false;

    return coreFrame->eventHandler()->handleWheelEvent(wheelEvent);
}

bool WebView::verticalScroll(WPARAM wParam, LPARAM /*lParam*/)
{
    ScrollDirection direction;
    ScrollGranularity granularity;
    switch (LOWORD(wParam)) {
    case SB_LINEDOWN:
        granularity = ScrollByLine;
        direction = ScrollDown;
        break;
    case SB_LINEUP:
        granularity = ScrollByLine;
        direction = ScrollUp;
        break;
    case SB_PAGEDOWN:
        granularity = ScrollByDocument;
        direction = ScrollDown;
        break;
    case SB_PAGEUP:
        granularity = ScrollByDocument;
        direction = ScrollUp;
        break;
    default:
        return false;
        break;
    }
    
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    return frame->eventHandler()->scrollRecursively(direction, granularity);
}

bool WebView::horizontalScroll(WPARAM wParam, LPARAM /*lParam*/)
{
    ScrollDirection direction;
    ScrollGranularity granularity;
    switch (LOWORD(wParam)) {
    case SB_LINELEFT:
        granularity = ScrollByLine;
        direction = ScrollLeft;
        break;
    case SB_LINERIGHT:
        granularity = ScrollByLine;
        direction = ScrollRight;
        break;
    case SB_PAGELEFT:
        granularity = ScrollByDocument;
        direction = ScrollLeft;
        break;
    case SB_PAGERIGHT:
        granularity = ScrollByDocument;
        direction = ScrollRight;
        break;
    default:
        return false;
    }

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    return frame->eventHandler()->scrollRecursively(direction, granularity);
}


bool WebView::execCommand(WPARAM wParam, LPARAM /*lParam*/)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    switch (LOWORD(wParam)) {
        case SelectAll:
            return frame->editor().command("SelectAll").execute();
        case Undo:
            return frame->editor().command("Undo").execute();
        case Redo:
            return frame->editor().command("Redo").execute();
    }
    return false;
}

bool WebView::keyUp(WPARAM virtualKeyCode, LPARAM keyData, bool systemKeyDown)
{
    PlatformKeyboardEvent keyEvent(m_viewWindow, virtualKeyCode, keyData, PlatformEvent::KeyUp, systemKeyDown);

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    m_currentCharacterCode = 0;

    return frame->eventHandler()->keyEvent(keyEvent);
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;


struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   CtrlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   CtrlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  CtrlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  CtrlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },

    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },
    
    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },

    { VK_ESCAPE, 0,                  "Cancel"                                      },
#if !OS(WINCE)
    { VK_OEM_PERIOD, CtrlKey,        "Cancel"                                      },
#endif
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, ShiftKey,           "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },

    // It's not quite clear whether clipboard shortcuts and Undo/Redo should be handled
    // in the application or in WebKit. We chose WebKit.
    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'X',       CtrlKey,            "Cut"                                         },
    { 'A',       CtrlKey,            "SelectAll"                                   },
    { VK_INSERT, CtrlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },
    { 'Z',       CtrlKey,            "Undo"                                        },
    { 'Z',       CtrlKey | ShiftKey, "Redo"                                        },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   ShiftKey,           "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};

const char* WebView::interpretKeyEvent(const KeyboardEvent* evt)
{
    ASSERT(evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);

    static HashMap<int, const char*>* keyDownCommandsMap = 0;
    static HashMap<int, const char*>* keyPressCommandsMap = 0;

    if (!keyDownCommandsMap) {
        keyDownCommandsMap = new HashMap<int, const char*>;
        keyPressCommandsMap = new HashMap<int, const char*>;

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyDownEntries); ++i)
            keyDownCommandsMap->set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyPressEntries); ++i)
            keyPressCommandsMap->set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
    }

    unsigned modifiers = 0;
    if (evt->shiftKey())
        modifiers |= ShiftKey;
    if (evt->altKey())
        modifiers |= AltKey;
    if (evt->ctrlKey())
        modifiers |= CtrlKey;

    if (evt->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | evt->keyCode();
        return mapKey ? keyDownCommandsMap->get(mapKey) : 0;
    }

    int mapKey = modifiers << 16 | evt->charCode();
    return mapKey ? keyPressCommandsMap->get(mapKey) : 0;
}

bool WebView::handleEditingKeyboardEvent(KeyboardEvent* evt)
{
    Node* node = evt->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = evt->keyEvent();
    if (!keyEvent || keyEvent->isSystemKey())  // do not treat this as text input if it's a system key event
        return false;

    Editor::Command command = frame->editor().command(interpretKeyEvent(evt));

    if (keyEvent->type() == PlatformEvent::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(evt);
    }

     if (command.execute(evt))
        return true;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (evt->charCode() < ' ')
        return false;

    return frame->editor().insertText(evt->keyEvent()->text(), evt);
}

bool WebView::keyDown(WPARAM virtualKeyCode, LPARAM keyData, bool systemKeyDown)
{
#if ENABLE(FULLSCREEN_API)
    // Trap the ESC key when in full screen mode.
    if (virtualKeyCode == VK_ESCAPE && isFullScreen()) {
        m_fullscreenController->exitFullScreen();
        return false;
    }
#endif
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    PlatformKeyboardEvent keyEvent(m_viewWindow, virtualKeyCode, keyData, PlatformEvent::RawKeyDown, systemKeyDown);
    bool handled = frame->eventHandler()->keyEvent(keyEvent);

    // These events cannot be canceled, and we have no default handling for them.
    // FIXME: match IE list more closely, see <http://msdn2.microsoft.com/en-us/library/ms536938.aspx>.
    if (systemKeyDown && virtualKeyCode != VK_RETURN)
        return false;

    if (handled) {
        // FIXME: remove WM_UNICHAR, too
        MSG msg;
        // WM_SYSCHAR events should not be removed, because access keys are implemented in WebCore in WM_SYSCHAR handler.
        if (!systemKeyDown)
            ::PeekMessage(&msg, m_viewWindow, WM_CHAR, WM_CHAR, PM_REMOVE);
        return true;
    }

    // We need to handle back/forward using either Ctrl+Left/Right Arrow keys.
    // FIXME: This logic should probably be in EventHandler::defaultArrowEventHandler().
    // FIXME: Should check that other modifiers aren't pressed.
    if (virtualKeyCode == VK_RIGHT && keyEvent.ctrlKey())
        return m_page->goForward();
    if (virtualKeyCode == VK_LEFT && keyEvent.ctrlKey())
        return m_page->goBack();

    // Need to scroll the page if the arrow keys, pgup/dn, or home/end are hit.
    ScrollDirection direction;
    ScrollGranularity granularity;
    switch (virtualKeyCode) {
        case VK_LEFT:
            granularity = ScrollByLine;
            direction = ScrollLeft;
            break;
        case VK_RIGHT:
            granularity = ScrollByLine;
            direction = ScrollRight;
            break;
        case VK_UP:
            granularity = ScrollByLine;
            direction = ScrollUp;
            break;
        case VK_DOWN:
            granularity = ScrollByLine;
            direction = ScrollDown;
            break;
        case VK_HOME:
            granularity = ScrollByDocument;
            direction = ScrollUp;
            break;
        case VK_END:
            granularity = ScrollByDocument;
            direction = ScrollDown;
            break;
        case VK_PRIOR:
            granularity = ScrollByPage;
            direction = ScrollUp;
            break;
        case VK_NEXT:
            granularity = ScrollByPage;
            direction = ScrollDown;
            break;
        default:
            return false;
    }

    return frame->eventHandler()->scrollRecursively(direction, granularity);
}

bool WebView::keyPress(WPARAM charCode, LPARAM keyData, bool systemKeyDown)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();

    PlatformKeyboardEvent keyEvent(m_viewWindow, charCode, keyData, PlatformEvent::Char, systemKeyDown);
    // IE does not dispatch keypress event for WM_SYSCHAR.
    if (systemKeyDown)
        return frame->eventHandler()->handleAccessKey(keyEvent);
    return frame->eventHandler()->keyEvent(keyEvent);
}

void WebView::setIsBeingDestroyed()
{
    m_isBeingDestroyed = true;

    // Remove our this pointer from the window so we won't try to handle any more window messages.
    // See <http://webkit.org/b/55054>.
    ::SetWindowLongPtrW(m_viewWindow, 0, 0);
}

void WebView::setShouldInvertColors(bool shouldInvertColors)
{
    if (m_shouldInvertColors == shouldInvertColors)
        return;

    m_shouldInvertColors = shouldInvertColors;

#if USE(ACCELERATED_COMPOSITING)
    if (m_layerTreeHost)
        m_layerTreeHost->setShouldInvertColors(shouldInvertColors);
#endif

    RECT windowRect = {0};
    frameRect(&windowRect);
    repaint(windowRect, true, true);
}

bool WebView::registerWebViewWindowClass()
{
    static bool haveRegisteredWindowClass = false;
    if (haveRegisteredWindowClass)
        return true;

    haveRegisteredWindowClass = true;

#if OS(WINCE)
    WNDCLASS wcex;
    wcex.style = 0;
    wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
#else
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_DBLCLKS;
    wcex.hbrBackground  = 0;
    wcex.hIconSm        = 0;
#endif

    wcex.lpfnWndProc    = WebViewWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(WebView*);
    wcex.hInstance      = gInstance;
    wcex.hIcon          = 0;
    wcex.hCursor        = ::LoadCursor(0, IDC_ARROW);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kWebViewWindowClassName;

#if OS(WINCE)
    return !!RegisterClass(&wcex);
#else
    return !!RegisterClassEx(&wcex);
#endif
}

static HWND findTopLevelParent(HWND window)
{
    if (!window)
        return 0;

    HWND current = window;
    for (HWND parent = GetParent(current); current; current = parent, parent = GetParent(parent))
        if (!parent || !(GetWindowLongPtr(current, GWL_STYLE) & (WS_POPUP | WS_CHILD)))
            return current;
    ASSERT_NOT_REACHED();
    return 0;
}

LRESULT CALLBACK WebView::WebViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    LONG_PTR longPtr = GetWindowLongPtr(hWnd, 0);
    WebView* webView = reinterpret_cast<WebView*>(longPtr);
    WebFrame* mainFrameImpl = webView ? webView->topLevelFrame() : 0;
    if (!mainFrameImpl)
        return DefWindowProcW(hWnd, message, wParam, lParam);

    // We shouldn't be trying to handle any window messages after WM_DESTROY.
    // See <http://webkit.org/b/55054>.
    ASSERT(!webView->isBeingDestroyed());

    // hold a ref, since the WebView could go away in an event handler.
    COMPtr<WebView> protector(webView);
    ASSERT(webView);

#if ENABLE(NETSCAPE_PLUGIN_API)
    // Windows Media Player has a modal message loop that will deliver messages
    // to us at inappropriate times and we will crash if we handle them when
    // they are delivered. We repost paint messages so that we eventually get
    // a chance to paint once the modal loop has exited, but other messages
    // aren't safe to repost, so we just drop them.
    if (PluginView::isCallingPlugin()) {
        if (message == WM_PAINT)
            PostMessage(hWnd, message, wParam, lParam);
        return 0;
    }
#endif

    bool handled = true;

    switch (message) {
        case WM_PAINT: {
            webView->paint(0, 0);
#if !OS(WINCE)
            if (webView->usesLayeredWindow())
                webView->performLayeredWindowUpdate();
#endif
            break;
        }
        case WM_ERASEBKGND:
#if !OS(WINCE)
            if (webView->usesLayeredWindow()) {
                // Don't perform a background erase for transparent views.
                handled = true;
                lResult = 1;
            }
#endif
            break;
#if !OS(WINCE)
        case WM_PRINTCLIENT:
            webView->paint((HDC)wParam, lParam);
            break;
#endif
        case WM_DESTROY:
            webView->setIsBeingDestroyed();
            webView->close();
            break;
#if USE(XP_GESTURE_MODEL)
        case WM_GESTURENOTIFY:
            handled = webView->gestureNotify(wParam, lParam);
            break;
#endif
        case WM_GESTURE:
            handled = webView->gesture(wParam, lParam);
            break;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
#if !OS(WINCE)
        case WM_MOUSELEAVE:
#endif
        case WM_CANCELMODE:
            if (Frame* coreFrame = core(mainFrameImpl))
                if (coreFrame->view()->didFirstLayout())
                    handled = webView->handleMouseEvent(message, wParam, lParam);
            break;
        case WM_MOUSEWHEEL:
        case WM_VISTA_MOUSEHWHEEL:
            if (Frame* coreFrame = core(mainFrameImpl))
                if (coreFrame->view()->didFirstLayout())
                    handled = webView->mouseWheel(wParam, lParam, message == WM_VISTA_MOUSEHWHEEL);
            break;
        case WM_SYSKEYDOWN:
            handled = webView->keyDown(wParam, lParam, true);
            break;
        case WM_KEYDOWN:
            handled = webView->keyDown(wParam, lParam);
            break;
        case WM_SYSKEYUP:
            handled = webView->keyUp(wParam, lParam, true);
            break;
        case WM_KEYUP:
            handled = webView->keyUp(wParam, lParam);
            break;
        case WM_SYSCHAR:
            handled = webView->keyPress(wParam, lParam, true);
            break;
        case WM_CHAR:
            handled = webView->keyPress(wParam, lParam);
            break;
        // FIXME: We need to check WM_UNICHAR to support supplementary characters (that don't fit in 16 bits).
        case WM_SIZE:
            if (lParam != 0)
                webView->sizeChanged(IntSize(LOWORD(lParam), HIWORD(lParam)));
            break;
        case WM_SHOWWINDOW:
            lResult = DefWindowProcW(hWnd, message, wParam, lParam);
            if (wParam == 0) {
                // The window is being hidden (e.g., because we switched tabs).
                // Null out our backing store.
                webView->deleteBackingStore();
            }
            break;
        case WM_SETFOCUS: {
            COMPtr<IWebUIDelegate> uiDelegate;
            COMPtr<IWebUIDelegatePrivate> uiDelegatePrivate;
            if (SUCCEEDED(webView->uiDelegate(&uiDelegate)) && uiDelegate
                && SUCCEEDED(uiDelegate->QueryInterface(IID_IWebUIDelegatePrivate, (void**) &uiDelegatePrivate)) && uiDelegatePrivate)
                uiDelegatePrivate->webViewReceivedFocus(webView);

            FocusController* focusController = webView->page()->focusController();
            if (Frame* frame = focusController->focusedFrame()) {
                // Send focus events unless the previously focused window is a
                // child of ours (for example a plugin).
                if (!IsChild(hWnd, reinterpret_cast<HWND>(wParam)))
                    focusController->setFocused(true);
            } else
                focusController->setFocused(true);
            break;
        }
        case WM_KILLFOCUS: {
            COMPtr<IWebUIDelegate> uiDelegate;
            COMPtr<IWebUIDelegatePrivate> uiDelegatePrivate;
            HWND newFocusWnd = reinterpret_cast<HWND>(wParam);
            if (SUCCEEDED(webView->uiDelegate(&uiDelegate)) && uiDelegate
                && SUCCEEDED(uiDelegate->QueryInterface(IID_IWebUIDelegatePrivate, (void**) &uiDelegatePrivate)) && uiDelegatePrivate)
                uiDelegatePrivate->webViewLostFocus(webView, (OLE_HANDLE)(ULONG64)newFocusWnd);

            FocusController* focusController = webView->page()->focusController();
            Frame* frame = focusController->focusedOrMainFrame();
            webView->resetIME(frame);
            // Send blur events unless we're losing focus to a child of ours.
            if (!IsChild(hWnd, newFocusWnd))
                focusController->setFocused(false);

            // If we are pan-scrolling when we lose focus, stop the pan scrolling.
            frame->eventHandler()->stopAutoscrollTimer();

            break;
        }
        case WM_WINDOWPOSCHANGED:
            if (reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW)
                webView->updateActiveStateSoon();
            handled = false;
            break;
        case WM_CUT:
            webView->cut(0);
            break;
        case WM_COPY:
            webView->copy(0);
            break;
        case WM_PASTE:
            webView->paste(0);
            break;
        case WM_CLEAR:
            webView->delete_(0);
            break;
        case WM_COMMAND:
            if (HIWORD(wParam))
                handled = webView->execCommand(wParam, lParam);
#if ENABLE(CONTEXT_MENUS)
            else // If the high word of wParam is 0, the message is from a menu
                webView->performContextMenuAction(wParam, lParam, false);
#endif
            break;
#if ENABLE(CONTEXT_MENUS)
#if !OS(WINCE)
        case WM_MENUCOMMAND:
            webView->performContextMenuAction(wParam, lParam, true);
            break;
#endif
        case WM_CONTEXTMENU:
            handled = webView->handleContextMenuEvent(wParam, lParam);
            break;
        case WM_INITMENUPOPUP:
            handled = webView->onInitMenuPopup(wParam, lParam);
            break;
        case WM_MEASUREITEM:
            handled = webView->onMeasureItem(wParam, lParam);
            break;
        case WM_DRAWITEM:
            handled = webView->onDrawItem(wParam, lParam);
            break;
#if !OS(WINCE)
        case WM_UNINITMENUPOPUP:
            handled = webView->onUninitMenuPopup(wParam, lParam);
            break;
#endif
#endif
        case WM_XP_THEMECHANGED:
            if (Frame* coreFrame = core(mainFrameImpl)) {
                webView->deleteBackingStore();
                coreFrame->page()->theme()->themeChanged();
                ScrollbarTheme::theme()->themeChanged();
                RECT windowRect;
                ::GetClientRect(hWnd, &windowRect);
                ::InvalidateRect(hWnd, &windowRect, false);
#if USE(ACCELERATED_COMPOSITING)
                if (webView->isAcceleratedCompositing())
                    webView->m_backingLayer->setNeedsDisplay();
#endif
           }
            break;
#if !OS(WINCE)
        case WM_MOUSEACTIVATE:
            webView->setMouseActivated(true);
            handled = false;
            break;
#endif
        case WM_GETDLGCODE: {
            COMPtr<IWebUIDelegate> uiDelegate;
            COMPtr<IWebUIDelegatePrivate> uiDelegatePrivate;
            LONG_PTR dlgCode = 0;
            UINT keyCode = 0;
            if (lParam) {
                LPMSG lpMsg = (LPMSG)lParam;
                if (lpMsg->message == WM_KEYDOWN)
                    keyCode = (UINT) lpMsg->wParam;
            }
            if (SUCCEEDED(webView->uiDelegate(&uiDelegate)) && uiDelegate
                && SUCCEEDED(uiDelegate->QueryInterface(IID_IWebUIDelegatePrivate, (void**) &uiDelegatePrivate)) && uiDelegatePrivate
                && SUCCEEDED(uiDelegatePrivate->webViewGetDlgCode(webView, keyCode, &dlgCode)))
                return dlgCode;
            handled = false;
            break;
        }
#if HAVE(ACCESSIBILITY)
        case WM_GETOBJECT:
            handled = webView->onGetObject(wParam, lParam, lResult);
            break;
#endif
        case WM_IME_STARTCOMPOSITION:
            handled = webView->onIMEStartComposition();
            break;
        case WM_IME_REQUEST:
            lResult = webView->onIMERequest(wParam, lParam);
            break;
        case WM_IME_COMPOSITION:
            handled = webView->onIMEComposition(lParam);
            break;
        case WM_IME_ENDCOMPOSITION:
            handled = webView->onIMEEndComposition();
            break;
        case WM_IME_CHAR:
            handled = webView->onIMEChar(wParam, lParam);
            break;
        case WM_IME_NOTIFY:
            handled = webView->onIMENotify(wParam, lParam, &lResult);
            break;
        case WM_IME_SELECT:
            handled = webView->onIMESelect(wParam, lParam);
            break;
        case WM_IME_SETCONTEXT:
            handled = webView->onIMESetContext(wParam, lParam);
            break;
        case WM_TIMER:
            switch (wParam) {
                case UpdateActiveStateTimer:
                    KillTimer(hWnd, UpdateActiveStateTimer);
                    webView->updateActiveState();
                    break;
                case DeleteBackingStoreTimer:
                    webView->deleteBackingStore();
                    break;
            }
            break;
        case WM_SETCURSOR:
            handled = ::SetCursor(webView->m_lastSetCursor);
            break;
        case WM_VSCROLL:
            handled = webView->verticalScroll(wParam, lParam);
            break;
        case WM_HSCROLL:
            handled = webView->horizontalScroll(wParam, lParam);
            break;
        default:
            handled = false;
            break;
    }

#if USE(SIMULATED_GESTURES)
        lResult = DefWindowProc_Proxy(WebViewWndProc, hWnd, message, wParam, lParam);
#else
        lResult = DefWindowProcW(hWnd, message, wParam, lParam);
#endif

    // Let the client know whether we consider this message handled.
    return (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) ? !handled : lResult;
}

bool WebView::developerExtrasEnabled() const
{
    if (m_preferences->developerExtrasDisabledByOverride())
        return false;

#ifdef NDEBUG
    BOOL enabled;
    return SUCCEEDED(m_preferences->developerExtrasEnabled(&enabled)) && enabled;
#else
    return true;
#endif
}

static String webKitVersionString()
{
    LPWSTR buildNumberStringPtr;
    if (!::LoadStringW(gInstance, BUILD_NUMBER, reinterpret_cast<LPWSTR>(&buildNumberStringPtr), 0) || !buildNumberStringPtr)
        return "534+";

    return buildNumberStringPtr;
}

const String& WebView::userAgentForKURL(const KURL&)
{
    if (m_userAgentOverridden)
        return m_userAgentCustom;

    if (!m_userAgentStandard.length())
        m_userAgentStandard = WebView::standardUserAgentWithApplicationName(m_applicationName);
    return m_userAgentStandard;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, CLSID_WebView))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebView*>(this);
    else if (IsEqualGUID(riid, IID_IWebView))
        *ppvObject = static_cast<IWebView*>(this);
    else if (IsEqualGUID(riid, IID_IWebViewPrivate))
        *ppvObject = static_cast<IWebViewPrivate*>(this);
    else if (IsEqualGUID(riid, IID_IWebIBActions))
        *ppvObject = static_cast<IWebIBActions*>(this);
    else if (IsEqualGUID(riid, IID_IWebViewCSS))
        *ppvObject = static_cast<IWebViewCSS*>(this);
    else if (IsEqualGUID(riid, IID_IWebViewEditing))
        *ppvObject = static_cast<IWebViewEditing*>(this);
    else if (IsEqualGUID(riid, IID_IWebViewUndoableEditing))
        *ppvObject = static_cast<IWebViewUndoableEditing*>(this);
    else if (IsEqualGUID(riid, IID_IWebViewEditingActions))
        *ppvObject = static_cast<IWebViewEditingActions*>(this);
    else if (IsEqualGUID(riid, IID_IWebNotificationObserver))
        *ppvObject = static_cast<IWebNotificationObserver*>(this);
#if ENABLE(DRAG_SUPPORT)
    else if (IsEqualGUID(riid, IID_IDropTarget))
        *ppvObject = static_cast<IDropTarget*>(this);
#endif
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebView::AddRef(void)
{
    ASSERT(!m_deletionHasBegun);
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebView::Release(void)
{
    ASSERT(!m_deletionHasBegun);

    if (m_refCount == 1) {
        // Call close() now so that clients don't have to. (It's harmless to call close() multiple
        // times.) We do this here instead of in our destructor because close() can cause AddRef()
        // and Release() to be called, and if that happened in our destructor we would be destroyed
        // more than once.
        close();
    }

    ULONG newRef = --m_refCount;
    if (!newRef) {
#if !ASSERT_DISABLED
        m_deletionHasBegun = true;
#endif
        delete(this);
    }

    return newRef;
}

// IWebView --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::canShowMIMEType( 
    /* [in] */ BSTR mimeType,
    /* [retval][out] */ BOOL* canShow)
{
    String mimeTypeStr = toString(mimeType);

    if (!canShow)
        return E_POINTER;

    Frame* coreFrame = core(m_mainFrame);
    bool allowPlugins = coreFrame && coreFrame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);

    *canShow = MIMETypeRegistry::isSupportedImageMIMEType(mimeTypeStr)
        || MIMETypeRegistry::isSupportedNonImageMIMEType(mimeTypeStr);

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (!*canShow && m_page && m_page->pluginData()) {
        *canShow = (m_page->pluginData()->supportsMimeType(mimeTypeStr, PluginData::AllPlugins) && allowPlugins)
            || m_page->pluginData()->supportsMimeType(mimeTypeStr, PluginData::OnlyApplicationPlugins);
    }
#endif

    if (!*canShow)
        *canShow = shouldUseEmbeddedView(mimeTypeStr);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canShowMIMETypeAsHTML( 
    /* [in] */ BSTR /*mimeType*/,
    /* [retval][out] */ BOOL* canShow)
{
    // FIXME
    *canShow = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::MIMETypesShownAsHTML( 
    /* [retval][out] */ IEnumVARIANT** /*enumVariant*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::setMIMETypesShownAsHTML( 
        /* [size_is][in] */ BSTR* /*mimeTypes*/,
        /* [in] */ int /*cMimeTypes*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::URLFromPasteboard( 
    /* [in] */ IDataObject* /*pasteboard*/,
    /* [retval][out] */ BSTR* /*url*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::URLTitleFromPasteboard( 
    /* [in] */ IDataObject* /*pasteboard*/,
    /* [retval][out] */ BSTR* /*urlTitle*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

static void WebKitSetApplicationCachePathIfNecessary()
{
    static bool initialized = false;
    if (initialized)
        return;

    String path = localUserSpecificStorageDirectory();
    if (!path.isNull())
        cacheStorage().setCacheDirectory(path);

    initialized = true;
}

bool WebView::shouldInitializeTrackPointHack()
{
    static bool shouldCreateScrollbars;
    static bool hasRunTrackPointCheck;

    if (hasRunTrackPointCheck)
        return shouldCreateScrollbars;

    hasRunTrackPointCheck = true;
    const WCHAR trackPointKeys[][50] = { L"Software\\Lenovo\\TrackPoint",
        L"Software\\Lenovo\\UltraNav",
        L"Software\\Alps\\Apoint\\TrackPoint",
        L"Software\\Synaptics\\SynTPEnh\\UltraNavUSB",
        L"Software\\Synaptics\\SynTPEnh\\UltraNavPS2" };

    for (int i = 0; i < 5; ++i) {
        HKEY trackPointKey;
        int readKeyResult = ::RegOpenKeyExW(HKEY_CURRENT_USER, trackPointKeys[i], 0, KEY_READ, &trackPointKey);
        ::RegCloseKey(trackPointKey);
        if (readKeyResult == ERROR_SUCCESS) {
            shouldCreateScrollbars = true;
            return shouldCreateScrollbars;
        }
    }

    return shouldCreateScrollbars;
}
    
HRESULT STDMETHODCALLTYPE WebView::initWithFrame( 
    /* [in] */ RECT frame,
    /* [in] */ BSTR frameName,
    /* [in] */ BSTR groupName)
{
    HRESULT hr = S_OK;

    if (m_viewWindow)
        return E_FAIL;

    registerWebViewWindowClass();

#if OS(WINCE)
    HWND parentHwnd = m_hostWindow;
#else
    HWND parentHwnd = m_hostWindow ? m_hostWindow : HWND_MESSAGE;
#endif

    m_viewWindow = CreateWindowEx(0, kWebViewWindowClassName, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        frame.left, frame.top, frame.right - frame.left, frame.bottom - frame.top, parentHwnd, 0, gInstance, 0);
    ASSERT(::IsWindow(m_viewWindow));

#if USE(CE_GESTURE_MODEL)
    EnableGesturesPtr()(m_viewWindow, TGF_GID_PAN | TGF_GID_SCROLL, TGF_SCOPE_WINDOW);
#endif
    if (shouldInitializeTrackPointHack()) {
        // If we detected a registry key belonging to a TrackPoint driver, then create fake trackpoint
        // scrollbars, so the WebView will receive WM_VSCROLL and WM_HSCROLL messages. We create one
        // vertical scrollbar and one horizontal to allow for receiving both types of messages.
        ::CreateWindowW(L"SCROLLBAR", L"FAKETRACKPOINTHSCROLLBAR", WS_CHILD | WS_VISIBLE | SBS_HORZ, 0, 0, 0, 0, m_viewWindow, 0, gInstance, 0);
        ::CreateWindowW(L"SCROLLBAR", L"FAKETRACKPOINTVSCROLLBAR", WS_CHILD | WS_VISIBLE | SBS_VERT, 0, 0, 0, 0, m_viewWindow, 0, gInstance, 0);
    }

#if ENABLE(DRAG_SUPPORT)
    hr = registerDragDrop();
#endif
    if (FAILED(hr))
        return hr;

    WebPreferences* sharedPreferences = WebPreferences::sharedStandardPreferences();
    sharedPreferences->willAddToWebView();
    m_preferences = sharedPreferences;

    static bool didOneTimeInitialization;
    if (!didOneTimeInitialization) {
#if !LOG_DISABLED
        initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED

        // Initialize our platform strategies first before invoking the rest
        // of the initialization code which may depend on the strategies.
        WebPlatformStrategies::initialize();

#if ENABLE(SQL_DATABASE)
        WebKitInitializeWebDatabasesIfNecessary();
#endif
        WebKitSetApplicationCachePathIfNecessary();
        Settings::setDefaultMinDOMTimerInterval(0.004);

        didOneTimeInitialization = true;
     }

#if USE(SAFARI_THEME)
    BOOL shouldPaintNativeControls;
    if (SUCCEEDED(m_preferences->shouldPaintNativeControls(&shouldPaintNativeControls)))
        Settings::setShouldPaintNativeControls(shouldPaintNativeControls);
#endif

    BOOL useHighResolutionTimer;
    if (SUCCEEDED(m_preferences->shouldUseHighResolutionTimers(&useHighResolutionTimer)))
        Settings::setShouldUseHighResolutionTimers(useHighResolutionTimer);

#if ENABLE(INSPECTOR)
    m_inspectorClient = new WebInspectorClient(this);
#endif // ENABLE(INSPECTOR)

#if ENABLE(INSPECTOR_SERVER)
    // localAddress may include the port number too; if it's not specified
    // we will default to 9222.
    BSTR localAddress;
    if (SUCCEEDED(m_preferences->inspectorServerAddress(&localAddress))) {
        String hostUrl(localAddress, SysStringLen(localAddress));
        SysFreeString(localAddress);
        //This is where we tell clients to get their inspector front end bits from
        String frontendUrl;
        BSTR frontendUrlPref;
        if (SUCCEEDED(m_preferences->inspectorURL(&frontendUrlPref))) {
            frontendUrl = String(frontendUrlPref, SysStringLen(frontendUrlPref));
            SysFreeString(frontendUrlPref);
        }
        else {
            frontendUrl = String("http://");
            frontendUrl.append(hostUrl);
            frontendUrl.append("/inspector/front-end/inspector.html");
        }

        unsigned short port = 9222;
        size_t portPosition = hostUrl.find(L':');
        if (portPosition == notFound) {
            hostUrl = String::format("%s:%hu", hostUrl.ascii().data(), port);
        }
        else {
            port = hostUrl.substring(portPosition + 1).toUInt();
        }

        //This is where we tell inspector to get its page content from
        WebCore::WebInspectorServer::shared().setFrontendUrl(frontendUrl, hostUrl);
        WebCore::WebInspectorServer::shared().listen("0.0.0.0", port);
    }
#endif

    Page::PageClients pageClients;
    pageClients.chromeClient = new WebChromeClient(this);
#if ENABLE(CONTEXT_MENUS)
    pageClients.contextMenuClient = new WebContextMenuClient(this);
#endif
    pageClients.editorClient = new WebEditorClient(this);
#if ENABLE(DRAG_SUPPORT)
    pageClients.dragClient = new WebDragClient(this);
#endif
#if ENABLE(INSPECTOR)
    pageClients.inspectorClient = m_inspectorClient;
#endif // ENABLE(INSPECTOR)

    m_page = new Page(pageClients);
#if ENABLE(GEOLOCATION)
    provideGeolocationTo(m_page, new WebGeolocationClient(this));
#endif

    unsigned layoutMilestones = DidFirstLayout | DidFirstVisuallyNonEmptyLayout;
    m_page->addLayoutMilestones(static_cast<LayoutMilestones>(layoutMilestones));

    BString localStoragePath;
    if (SUCCEEDED(m_preferences->localStorageDatabasePath(&localStoragePath)))
        m_page->settings()->setLocalStorageDatabasePath(toString(localStoragePath));

    if (m_uiDelegate) {
        BString path;
        if (SUCCEEDED(m_uiDelegate->ftpDirectoryTemplatePath(this, &path)))
            m_page->settings()->setFTPDirectoryTemplatePath(toString(path));
    }

    WebFrame* webFrame = WebFrame::createInstance();
    RefPtr<Frame> coreFrame = webFrame->init(this, m_page, 0);
    m_mainFrame = webFrame;
    webFrame->Release(); // The WebFrame is owned by the Frame, so release our reference to it.

    coreFrame->tree()->setName(toString(frameName));
    coreFrame->init();
    setGroupName(groupName);

    addToAllWebViewsSet();

    #pragma warning(suppress: 4244)
    SetWindowLongPtr(m_viewWindow, 0, (LONG_PTR)this);
    ShowWindow(m_viewWindow, SW_SHOW);

    initializeToolTipWindow();
    windowAncestryDidChange();

    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->addObserver(this, WebPreferences::webPreferencesChangedNotification(), static_cast<IWebPreferences*>(m_preferences.get()));
    m_preferences->postPreferencesChangesNotification();

    setSmartInsertDeleteEnabled(TRUE);
    return hr;
}

static bool initCommonControls()
{
    static bool haveInitialized = false;
    if (haveInitialized)
        return true;

    INITCOMMONCONTROLSEX init;
    init.dwSize = sizeof(init);
    init.dwICC = ICC_TREEVIEW_CLASSES;
    haveInitialized = !!::InitCommonControlsEx(&init);
    return haveInitialized;
}

void WebView::initializeToolTipWindow()
{
    if (!initCommonControls())
        return;

#if OS(WINCE)
    DWORD windowStyle = WS_OVERLAPPED;
#else
    DWORD windowStyle = WS_EX_TRANSPARENT;
#endif
    m_toolTipHwnd = CreateWindowEx(windowStyle, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                   m_viewWindow, 0, 0, 0);
    if (!m_toolTipHwnd)
        return;

    TOOLINFO info = {0};
    info.cbSize = sizeof(info);
    info.uFlags = TTF_IDISHWND | TTF_SUBCLASS ;
    info.uId = reinterpret_cast<UINT_PTR>(m_viewWindow);

    ::SendMessage(m_toolTipHwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&info));
    ::SendMessage(m_toolTipHwnd, TTM_SETMAXTIPWIDTH, 0, maxToolTipWidth);

    ::SetWindowPos(m_toolTipHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void WebView::setToolTip(const String& toolTip)
{
    if (!m_toolTipHwnd)
        return;

    if (toolTip == m_toolTip)
        return;

    m_toolTip = toolTip;

    if (!m_toolTip.isEmpty()) {
        TOOLINFO info = {0};
        info.cbSize = sizeof(info);
        info.uFlags = TTF_IDISHWND;
        info.uId = reinterpret_cast<UINT_PTR>(m_viewWindow);
        info.lpszText = const_cast<UChar*>(m_toolTip.charactersWithNullTermination());
        ::SendMessage(m_toolTipHwnd, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&info));
    }

    ::SendMessage(m_toolTipHwnd, TTM_ACTIVATE, !m_toolTip.isEmpty(), 0);
}

HRESULT WebView::notifyDidAddIcon(IWebNotification* notification)
{
    COMPtr<IPropertyBag> propertyBag;
    HRESULT hr = notification->userInfo(&propertyBag);
    if (FAILED(hr))
        return hr;
    if (!propertyBag)
        return E_FAIL;

#if USE(CF)
    COMPtr<CFDictionaryPropertyBag> dictionaryPropertyBag;
    hr = propertyBag->QueryInterface(&dictionaryPropertyBag);
    if (FAILED(hr))
        return hr;

    CFDictionaryRef dictionary = dictionaryPropertyBag->dictionary();
    if (!dictionary)
        return E_FAIL;

    CFTypeRef value = CFDictionaryGetValue(dictionary, WebIconDatabase::iconDatabaseNotificationUserInfoURLKey());
    if (!value)
        return E_FAIL;
    if (CFGetTypeID(value) != CFStringGetTypeID())
        return E_FAIL;
    String frameURL = String((CFStringRef)value);
#else
    COMPtr<IPropertyBag> dictionaryPropertyBag;
    hr = propertyBag->QueryInterface(&dictionaryPropertyBag);
    if (FAILED(hr))
        return hr;

    VARIANT frameUrlVariant;
    hr = dictionaryPropertyBag->Read(WebIconDatabase::iconDatabaseNotificationUserInfoURLKey(), &frameUrlVariant, NULL);
    if (FAILED(hr))
        return hr;

    if (V_VT(&frameUrlVariant) != VT_BSTR)
        return E_FAIL;

    String frameURL = String(V_BSTR(&frameUrlVariant));
#endif
    String mainFrameURL;
    if (m_mainFrame)
        mainFrameURL = m_mainFrame->url().string();

    if (!mainFrameURL.isEmpty() && mainFrameURL == frameURL)
        dispatchDidReceiveIconFromWebFrame(m_mainFrame);

    return hr;
}

void WebView::registerForIconNotification(bool listen)
{
    IWebNotificationCenter* nc = WebNotificationCenter::defaultCenterInternal();
    if (listen)
        nc->addObserver(this, WebIconDatabase::iconDatabaseDidAddIconNotification(), 0);
    else
        nc->removeObserver(this, WebIconDatabase::iconDatabaseDidAddIconNotification(), 0);
}

void WebView::dispatchDidReceiveIconFromWebFrame(WebFrame* frame)
{
    registerForIconNotification(false);

    if (m_frameLoadDelegate)
        // FIXME: <rdar://problem/5491010> - Pass in the right HBITMAP. 
        m_frameLoadDelegate->didReceiveIcon(this, 0, frame);
}

HRESULT STDMETHODCALLTYPE WebView::setUIDelegate( 
    /* [in] */ IWebUIDelegate* d)
{
    m_uiDelegate = d;

    if (m_uiDelegatePrivate)
        m_uiDelegatePrivate = 0;

    if (d) {
        if (FAILED(d->QueryInterface(IID_IWebUIDelegatePrivate, (void**)&m_uiDelegatePrivate)))
            m_uiDelegatePrivate = 0;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::uiDelegate( 
    /* [out][retval] */ IWebUIDelegate** d)
{
    if (!m_uiDelegate)
        return E_FAIL;

    return m_uiDelegate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::setResourceLoadDelegate( 
    /* [in] */ IWebResourceLoadDelegate* d)
{
    m_resourceLoadDelegate = d;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::resourceLoadDelegate( 
    /* [out][retval] */ IWebResourceLoadDelegate** d)
{
    if (!m_resourceLoadDelegate)
        return E_FAIL;

    return m_resourceLoadDelegate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::setDownloadDelegate( 
    /* [in] */ IWebDownloadDelegate* d)
{
    m_downloadDelegate = d;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::downloadDelegate( 
    /* [out][retval] */ IWebDownloadDelegate** d)
{
    if (!m_downloadDelegate)
        return E_FAIL;

    return m_downloadDelegate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::setFrameLoadDelegate( 
    /* [in] */ IWebFrameLoadDelegate* d)
{
    m_frameLoadDelegate = d;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::frameLoadDelegate( 
    /* [out][retval] */ IWebFrameLoadDelegate** d)
{
    if (!m_frameLoadDelegate)
        return E_FAIL;

    return m_frameLoadDelegate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::setPolicyDelegate( 
    /* [in] */ IWebPolicyDelegate* d)
{
    m_policyDelegate = d;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::policyDelegate( 
    /* [out][retval] */ IWebPolicyDelegate** d)
{
    if (!m_policyDelegate)
        return E_FAIL;
    return m_policyDelegate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::mainFrame( 
    /* [out][retval] */ IWebFrame** frame)
{
    if (!frame) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *frame = m_mainFrame;
    if (!m_mainFrame)
        return E_FAIL;

    m_mainFrame->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::focusedFrame( 
    /* [out][retval] */ IWebFrame** frame)
{
    if (!frame) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *frame = 0;
    Frame* f = m_page->focusController()->focusedFrame();
    if (!f)
        return E_FAIL;

    WebFrame* webFrame = kit(f);
    if (!webFrame)
        return E_FAIL;

    return webFrame->QueryInterface(IID_IWebFrame, (void**) frame);
}

HRESULT STDMETHODCALLTYPE WebView::backForwardList( 
    /* [out][retval] */ IWebBackForwardList** list)
{
    if (!m_useBackForwardList)
        return E_FAIL;
 
    *list = WebBackForwardList::createInstance(static_cast<WebCore::BackForwardListImpl*>(m_page->backForwardList()));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setMaintainsBackForwardList( 
    /* [in] */ BOOL flag)
{
    m_useBackForwardList = !!flag;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::goBack( 
    /* [retval][out] */ BOOL* succeeded)
{
    *succeeded = m_page->goBack();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::goForward( 
    /* [retval][out] */ BOOL* succeeded)
{
    *succeeded = m_page->goForward();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::goToBackForwardItem( 
    /* [in] */ IWebHistoryItem* item,
    /* [retval][out] */ BOOL* succeeded)
{
    *succeeded = FALSE;

    COMPtr<WebHistoryItem> webHistoryItem;
    HRESULT hr = item->QueryInterface(&webHistoryItem);
    if (FAILED(hr))
        return hr;

    m_page->goToItem(webHistoryItem->historyItem(), FrameLoadTypeIndexedBackForward);
    *succeeded = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setTextSizeMultiplier( 
    /* [in] */ float multiplier)
{
    if (!m_mainFrame)
        return E_FAIL;
    setZoomMultiplier(multiplier, true);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setPageSizeMultiplier( 
    /* [in] */ float multiplier)
{
    if (!m_mainFrame)
        return E_FAIL;
    setZoomMultiplier(multiplier, false);
    return S_OK;
}

void WebView::setZoomMultiplier(float multiplier, bool isTextOnly)
{
    m_zoomMultiplier = multiplier;
    m_zoomsTextOnly = isTextOnly;

    if (Frame* coreFrame = core(m_mainFrame)) {
        if (m_zoomsTextOnly)
            coreFrame->setPageAndTextZoomFactors(1, multiplier);
        else
            coreFrame->setPageAndTextZoomFactors(multiplier, 1);
    }
}

HRESULT STDMETHODCALLTYPE WebView::textSizeMultiplier( 
    /* [retval][out] */ float* multiplier)
{
    *multiplier = zoomMultiplier(true);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::pageSizeMultiplier( 
    /* [retval][out] */ float* multiplier)
{
    *multiplier = zoomMultiplier(false);
    return S_OK;
}

float WebView::zoomMultiplier(bool isTextOnly)
{
    if (isTextOnly != m_zoomsTextOnly)
        return 1.0f;
    return m_zoomMultiplier;
}

HRESULT STDMETHODCALLTYPE WebView::setApplicationNameForUserAgent( 
    /* [in] */ BSTR applicationName)
{
    m_applicationName = toString(applicationName);
    m_userAgentStandard = String();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::applicationNameForUserAgent( 
    /* [retval][out] */ BSTR* applicationName)
{
    *applicationName = SysAllocStringLen(m_applicationName.characters(), m_applicationName.length());
    if (!*applicationName && m_applicationName.length())
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCustomUserAgent( 
    /* [in] */ BSTR userAgentString)
{
    m_userAgentOverridden = userAgentString;
    m_userAgentCustom = toString(userAgentString);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::customUserAgent( 
    /* [retval][out] */ BSTR* userAgentString)
{
    *userAgentString = 0;
    if (!m_userAgentOverridden)
        return S_OK;
    *userAgentString = SysAllocStringLen(m_userAgentCustom.characters(), m_userAgentCustom.length());
    if (!*userAgentString && m_userAgentCustom.length())
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::userAgentForURL( 
    /* [in] */ BSTR url,
    /* [retval][out] */ BSTR* userAgent)
{
    String userAgentString = userAgentForKURL(MarshallingHelpers::BSTRToKURL(url));
    *userAgent = BString(userAgentString).release();
    if (!*userAgent && userAgentString.length())
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::supportsTextEncoding( 
    /* [retval][out] */ BOOL* supports)
{
    *supports = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCustomTextEncodingName( 
    /* [in] */ BSTR encodingName)
{
    if (!m_mainFrame)
        return E_FAIL;

    HRESULT hr;
    BString oldEncoding;
    hr = customTextEncodingName(&oldEncoding);
    if (FAILED(hr))
        return hr;

    if (oldEncoding != encodingName && (!oldEncoding || !encodingName || wcscmp(oldEncoding, encodingName))) {
        if (Frame* coreFrame = core(m_mainFrame))
            coreFrame->loader()->reloadWithOverrideEncoding(toString(encodingName));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::customTextEncodingName( 
    /* [retval][out] */ BSTR* encodingName)
{
    HRESULT hr = S_OK;
    COMPtr<IWebDataSource> dataSource;
    COMPtr<WebDataSource> dataSourceImpl;
    *encodingName = 0;

    if (!m_mainFrame)
        return E_FAIL;

    if (FAILED(m_mainFrame->provisionalDataSource(&dataSource)) || !dataSource) {
        hr = m_mainFrame->dataSource(&dataSource);
        if (FAILED(hr) || !dataSource)
            return hr;
    }

    hr = dataSource->QueryInterface(IID_WebDataSource, (void**)&dataSourceImpl);
    if (FAILED(hr))
        return hr;

    BString str = dataSourceImpl->documentLoader()->overrideEncoding();
    if (FAILED(hr))
        return hr;

    if (!*encodingName)
        *encodingName = BString(m_overrideEncoding).release();

    if (!*encodingName && m_overrideEncoding.length())
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setMediaStyle( 
    /* [in] */ BSTR /*media*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::mediaStyle( 
    /* [retval][out] */ BSTR* /*media*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::stringByEvaluatingJavaScriptFromString( 
    /* [in] */ BSTR script, // assumes input does not have "JavaScript" at the begining.
    /* [retval][out] */ BSTR* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = 0;

    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return E_FAIL;

    JSC::JSValue scriptExecutionResult = coreFrame->script()->executeScript(WTF::String(script), true).jsValue();
    if (!scriptExecutionResult)
        return E_FAIL;
    else if (scriptExecutionResult.isString()) {
        JSC::ExecState* exec = coreFrame->script()->globalObject(mainThreadNormalWorld())->globalExec();
        JSC::JSLockHolder lock(exec);
        *result = BString(scriptExecutionResult.getString(exec));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::windowScriptObject( 
    /* [retval][out] */ IWebScriptObject** /*webScriptObject*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::setPreferences( 
    /* [in] */ IWebPreferences* prefs)
{
    if (!prefs)
        prefs = WebPreferences::sharedStandardPreferences();

    if (m_preferences == prefs)
        return S_OK;

    COMPtr<WebPreferences> webPrefs(Query, prefs);
    if (!webPrefs)
        return E_NOINTERFACE;
    webPrefs->willAddToWebView();

    COMPtr<WebPreferences> oldPrefs = m_preferences;

    IWebNotificationCenter* nc = WebNotificationCenter::defaultCenterInternal();
    nc->removeObserver(this, WebPreferences::webPreferencesChangedNotification(), static_cast<IWebPreferences*>(m_preferences.get()));

    BString identifier;
    oldPrefs->identifier(&identifier);
    oldPrefs->didRemoveFromWebView();
    oldPrefs = 0; // Make sure we release the reference, since WebPreferences::removeReferenceForIdentifier will check for last reference to WebPreferences

    m_preferences = webPrefs;

    if (identifier)
        WebPreferences::removeReferenceForIdentifier(identifier);

    nc->addObserver(this, WebPreferences::webPreferencesChangedNotification(), static_cast<IWebPreferences*>(m_preferences.get()));

    m_preferences->postPreferencesChangesNotification();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::preferences( 
    /* [retval][out] */ IWebPreferences** prefs)
{
    if (!prefs)
        return E_POINTER;
    *prefs = m_preferences.get();
    if (m_preferences)
        m_preferences->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setPreferencesIdentifier( 
    /* [in] */ BSTR /*anIdentifier*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::preferencesIdentifier( 
    /* [retval][out] */ BSTR* /*anIdentifier*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

static void systemParameterChanged(WPARAM parameter)
{
#if USE(CG)
    if (parameter == SPI_SETFONTSMOOTHING || parameter == SPI_SETFONTSMOOTHINGTYPE || parameter == SPI_SETFONTSMOOTHINGCONTRAST || parameter == SPI_SETFONTSMOOTHINGORIENTATION)
        wkSystemFontSmoothingChanged();
#endif
}

void WebView::windowReceivedMessage(HWND, UINT message, WPARAM wParam, LPARAM)
{
    switch (message) {
#if !OS(WINCE) && !PLATFORM(HP)
    case WM_NCACTIVATE:
        updateActiveStateSoon();
        if (!wParam)
            deleteBackingStoreSoon();
        break;
#endif
    case WM_SETTINGCHANGE:
        systemParameterChanged(wParam);
        break;
    }
}

void WebView::updateActiveStateSoon() const
{
    // This function is called while processing the WM_NCACTIVATE message.
    // While processing WM_NCACTIVATE when we are being deactivated, GetActiveWindow() will
    // still return our window. If we were to call updateActiveState() in that case, we would
    // wrongly think that we are still the active window. To work around this, we update our
    // active state after a 0-delay timer fires, at which point GetActiveWindow() will return
    // the newly-activated window.

    SetTimer(m_viewWindow, UpdateActiveStateTimer, 0, 0);
}

void WebView::deleteBackingStoreSoon()
{
    if (pendingDeleteBackingStoreSet->size() > 2) {
        Vector<WebView*> views;
        HashSet<WebView*>::iterator end = pendingDeleteBackingStoreSet->end();
        for (HashSet<WebView*>::iterator it = pendingDeleteBackingStoreSet->begin(); it != end; ++it)
            views.append(*it);
        for (int i = 0; i < views.size(); ++i)
            views[i]->deleteBackingStore();
        ASSERT(pendingDeleteBackingStoreSet->isEmpty());
    }

    pendingDeleteBackingStoreSet->add(this);
    m_deleteBackingStoreTimerActive = true;
    SetTimer(m_viewWindow, DeleteBackingStoreTimer, delayBeforeDeletingBackingStoreMsec, 0);
}

void WebView::cancelDeleteBackingStoreSoon()
{
    if (!m_deleteBackingStoreTimerActive)
        return;
    pendingDeleteBackingStoreSet->remove(this);
    m_deleteBackingStoreTimerActive = false;
    KillTimer(m_viewWindow, DeleteBackingStoreTimer);
}

HRESULT STDMETHODCALLTYPE WebView::setHostWindow( 
    /* [in] */ OLE_HANDLE oleWindow)
{
    HWND window = (HWND)(ULONG64)oleWindow;
    if (m_viewWindow) {
        if (window)
            SetParent(m_viewWindow, window);
#if !OS(WINCE)
        else if (!isBeingDestroyed()) {
            // Turn the WebView into a message-only window so it will no longer be a child of the
            // old host window and will be hidden from screen. We only do this when
            // isBeingDestroyed() is false because doing this while handling WM_DESTROY can leave
            // m_viewWindow in a weird state (see <http://webkit.org/b/29337>).
            SetParent(m_viewWindow, HWND_MESSAGE);
        }
#endif
    }

    m_hostWindow = window;

    windowAncestryDidChange();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::hostWindow( 
    /* [retval][out] */ OLE_HANDLE* window)
{
    *window = (OLE_HANDLE)(ULONG64)m_hostWindow;
    return S_OK;
}


static Frame *incrementFrame(Frame *curr, bool forward, bool wrapFlag)
{
    return forward
        ? curr->tree()->traverseNextWithWrap(wrapFlag)
        : curr->tree()->traversePreviousWithWrap(wrapFlag);
}

HRESULT STDMETHODCALLTYPE WebView::searchFor( 
    /* [in] */ BSTR str,
    /* [in] */ BOOL forward,
    /* [in] */ BOOL caseFlag,
    /* [in] */ BOOL wrapFlag,
    /* [retval][out] */ BOOL* found)
{
    if (!found)
        return E_INVALIDARG;
    
    if (!m_page || !m_page->mainFrame())
        return E_UNEXPECTED;

    if (!str || !SysStringLen(str))
        return E_INVALIDARG;

    *found = m_page->findString(toString(str), caseFlag ? TextCaseSensitive : TextCaseInsensitive, forward ? FindDirectionForward : FindDirectionBackward, wrapFlag);
    return S_OK;
}

bool WebView::active()
{
    HWND activeWindow = GetActiveWindow();
#if !OS(WINCE)
    if (usesLayeredWindow() && activeWindow == m_viewWindow)
        return true;
#endif
    // When placed on a winform we cannot always walk up the stack of
    // windows to the point where m_topLevelParent == findTopLevelParent(activeWindow).
    // GetActiveWindow() always returns NULL if we are not the active window
    // so checking its return value is enough.
    return (activeWindow);
    //return (activeWindow && m_topLevelParent == findTopLevelParent(activeWindow));
}

void WebView::updateActiveState()
{
    if (m_page)
        m_page->focusController()->setActive(active());
}

HRESULT STDMETHODCALLTYPE WebView::updateFocusedAndActiveState()
{
    updateActiveState();

    bool active = m_page->focusController()->isActive();
    Frame* mainFrame = m_page->mainFrame();
    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    mainFrame->selection()->setFocused(active && mainFrame == focusedFrame);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::executeCoreCommandByName(BSTR name, BSTR value)
{
    m_page->focusController()->focusedOrMainFrame()->editor().command(toString(name)).execute(toString(value));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::clearMainFrameName()
{
    m_page->mainFrame()->tree()->clearName();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::markAllMatchesForText(
    BSTR str, BOOL caseSensitive, BOOL highlight, UINT limit, UINT* matches)
{
    if (!matches)
        return E_INVALIDARG;

    if (!m_page || !m_page->mainFrame())
        return E_UNEXPECTED;

    if (!str || !SysStringLen(str))
        return E_INVALIDARG;

    *matches = m_page->markAllMatchesForText(toString(str), caseSensitive ? TextCaseSensitive : TextCaseInsensitive, highlight, limit);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::unmarkAllTextMatches()
{
    if (!m_page || !m_page->mainFrame())
        return E_UNEXPECTED;

    m_page->unmarkAllTextMatches();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::rectsForTextMatches(
    IEnumTextMatches** pmatches)
{
    Vector<IntRect> allRects;
    WebCore::Frame* frame = m_page->mainFrame();
    do {
        if (Document* document = frame->document()) {
            IntRect visibleRect = frame->view()->visibleContentRect();
            Vector<IntRect> frameRects = document->markers()->renderedRectsForMarkers(DocumentMarker::TextMatch);
            IntPoint frameOffset(-frame->view()->scrollOffset().width(), -frame->view()->scrollOffset().height());
            frameOffset = frame->view()->convertToContainingWindow(frameOffset);

            Vector<IntRect>::iterator end = frameRects.end();
            for (Vector<IntRect>::iterator it = frameRects.begin(); it < end; it++) {
                it->intersect(visibleRect);
                it->move(frameOffset.x(), frameOffset.y());
                allRects.append(*it);
            }
        }
        frame = incrementFrame(frame, true, false);
    } while (frame);

    return createMatchEnumerator(&allRects, pmatches);
}

HRESULT STDMETHODCALLTYPE WebView::generateSelectionImage(BOOL forceWhiteText, OLE_HANDLE* hBitmap)
{
    *hBitmap = 0;

    WebCore::Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (frame) {
        HBITMAP bitmap = imageFromSelection(frame, forceWhiteText ? TRUE : FALSE);
        *hBitmap = (OLE_HANDLE)(ULONG64)bitmap;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::selectionRect(RECT* rc)
{
    WebCore::Frame* frame = m_page->focusController()->focusedOrMainFrame();

    if (frame) {
        IntRect ir = enclosingIntRect(frame->selection()->bounds());
        ir = frame->view()->convertToContainingWindow(ir);
        ir.move(-frame->view()->scrollOffset().width(), -frame->view()->scrollOffset().height());
        rc->left = ir.x();
        rc->top = ir.y();
        rc->bottom = rc->top + ir.height();
        rc->right = rc->left + ir.width();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::registerViewClass( 
    /* [in] */ IWebDocumentView* /*view*/,
    /* [in] */ IWebDocumentRepresentation* /*representation*/,
    /* [in] */ BSTR /*forMIMEType*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::setGroupName( 
        /* [in] */ BSTR groupName)
{
    if (!m_page)
        return S_OK;
    m_page->setGroupName(toString(groupName));
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::groupName( 
        /* [retval][out] */ BSTR* groupName)
{
    *groupName = 0;
    if (!m_page)
        return S_OK;
    String groupNameString = m_page->groupName();
    *groupName = SysAllocStringLen(groupNameString.characters(), groupNameString.length());
    if (!*groupName && groupNameString.length())
        return E_OUTOFMEMORY;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::estimatedProgress( 
        /* [retval][out] */ double* estimatedProgress)
{
    *estimatedProgress = m_page->progress()->estimatedProgress();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::isLoading( 
        /* [retval][out] */ BOOL* isLoading)
{
    COMPtr<IWebDataSource> dataSource;
    COMPtr<IWebDataSource> provisionalDataSource;

    if (!isLoading)
        return E_POINTER;

    *isLoading = FALSE;

    if (!m_mainFrame)
        return E_FAIL;

    if (SUCCEEDED(m_mainFrame->dataSource(&dataSource)))
        dataSource->isLoading(isLoading);

    if (*isLoading)
        return S_OK;

    if (SUCCEEDED(m_mainFrame->provisionalDataSource(&provisionalDataSource)))
        provisionalDataSource->isLoading(isLoading);
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::elementAtPoint( 
        /* [in] */ LPPOINT point,
        /* [retval][out] */ IPropertyBag** elementDictionary)
{
    if (!elementDictionary) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *elementDictionary = 0;

    Frame* frame = core(m_mainFrame);
    if (!frame)
        return E_FAIL;

    IntPoint webCorePoint = IntPoint(point->x, point->y);
    HitTestResult result = HitTestResult(webCorePoint);
    if (frame->contentRenderer())
        result = frame->eventHandler()->hitTestResultAtPoint(webCorePoint);
    *elementDictionary = WebElementPropertyBag::createInstance(result);
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteboardTypesForSelection( 
    /* [retval][out] */ IEnumVARIANT** /*enumVariant*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::writeSelectionWithPasteboardTypes( 
        /* [size_is][in] */ BSTR* /*types*/,
        /* [in] */ int /*cTypes*/,
        /* [in] */ IDataObject* /*pasteboard*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteboardTypesForElement( 
    /* [in] */ IPropertyBag* /*elementDictionary*/,
    /* [retval][out] */ IEnumVARIANT** /*enumVariant*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::writeElement( 
        /* [in] */ IPropertyBag* /*elementDictionary*/,
        /* [size_is][in] */ BSTR* /*withPasteboardTypes*/,
        /* [in] */ int /*cWithPasteboardTypes*/,
        /* [in] */ IDataObject* /*pasteboard*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::selectedText(
        /* [out, retval] */ BSTR* text)
{
    if (!text) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *text = 0;

    Frame* focusedFrame = (m_page && m_page->focusController()) ? m_page->focusController()->focusedOrMainFrame() : 0;
    if (!focusedFrame)
        return E_FAIL;

    String frameSelectedText = focusedFrame->editor().selectedText();
    *text = SysAllocStringLen(frameSelectedText.characters(), frameSelectedText.length());
    if (!*text && frameSelectedText.length())
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::centerSelectionInVisibleArea(
        /* [in] */ IUnknown* /* sender */)
{
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return E_FAIL;

    coreFrame->selection()->revealSelection(ScrollAlignment::alignCenterAlways);
    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebView::moveDragCaretToPoint( 
        /* [in] */ LPPOINT /*point*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::removeDragCaret( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setDrawsBackground( 
        /* [in] */ BOOL /*drawsBackground*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::drawsBackground( 
        /* [retval][out] */ BOOL* /*drawsBackground*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setMainFrameURL( 
        /* [in] */ BSTR /*urlString*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::mainFrameURL( 
        /* [retval][out] */ BSTR* urlString)
{
    if (!urlString)
        return E_POINTER;

    if (!m_mainFrame)
        return E_FAIL;

    COMPtr<IWebDataSource> dataSource;

    if (FAILED(m_mainFrame->provisionalDataSource(&dataSource))) {
        if (FAILED(m_mainFrame->dataSource(&dataSource)))
            return E_FAIL;
    }

    if (!dataSource) {
        *urlString = 0;
        return S_OK;
    }
    
    COMPtr<IWebMutableURLRequest> request;
    if (FAILED(dataSource->request(&request)) || !request)
        return E_FAIL;

    if (FAILED(request->URL(urlString)))
        return E_FAIL;

    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::mainFrameDocument( 
        /* [retval][out] */ IDOMDocument** document)
{
    if (document)
        *document = 0;
    if (!m_mainFrame)
        return E_FAIL;
    return m_mainFrame->DOMDocument(document);
}
    
HRESULT STDMETHODCALLTYPE WebView::mainFrameTitle( 
        /* [retval][out] */ BSTR* /*title*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::mainFrameIcon( 
        /* [retval][out] */ OLE_HANDLE* /*hBitmap*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebView::registerURLSchemeAsLocal( 
        /* [in] */ BSTR scheme)
{
    if (!scheme)
        return E_POINTER;

    SchemeRegistry::registerURLSchemeAsLocal(toString(scheme));

    return S_OK;
}

// IWebIBActions ---------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::takeStringURLFrom( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::stopLoading( 
        /* [in] */ IUnknown* /*sender*/)
{
    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->stopLoading();
}
    
HRESULT STDMETHODCALLTYPE WebView::reload( 
        /* [in] */ IUnknown* /*sender*/)
{
    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->reload();
}
    
HRESULT STDMETHODCALLTYPE WebView::canGoBack( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    *result = !!(m_page->backForwardList()->backItem() && !m_page->defersLoading());
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::goBack( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::canGoForward( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    *result = !!(m_page->backForwardList()->forwardItem() && !m_page->defersLoading());
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::goForward( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// FIXME: This code should move into WebCore so it can be shared by all the WebKits.
#define MinimumZoomMultiplier   0.5f
#define MaximumZoomMultiplier   3.0f
#define ZoomMultiplierRatio     1.2f

HRESULT STDMETHODCALLTYPE WebView::canMakeTextLarger( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    bool canGrowMore = canZoomIn(m_zoomsTextOnly);
    *result = canGrowMore ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canZoomPageIn( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    bool canGrowMore = canZoomIn(false);
    *result = canGrowMore ? TRUE : FALSE;
    return S_OK;
}

bool WebView::canZoomIn(bool isTextOnly)
{
    return zoomMultiplier(isTextOnly) * ZoomMultiplierRatio < MaximumZoomMultiplier;
}
    
HRESULT STDMETHODCALLTYPE WebView::makeTextLarger( 
        /* [in] */ IUnknown* /*sender*/)
{
    return zoomIn(m_zoomsTextOnly);
}

HRESULT STDMETHODCALLTYPE WebView::zoomPageIn( 
        /* [in] */ IUnknown* /*sender*/)
{
    return zoomIn(false);
}

HRESULT WebView::zoomIn(bool isTextOnly)
{
    if (!canZoomIn(isTextOnly))
        return E_FAIL;
    setZoomMultiplier(zoomMultiplier(isTextOnly) * ZoomMultiplierRatio, isTextOnly);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canMakeTextSmaller( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    bool canShrinkMore = canZoomOut(m_zoomsTextOnly);
    *result = canShrinkMore ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canZoomPageOut( 
        /* [in] */ IUnknown* /*sender*/,
        /* [retval][out] */ BOOL* result)
{
    bool canShrinkMore = canZoomOut(false);
    *result = canShrinkMore ? TRUE : FALSE;
    return S_OK;
}

bool WebView::canZoomOut(bool isTextOnly)
{
    return zoomMultiplier(isTextOnly) / ZoomMultiplierRatio > MinimumZoomMultiplier;
}

HRESULT STDMETHODCALLTYPE WebView::makeTextSmaller( 
        /* [in] */ IUnknown* /*sender*/)
{
    return zoomOut(m_zoomsTextOnly);
}

HRESULT STDMETHODCALLTYPE WebView::zoomPageOut( 
        /* [in] */ IUnknown* /*sender*/)
{
    return zoomOut(false);
}

HRESULT WebView::zoomOut(bool isTextOnly)
{
    if (!canZoomOut(isTextOnly))
        return E_FAIL;
    setZoomMultiplier(zoomMultiplier(isTextOnly) / ZoomMultiplierRatio, isTextOnly);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canMakeTextStandardSize( 
    /* [in] */ IUnknown* /*sender*/,
    /* [retval][out] */ BOOL* result)
{
    // Since we always reset text zoom and page zoom together, this should continue to return an answer about text zoom even if its not enabled.
    bool notAlreadyStandard = canResetZoom(true);
    *result = notAlreadyStandard ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::canResetPageZoom( 
    /* [in] */ IUnknown* /*sender*/,
    /* [retval][out] */ BOOL* result)
{
    bool notAlreadyStandard = canResetZoom(false);
    *result = notAlreadyStandard ? TRUE : FALSE;
    return S_OK;
}

bool WebView::canResetZoom(bool isTextOnly)
{
    return zoomMultiplier(isTextOnly) != 1.0f;
}

HRESULT STDMETHODCALLTYPE WebView::makeTextStandardSize( 
    /* [in] */ IUnknown* /*sender*/)
{
    return resetZoom(true);
}

HRESULT STDMETHODCALLTYPE WebView::resetPageZoom( 
    /* [in] */ IUnknown* /*sender*/)
{
    return resetZoom(false);
}

HRESULT WebView::resetZoom(bool isTextOnly)
{
    if (!canResetZoom(isTextOnly))
        return E_FAIL;
    setZoomMultiplier(1.0f, isTextOnly);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::toggleContinuousSpellChecking( 
    /* [in] */ IUnknown* /*sender*/)
{
    HRESULT hr;
    BOOL enabled;
    if (FAILED(hr = isContinuousSpellCheckingEnabled(&enabled)))
        return hr;
    return setContinuousSpellCheckingEnabled(enabled ? FALSE : TRUE);
}

HRESULT STDMETHODCALLTYPE WebView::toggleSmartInsertDelete( 
    /* [in] */ IUnknown* /*sender*/)
{
    BOOL enabled = FALSE;
    HRESULT hr = smartInsertDeleteEnabled(&enabled);
    if (FAILED(hr))
        return hr;

    return setSmartInsertDeleteEnabled(enabled ? FALSE : TRUE);
}

HRESULT STDMETHODCALLTYPE WebView::toggleGrammarChecking( 
    /* [in] */ IUnknown* /*sender*/)
{
    BOOL enabled;
    HRESULT hr = isGrammarCheckingEnabled(&enabled);
    if (FAILED(hr))
        return hr;

    return setGrammarCheckingEnabled(enabled ? FALSE : TRUE);
}

HRESULT STDMETHODCALLTYPE WebView::reloadFromOrigin( 
        /* [in] */ IUnknown* /*sender*/)
{
    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->reloadFromOrigin();
}

// IWebViewCSS -----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::computedStyleForElement( 
        /* [in] */ IDOMElement* /*element*/,
        /* [in] */ BSTR /*pseudoElement*/,
        /* [retval][out] */ IDOMCSSStyleDeclaration** /*style*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// IWebViewEditing -------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::editableDOMRangeForPoint( 
        /* [in] */ LPPOINT /*point*/,
        /* [retval][out] */ IDOMRange** /*range*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setSelectedDOMRange( 
        /* [in] */ IDOMRange* /*range*/,
        /* [in] */ WebSelectionAffinity /*affinity*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::selectedDOMRange( 
        /* [retval][out] */ IDOMRange** /*range*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::selectionAffinity( 
        /* [retval][out][retval][out] */ WebSelectionAffinity* /*affinity*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setEditable( 
        /* [in] */ BOOL /*flag*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::isEditable( 
        /* [retval][out] */ BOOL* /*isEditable*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setTypingStyle( 
        /* [in] */ IDOMCSSStyleDeclaration* /*style*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::typingStyle( 
        /* [retval][out] */ IDOMCSSStyleDeclaration** /*style*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setSmartInsertDeleteEnabled( 
        /* [in] */ BOOL flag)
{
    if (m_page->settings()->smartInsertDeleteEnabled() != !!flag) {
        m_page->settings()->setSmartInsertDeleteEnabled(!!flag);
        setSelectTrailingWhitespaceEnabled(!flag);
    }
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::smartInsertDeleteEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    *enabled = m_page->settings()->smartInsertDeleteEnabled() ? TRUE : FALSE;
    return S_OK;
}
 
HRESULT STDMETHODCALLTYPE WebView::setSelectTrailingWhitespaceEnabled( 
        /* [in] */ BOOL flag)
{
    if (m_page->settings()->selectTrailingWhitespaceEnabled() != !!flag) {
        m_page->settings()->setSelectTrailingWhitespaceEnabled(!!flag);
        setSmartInsertDeleteEnabled(!flag);
    }
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::isSelectTrailingWhitespaceEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    *enabled = m_page->settings()->selectTrailingWhitespaceEnabled() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setContinuousSpellCheckingEnabled( 
        /* [in] */ BOOL flag)
{
    if (continuousSpellCheckingEnabled != !!flag) {
        continuousSpellCheckingEnabled = !!flag;
        COMPtr<IWebPreferences> prefs;
        if (SUCCEEDED(preferences(&prefs)))
            prefs->setContinuousSpellCheckingEnabled(flag);
    }
    
    BOOL spellCheckingEnabled;
    if (SUCCEEDED(isContinuousSpellCheckingEnabled(&spellCheckingEnabled)) && spellCheckingEnabled)
        preflightSpellChecker();
    else
        m_mainFrame->unmarkAllMisspellings();

    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::isContinuousSpellCheckingEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    *enabled = (continuousSpellCheckingEnabled && continuousCheckingAllowed()) ? TRUE : FALSE;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::spellCheckerDocumentTag( 
        /* [retval][out] */ int* tag)
{
    // we just use this as a flag to indicate that we've spell checked the document
    // and need to close the spell checker out when the view closes.
    *tag = 0;
    m_hasSpellCheckerDocumentTag = true;
    return S_OK;
}

static COMPtr<IWebEditingDelegate> spellingDelegateForTimer;

static void preflightSpellCheckerNow()
{
    spellingDelegateForTimer->preflightChosenSpellServer();
    spellingDelegateForTimer = 0;
}

static void CALLBACK preflightSpellCheckerTimerCallback(HWND, UINT, UINT_PTR id, DWORD)
{
    ::KillTimer(0, id);
    preflightSpellCheckerNow();
}

void WebView::preflightSpellChecker()
{
    // As AppKit does, we wish to delay tickling the shared spellchecker into existence on application launch.
    if (!m_editingDelegate)
        return;

    BOOL exists;
    spellingDelegateForTimer = m_editingDelegate;
    if (SUCCEEDED(m_editingDelegate->sharedSpellCheckerExists(&exists)) && exists)
        preflightSpellCheckerNow();
    else
        ::SetTimer(0, 0, 2000, preflightSpellCheckerTimerCallback);
}

bool WebView::continuousCheckingAllowed()
{
    static bool allowContinuousSpellChecking = true;
    static bool readAllowContinuousSpellCheckingDefault = false;
    if (!readAllowContinuousSpellCheckingDefault) {
        COMPtr<IWebPreferences> prefs;
        if (SUCCEEDED(preferences(&prefs))) {
            BOOL allowed;
            prefs->allowContinuousSpellChecking(&allowed);
            allowContinuousSpellChecking = !!allowed;
        }
        readAllowContinuousSpellCheckingDefault = true;
    }
    return allowContinuousSpellChecking;
}

HRESULT STDMETHODCALLTYPE WebView::undoManager( 
        /* [retval][out] */ IWebUndoManager** /*manager*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::setEditingDelegate( 
        /* [in] */ IWebEditingDelegate* d)
{
    m_editingDelegate = d;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::editingDelegate( 
        /* [retval][out] */ IWebEditingDelegate** d)
{
    if (!d) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *d = m_editingDelegate.get();
    if (!*d)
        return E_FAIL;

    (*d)->AddRef();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::styleDeclarationWithText( 
        /* [in] */ BSTR /*text*/,
        /* [retval][out] */ IDOMCSSStyleDeclaration** /*style*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::hasSelectedRange( 
        /* [retval][out] */ BOOL* hasSelectedRange)
{
    *hasSelectedRange = m_page->mainFrame()->selection()->isRange();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::cutEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    Editor& editor = m_page->focusController()->focusedOrMainFrame()->editor();
    *enabled = editor.canCut() || editor.canDHTMLCut();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::copyEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    Editor& editor = m_page->focusController()->focusedOrMainFrame()->editor();
    *enabled = editor.canCopy() || editor.canDHTMLCopy();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    Editor& editor = m_page->focusController()->focusedOrMainFrame()->editor();
    *enabled = editor.canPaste() || editor.canDHTMLPaste();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::deleteEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    *enabled = m_page->focusController()->focusedOrMainFrame()->editor().canDelete();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::editingEnabled( 
        /* [retval][out] */ BOOL* enabled)
{
    *enabled = m_page->focusController()->focusedOrMainFrame()->editor().canEdit();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::isGrammarCheckingEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = grammarCheckingEnabled ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setGrammarCheckingEnabled( 
    BOOL enabled)
{
    if (!m_editingDelegate) {
        LOG_ERROR("No NSSpellChecker");
        return E_FAIL;
    }

    if (grammarCheckingEnabled == !!enabled)
        return S_OK;
    
    grammarCheckingEnabled = !!enabled;
    COMPtr<IWebPreferences> prefs;
    if (SUCCEEDED(preferences(&prefs)))
        prefs->setGrammarCheckingEnabled(enabled);
    
    m_editingDelegate->updateGrammar();

    // We call _preflightSpellChecker when turning continuous spell checking on, but we don't need to do that here
    // because grammar checking only occurs on code paths that already preflight spell checking appropriately.
    
    BOOL grammarEnabled;
    if (SUCCEEDED(isGrammarCheckingEnabled(&grammarEnabled)) && !grammarEnabled)
        m_mainFrame->unmarkAllBadGrammar();

    return S_OK;
}

// IWebViewUndoableEditing -----------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::replaceSelectionWithNode( 
        /* [in] */ IDOMNode* /*node*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::replaceSelectionWithText( 
        /* [in] */ BSTR text)
{
    Position start = m_page->mainFrame()->selection()->selection().start();
    m_page->focusController()->focusedOrMainFrame()->editor().insertText(toString(text), 0);
    m_page->mainFrame()->selection()->setBase(start);
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::replaceSelectionWithMarkupString( 
        /* [in] */ BSTR /*markupString*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::replaceSelectionWithArchive( 
        /* [in] */ IWebArchive* /*archive*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::deleteSelection( void)
{
    Editor& editor = m_page->focusController()->focusedOrMainFrame()->editor();
    editor.deleteSelectionWithSmartDelete(editor.canSmartCopyOrDelete());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::clearSelection( void)
{
    m_page->focusController()->focusedOrMainFrame()->selection()->clear();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::applyStyle( 
        /* [in] */ IDOMCSSStyleDeclaration* /*style*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// IWebViewEditingActions ------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::copy( 
        /* [in] */ IUnknown* /*sender*/)
{
    m_page->focusController()->focusedOrMainFrame()->editor().command("Copy").execute();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::cut( 
        /* [in] */ IUnknown* /*sender*/)
{
    m_page->focusController()->focusedOrMainFrame()->editor().command("Cut").execute();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::paste( 
        /* [in] */ IUnknown* /*sender*/)
{
    m_page->focusController()->focusedOrMainFrame()->editor().command("Paste").execute();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::copyURL( 
        /* [in] */ BSTR url)
{
    m_page->focusController()->focusedOrMainFrame()->editor().copyURL(MarshallingHelpers::BSTRToKURL(url), "");
    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebView::copyFont( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteFont( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::delete_( 
        /* [in] */ IUnknown* /*sender*/)
{
    m_page->focusController()->focusedOrMainFrame()->editor().command("Delete").execute();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteAsPlainText( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::pasteAsRichText( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::changeFont( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::changeAttributes( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::changeDocumentBackgroundColor( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::changeColor( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::alignCenter( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::alignJustified( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::alignLeft( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::alignRight( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::checkSpelling( 
        /* [in] */ IUnknown* /*sender*/)
{
    if (!m_editingDelegate) {
        LOG_ERROR("No NSSpellChecker");
        return E_FAIL;
    }
    
    core(m_mainFrame)->editor().advanceToNextMisspelling();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::showGuessPanel( 
        /* [in] */ IUnknown* /*sender*/)
{
    if (!m_editingDelegate) {
        LOG_ERROR("No NSSpellChecker");
        return E_FAIL;
    }
    
    // Post-Tiger, this menu item is a show/hide toggle, to match AppKit. Leave Tiger behavior alone
    // to match rest of OS X.
    BOOL showing;
    if (SUCCEEDED(m_editingDelegate->spellingUIIsShowing(&showing)) && showing) {
        m_editingDelegate->showSpellingUI(FALSE);
    }
    
    core(m_mainFrame)->editor().advanceToNextMisspelling(true);
    m_editingDelegate->showSpellingUI(TRUE);
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE WebView::performFindPanelAction( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::startSpeaking( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE WebView::stopSpeaking( 
        /* [in] */ IUnknown* /*sender*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// IWebNotificationObserver -----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::onNotify( 
    /* [in] */ IWebNotification* notification)
{
    BString name;
    HRESULT hr = notification->name(&name);
    if (FAILED(hr))
        return hr;

    if (!wcscmp(name, WebIconDatabase::iconDatabaseDidAddIconNotification()))
        return notifyDidAddIcon(notification);

    if (!wcscmp(name, WebPreferences::webPreferencesChangedNotification()))
        return notifyPreferencesChanged(notification);

    return hr;
}

HRESULT WebView::notifyPreferencesChanged(IWebNotification* notification)
{
#define ASSERT_HR_SUCCESS(x) { ASSERT(SUCCEEDED(x)); if (FAILED(x)) CRASH(); }
    HRESULT hr;

    COMPtr<IUnknown> unkPrefs;
    hr = notification->getObject(&unkPrefs);
    ASSERT_HR_SUCCESS(hr);

    COMPtr<IWebPreferences> preferences(Query, unkPrefs);
    if (!preferences)
        return E_NOINTERFACE;

    ASSERT(preferences == m_preferences);

    BString str;
    int size;
    BOOL enabled;

    Settings* settings = m_page->settings();

    settings->setUsesEncodingDetector(true);

    hr = preferences->cursiveFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setCursiveFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->defaultFixedFontSize(&size);
    ASSERT_HR_SUCCESS(hr);
    settings->setDefaultFixedFontSize(size);

    hr = preferences->defaultFontSize(&size);
    ASSERT_HR_SUCCESS(hr);
    settings->setDefaultFontSize(size);

    hr = preferences->defaultTextEncodingName(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setDefaultTextEncodingName(toString(str));
    str.clear();

    hr = preferences->fantasyFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setFantasyFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->fixedFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setFixedFontFamily(toAtomicString(str));
    str.clear();

#if ENABLE(VIDEO_TRACK)
    hr = preferences->shouldDisplaySubtitles(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldDisplaySubtitles(enabled);

    hr = preferences->shouldDisplayCaptions(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldDisplayCaptions(enabled);

    hr = preferences->shouldDisplayTextDescriptions(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldDisplayTextDescriptions(enabled);
#endif

#if ENABLE(TOUCH_EVENTS)
    hr = preferences->emulateTouchEvents(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setTouchEventEmulationEnabled(enabled);
#endif

    preferences->mouseMoveProcessingMode(&m_mouseProcessMode);
    ASSERT_HR_SUCCESS(hr);

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (prefsPrivate) {
        hr = prefsPrivate->localStorageDatabasePath(&str);
        if (FAILED(hr))
            return hr;
        settings->setLocalStorageDatabasePath(toString(str));
        str.clear();
    }

    hr = preferences->pictographFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setPictographFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->isJavaEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setJavaEnabled(!!enabled);

    hr = preferences->isJavaScriptEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setScriptEnabled(!!enabled);

    hr = preferences->javaScriptCanOpenWindowsAutomatically(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setJavaScriptCanOpenWindowsAutomatically(!!enabled);

    hr = preferences->minimumFontSize(&size);
    ASSERT_HR_SUCCESS(hr);
    settings->setMinimumFontSize(size);

    hr = preferences->minimumLogicalFontSize(&size);
    ASSERT_HR_SUCCESS(hr);
    settings->setMinimumLogicalFontSize(size);

    hr = preferences->arePlugInsEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setPluginsEnabled(!!enabled);

    hr = preferences->isCSSRegionsEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    RuntimeEnabledFeatures::setCSSRegionsEnabled(!!enabled);

    hr = preferences->areSeamlessIFramesEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    RuntimeEnabledFeatures::setSeamlessIFramesEnabled(!!enabled);

    hr = preferences->privateBrowsingEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
#if PLATFORM(WIN) || USE(CFNETWORK)
    if (enabled)
        WebFrameNetworkingContext::ensurePrivateBrowsingSession();
    else
        WebFrameNetworkingContext::destroyPrivateBrowsingSession();
#endif
    settings->setPrivateBrowsingEnabled(!!enabled);

    hr = preferences->sansSerifFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setSansSerifFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->serifFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setSerifFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->standardFontFamily(&str);
    ASSERT_HR_SUCCESS(hr);
    settings->setStandardFontFamily(toAtomicString(str));
    str.clear();

    hr = preferences->loadsImagesAutomatically(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setLoadsImagesAutomatically(!!enabled);

    hr = preferences->userStyleSheetEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    if (enabled) {
        hr = preferences->userStyleSheetLocation(&str);
        if (FAILED(hr))
            return hr;
#if USE(CF)
        RetainPtr<CFURLRef> url = adoptCF(CFURLCreateWithString(kCFAllocatorDefault, toString(str).createCFString().get(), 0));

        // Check if the passed in string is a path and convert it to a URL.
        // FIXME: This is a workaround for nightly builds until we can get Safari to pass 
        // in an URL here. See <rdar://problem/5478378>
        if (!url) {
            DWORD len = SysStringLen(str) + 1;

            int result = WideCharToMultiByte(CP_UTF8, 0, str, len, 0, 0, 0, 0);
            Vector<UInt8> utf8Path(result);
            if (!WideCharToMultiByte(CP_UTF8, 0, str, len, (LPSTR)utf8Path.data(), result, 0, 0))
                return E_FAIL;

            url = adoptCF(CFURLCreateFromFileSystemRepresentation(0, utf8Path.data(), result - 1, false));
        }

        settings->setUserStyleSheetLocation(url.get());
#else
        settings->setUserStyleSheetLocation(MarshallingHelpers::BSTRToKURL(str));
#endif
        str.clear();
    } else
        settings->setUserStyleSheetLocation(KURL());

    hr = preferences->shouldPrintBackgrounds(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldPrintBackgrounds(!!enabled);

    hr = preferences->textAreasAreResizable(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setTextAreasAreResizable(!!enabled);

    WebKitEditableLinkBehavior behavior;
    hr = preferences->editableLinkBehavior(&behavior);
    ASSERT_HR_SUCCESS(hr);
    settings->setEditableLinkBehavior((EditableLinkBehavior)behavior);

    hr = preferences->usesPageCache(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setUsesPageCache(!!enabled);

    hr = preferences->isDOMPasteAllowed(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setDOMPasteAllowed(!!enabled);

    hr = preferences->zoomsTextOnly(&enabled);
    ASSERT_HR_SUCCESS(hr);

    if (m_zoomsTextOnly != !!enabled)
        setZoomMultiplier(m_zoomMultiplier, enabled);

    settings->setShowsURLsInToolTips(false);

    settings->setForceFTPDirectoryListings(true);
    settings->setDeveloperExtrasEnabled(developerExtrasEnabled());
    settings->setNeedsSiteSpecificQuirks(s_allowSiteSpecificHacks);

    FontSmoothingType smoothingType;
    hr = preferences->fontSmoothing(&smoothingType);
    ASSERT_HR_SUCCESS(hr);
    settings->setFontRenderingMode(smoothingType != FontSmoothingTypeWindows ? NormalRenderingMode : AlternateRenderingMode);

#if USE(AVFOUNDATION)
    hr = preferences->avFoundationEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setAVFoundationEnabled(enabled);
#endif

    if (prefsPrivate) {
        hr = prefsPrivate->authorAndUserStylesEnabled(&enabled);
        if (FAILED(hr))
            return hr;
        settings->setAuthorAndUserStylesEnabled(enabled);
    }

    hr = prefsPrivate->inApplicationChromeMode(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setApplicationChromeMode(enabled);

    hr = prefsPrivate->offlineWebApplicationCacheEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setOfflineWebApplicationCacheEnabled(enabled);

#if ENABLE(SQL_DATABASE)
    hr = prefsPrivate->databasesEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    DatabaseManager::manager().setIsAvailable(enabled);
#endif

    hr = prefsPrivate->localStorageEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setLocalStorageEnabled(enabled);

    hr = prefsPrivate->experimentalNotificationsEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setExperimentalNotificationsEnabled(enabled);

    hr = prefsPrivate->isWebSecurityEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setWebSecurityEnabled(!!enabled);

    hr = prefsPrivate->allowUniversalAccessFromFileURLs(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setAllowUniversalAccessFromFileURLs(!!enabled);

    hr = prefsPrivate->allowFileAccessFromFileURLs(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setAllowFileAccessFromFileURLs(!!enabled);

    hr = prefsPrivate->javaScriptCanAccessClipboard(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setJavaScriptCanAccessClipboard(!!enabled);

    hr = prefsPrivate->isXSSAuditorEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setXSSAuditorEnabled(!!enabled);

#if USE(SAFARI_THEME)
    hr = prefsPrivate->shouldPaintNativeControls(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldPaintNativeControls(!!enabled);
#endif

    hr = prefsPrivate->shouldUseHighResolutionTimers(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShouldUseHighResolutionTimers(enabled);

    hr = prefsPrivate->isFrameFlatteningEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setFrameFlatteningEnabled(enabled);

#if USE(ACCELERATED_COMPOSITING)
    hr = prefsPrivate->acceleratedCompositingEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setAcceleratedCompositingEnabled(enabled);
#endif

    hr = prefsPrivate->showDebugBorders(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShowDebugBorders(enabled);

    hr = prefsPrivate->showRepaintCounter(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setShowRepaintCounter(enabled);

#if ENABLE(WEB_AUDIO)
    settings->setWebAudioEnabled(true);
#endif // ENABLE(WEB_AUDIO)

#if ENABLE(WEBGL)
    settings->setWebGLEnabled(true);
#endif // ENABLE(WEBGL)

    hr = prefsPrivate->isDNSPrefetchingEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setDNSPrefetchingEnabled(enabled);

    hr = prefsPrivate->memoryInfoEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setMemoryInfoEnabled(enabled);
    
    hr = prefsPrivate->hyperlinkAuditingEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setHyperlinkAuditingEnabled(enabled);

    hr = prefsPrivate->loadsSiteIconsIgnoringImageLoadingPreference(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setLoadsSiteIconsIgnoringImageLoadingSetting(!!enabled);

    hr = prefsPrivate->showsToolTipOverTruncatedText(&enabled);
    ASSERT_HR_SUCCESS(hr);

    settings->setShowsToolTipOverTruncatedText(enabled);

    if (!m_closeWindowTimer)
        m_mainFrame->invalidate(); // FIXME

    hr = updateSharedSettingsFromPreferencesIfNeeded(preferences.get());
    ASSERT_HR_SUCCESS(hr);

#if ENABLE(FULLSCREEN_API)
    hr = prefsPrivate->isFullScreenEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setFullScreenEnabled(enabled);
#endif

    hr = prefsPrivate->mediaPlaybackRequiresUserGesture(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setMediaPlaybackRequiresUserGesture(enabled);

    hr = prefsPrivate->mediaPlaybackAllowsInline(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setMediaPlaybackAllowsInline(enabled);

    hr = prefsPrivate->shouldInvertColors(&enabled);
    ASSERT_HR_SUCCESS(hr);
    setShouldInvertColors(enabled);

    hr = prefsPrivate->requestAnimationFrameEnabled(&enabled);
    ASSERT_HR_SUCCESS(hr);
    settings->setRequestAnimationFrameEnabled(enabled);

    Vector<String> languageList;
    preferences->defaultLanguage(&str);
    if (str) {
        String language(str, SysStringLen(str));
        languageList.append(language);
        SysFreeString(str);
    }
    overrideUserPreferredLanguages(languageList);

    return S_OK;
}

HRESULT updateSharedSettingsFromPreferencesIfNeeded(IWebPreferences* preferences)
{
    if (preferences != WebPreferences::sharedStandardPreferences())
        return S_OK;

    WebKitCookieStorageAcceptPolicy acceptPolicy;
    HRESULT hr = preferences->cookieStorageAcceptPolicy(&acceptPolicy);
    if (FAILED(hr))
        return hr;

#if USE(CFNETWORK)
    WebFrameNetworkingContext::setCookieAcceptPolicyForAllContexts(acceptPolicy);
#endif

    return S_OK;
}

// IWebViewPrivate ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebView::MIMETypeForExtension(
    /* [in] */ BSTR extension,
    /* [retval][out] */ BSTR* mimeType)
{
    if (!mimeType)
        return E_POINTER;

    *mimeType = BString(MIMETypeRegistry::getMIMETypeForExtension(toString(extension))).release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCustomDropTarget(
    /* [in] */ IDropTarget* dt)
{
#if ENABLE(DRAG_SUPPORT)
    ASSERT(::IsWindow(m_viewWindow));
    if (!dt)
        return E_POINTER;
    m_hasCustomDropTarget = true;
    revokeDragDrop();
    return ::RegisterDragDrop(m_viewWindow,dt);
#else
    return E_FAIL;
#endif
}

HRESULT STDMETHODCALLTYPE WebView::removeCustomDropTarget()
{
#if ENABLE(DRAG_SUPPORT)
    if (!m_hasCustomDropTarget)
        return S_OK;
    m_hasCustomDropTarget = false;
    revokeDragDrop();
    return registerDragDrop();
#else
    return S_OK;
#endif
}

HRESULT STDMETHODCALLTYPE WebView::setInViewSourceMode( 
        /* [in] */ BOOL flag)
{
    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->setInViewSourceMode(flag);
}
    
HRESULT STDMETHODCALLTYPE WebView::inViewSourceMode( 
        /* [retval][out] */ BOOL* flag)
{
    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->inViewSourceMode(flag);
}

HRESULT STDMETHODCALLTYPE WebView::viewWindow( 
        /* [retval][out] */ OLE_HANDLE *window)
{
    *window = (OLE_HANDLE)(ULONG64)m_viewWindow;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setFormDelegate( 
    /* [in] */ IWebFormDelegate *formDelegate)
{
    m_formDelegate = formDelegate;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::formDelegate( 
    /* [retval][out] */ IWebFormDelegate **formDelegate)
{
    if (!m_formDelegate)
        return E_FAIL;

    return m_formDelegate.copyRefTo(formDelegate);
}

HRESULT STDMETHODCALLTYPE WebView::setFrameLoadDelegatePrivate( 
    /* [in] */ IWebFrameLoadDelegatePrivate* d)
{
    m_frameLoadDelegatePrivate = d;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::frameLoadDelegatePrivate( 
    /* [out][retval] */ IWebFrameLoadDelegatePrivate** d)
{
    if (!m_frameLoadDelegatePrivate)
        return E_FAIL;
        
    return m_frameLoadDelegatePrivate.copyRefTo(d);
}

HRESULT STDMETHODCALLTYPE WebView::scrollOffset( 
    /* [retval][out] */ LPPOINT offset)
{
    if (!offset)
        return E_POINTER;
    IntSize offsetIntSize = m_page->mainFrame()->view()->scrollOffset();
    offset->x = offsetIntSize.width();
    offset->y = offsetIntSize.height();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::scrollBy( 
    /* [in] */ LPPOINT offset)
{
    if (!offset)
        return E_POINTER;
    m_page->mainFrame()->view()->scrollBy(IntSize(offset->x, offset->y));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::visibleContentRect( 
    /* [retval][out] */ LPRECT rect)
{
    if (!rect)
        return E_POINTER;
    FloatRect visibleContent = m_page->mainFrame()->view()->visibleContentRect();
    rect->left = (LONG) visibleContent.x();
    rect->top = (LONG) visibleContent.y();
    rect->right = (LONG) visibleContent.maxX();
    rect->bottom = (LONG) visibleContent.maxY();
    return S_OK;
}

#if ENABLE(DRAG_SUPPORT)
static DWORD dragOperationToDragCursor(DragOperation op) {
    DWORD res = DROPEFFECT_NONE;
    if (op & DragOperationCopy) 
        res = DROPEFFECT_COPY;
    else if (op & DragOperationLink) 
        res = DROPEFFECT_LINK;
    else if (op & DragOperationMove) 
        res = DROPEFFECT_MOVE;
    else if (op & DragOperationGeneric) 
        res = DROPEFFECT_MOVE; //This appears to be the Firefox behaviour
    return res;
}

DragOperation WebView::keyStateToDragOperation(DWORD grfKeyState) const
{
    if (!m_page)
        return DragOperationNone;

    // Conforms to Microsoft's key combinations as documented for 
    // IDropTarget::DragOver. Note, grfKeyState is the current 
    // state of the keyboard modifier keys on the keyboard. See:
    // <http://msdn.microsoft.com/en-us/library/ms680129(VS.85).aspx>.
    DragOperation operation = m_page->dragController()->sourceDragOperation();

    if ((grfKeyState & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT))
        operation = DragOperationLink;
    else if ((grfKeyState & MK_CONTROL) == MK_CONTROL)
        operation = DragOperationCopy;
    else if ((grfKeyState & MK_SHIFT) == MK_SHIFT)
        operation = DragOperationGeneric;

    return operation;
}

HRESULT STDMETHODCALLTYPE WebView::DragEnter(
        IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    m_dragData = 0;

#if !OS(WINCE) && !PLATFORM(HP)
    if (m_dropTargetHelper)
        m_dropTargetHelper->DragEnter(m_viewWindow, pDataObject, (POINT*)&pt, *pdwEffect);
#endif

    POINTL localpt = pt;
    ::ScreenToClient(m_viewWindow, (LPPOINT)&localpt);
    DragData data(pDataObject, IntPoint(localpt.x, localpt.y), 
        IntPoint(pt.x, pt.y), keyStateToDragOperation(grfKeyState));
    *pdwEffect = dragOperationToDragCursor(m_page->dragController()->dragEntered(&data).operation);

    m_lastDropEffect = *pdwEffect;
    m_dragData = pDataObject;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::DragOver(
        DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
#if !OS(WINCE) && !PLATFORM(HP)
    if (m_dropTargetHelper)
        m_dropTargetHelper->DragOver((POINT*)&pt, *pdwEffect);
#endif

    if (m_dragData) {
        POINTL localpt = pt;
        ::ScreenToClient(m_viewWindow, (LPPOINT)&localpt);
        DragData data(m_dragData.get(), IntPoint(localpt.x, localpt.y), 
            IntPoint(pt.x, pt.y), keyStateToDragOperation(grfKeyState));
        *pdwEffect = dragOperationToDragCursor(m_page->dragController()->dragUpdated(&data).operation);
    } else
        *pdwEffect = DROPEFFECT_NONE;

    m_lastDropEffect = *pdwEffect;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::DragLeave()
{
#if !OS(WINCE) && !PLATFORM(HP)
    if (m_dropTargetHelper)
        m_dropTargetHelper->DragLeave();
#endif

    if (m_dragData) {
        DragData data(m_dragData.get(), IntPoint(), IntPoint(), 
            DragOperationNone);
        m_page->dragController()->dragExited(&data);
        m_dragData = 0;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::Drop(
        IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
#if !OS(WINCE) && !PLATFORM(HP)
    if (m_dropTargetHelper)
        m_dropTargetHelper->Drop(pDataObject, (POINT*)&pt, *pdwEffect);
#endif

    m_dragData = 0;
    *pdwEffect = m_lastDropEffect;
    POINTL localpt = pt;
    ::ScreenToClient(m_viewWindow, (LPPOINT)&localpt);
    DragData data(pDataObject, IntPoint(localpt.x, localpt.y), 
        IntPoint(pt.x, pt.y), keyStateToDragOperation(grfKeyState));
    m_page->dragController()->performDrag(&data);
    return S_OK;
}
#endif

HRESULT STDMETHODCALLTYPE WebView::canHandleRequest( 
    IWebURLRequest *request,
    BOOL *result)
{
    COMPtr<WebMutableURLRequest> requestImpl;

    HRESULT hr = request->QueryInterface(&requestImpl);
    if (FAILED(hr))
        return hr;

    *result = !!canHandleRequest(requestImpl->resourceRequest());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::standardUserAgentWithApplicationName( 
    BSTR applicationName,
    BSTR* groupName)
{
    if (!groupName) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *groupName;

    if (!applicationName) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *groupName = BString(standardUserAgentWithApplicationName(toString(applicationName))).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::clearFocusNode()
{
    if (m_page && m_page->focusController())
        m_page->focusController()->setFocusedNode(0, 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setInitialFocus(
    /* [in] */ BOOL forward)
{
    if (m_page && m_page->focusController()) {
        Frame* frame = m_page->focusController()->focusedOrMainFrame();
        frame->document()->setFocusedNode(0);
        m_page->focusController()->setInitialFocus(forward ? FocusDirectionForward : FocusDirectionBackward, 0);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setTabKeyCyclesThroughElements( 
    /* [in] */ BOOL cycles)
{
    if (m_page)
        m_page->setTabKeyCyclesThroughElements(!!cycles);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::tabKeyCyclesThroughElements( 
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = m_page && m_page->tabKeyCyclesThroughElements() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setAllowSiteSpecificHacks(
    /* [in] */ BOOL allow)
{
    s_allowSiteSpecificHacks = !!allow;
    // FIXME: This sets a global so it needs to call notifyPreferencesChanged
    // on all WebView objects (not just itself).
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::addAdditionalPluginDirectory( 
        /* [in] */ BSTR directory)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginDatabase::installedPlugins()->addExtraPluginDirectory(toString(directory));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE WebView::loadBackForwardListFromOtherView( 
    /* [in] */ IWebView* otherView)
{
    if (!m_page)
        return E_FAIL;
    
    // It turns out the right combination of behavior is done with the back/forward load
    // type.  (See behavior matrix at the top of WebFramePrivate.)  So we copy all the items
    // in the back forward list, and go to the current one.
    BackForwardList* backForwardList = m_page->backForwardList();
    ASSERT(!backForwardList->currentItem()); // destination list should be empty

    COMPtr<WebView> otherWebView;
    if (FAILED(otherView->QueryInterface(&otherWebView)))
        return E_FAIL;
    BackForwardList* otherBackForwardList = otherWebView->m_page->backForwardList();
    if (!otherBackForwardList->currentItem())
        return S_OK; // empty back forward list, bail
    
    HistoryItem* newItemToGoTo = 0;

    int lastItemIndex = otherBackForwardList->forwardListCount();
    for (int i = -otherBackForwardList->backListCount(); i <= lastItemIndex; ++i) {
        if (!i) {
            // If this item is showing , save away its current scroll and form state,
            // since that might have changed since loading and it is normally not saved
            // until we leave that page.
            otherWebView->m_page->mainFrame()->loader()->history()->saveDocumentAndScrollState();
        }
        RefPtr<HistoryItem> newItem = otherBackForwardList->itemAtIndex(i)->copy();
        if (!i) 
            newItemToGoTo = newItem.get();
        backForwardList->addItem(newItem.release());
    }
    
    ASSERT(newItemToGoTo);
    m_page->goToItem(newItemToGoTo, FrameLoadTypeIndexedBackForward);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::clearUndoRedoOperations()
{
    if (Frame* frame = m_page->focusController()->focusedOrMainFrame())
        frame->editor().clearUndoRedoOperations();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::shouldClose( 
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = TRUE;
    if (Frame* frame = m_page->mainFrame())
        *result = frame->loader()->shouldClose();
    return S_OK;
}

#if ENABLE(DRAG_SUPPORT)
HRESULT WebView::registerDragDrop()
{
    ASSERT(::IsWindow(m_viewWindow));
    return ::RegisterDragDrop(m_viewWindow, this);
}

HRESULT WebView::revokeDragDrop()
{
    if (!m_viewWindow)
        return S_OK;

    return ::RevokeDragDrop(m_viewWindow);
}
#endif

HRESULT WebView::setProhibitsMainFrameScrolling(BOOL b)
{
    if (!m_page)
        return E_FAIL;

    m_page->mainFrame()->view()->setProhibitsScrolling(b);
    return S_OK;
}

HRESULT WebView::setShouldApplyMacFontAscentHack(BOOL b)
{
#if OS(WINCE)
    return E_FAIL;
#else
    SimpleFontData::setShouldApplyMacAscentHack(b);
    return S_OK;
#endif
}

class IMMDict {
    typedef HIMC (CALLBACK *getContextPtr)(HWND);
    typedef BOOL (CALLBACK *releaseContextPtr)(HWND, HIMC);
    typedef LONG (CALLBACK *getCompositionStringPtr)(HIMC, DWORD, LPVOID, DWORD);
    typedef BOOL (CALLBACK *setCandidateWindowPtr)(HIMC, LPCANDIDATEFORM);
    typedef BOOL (CALLBACK *setOpenStatusPtr)(HIMC, BOOL);
    typedef BOOL (CALLBACK *notifyIMEPtr)(HIMC, DWORD, DWORD, DWORD);
    typedef BOOL (CALLBACK *associateContextExPtr)(HWND, HIMC, DWORD);

public:
    getContextPtr getContext;
    releaseContextPtr releaseContext;
    getCompositionStringPtr getCompositionString;
    setCandidateWindowPtr setCandidateWindow;
    setOpenStatusPtr setOpenStatus;
    notifyIMEPtr notifyIME;
    associateContextExPtr associateContextEx;

    static const IMMDict& dict();
private:
    IMMDict();
    HMODULE m_instance;
};

const IMMDict& IMMDict::dict()
{
    static IMMDict instance;
    return instance;
}

IMMDict::IMMDict()
{
#if OS(WINCE)
    m_instance = ::LoadLibrary(TEXT("coredll.DLL"));
#define LOAD_METHOD(x) (::GetProcAddress(m_instance, TEXT(x)))
#else
    m_instance = ::LoadLibraryW(L"IMM32.DLL");
#define LOAD_METHOD(x) (::GetProcAddress(m_instance, x))
#endif
    getContext = reinterpret_cast<getContextPtr> LOAD_METHOD("ImmGetContext");
    ASSERT(getContext);
    releaseContext = reinterpret_cast<releaseContextPtr> LOAD_METHOD("ImmReleaseContext");
    ASSERT(releaseContext);
    getCompositionString = reinterpret_cast<getCompositionStringPtr> LOAD_METHOD("ImmGetCompositionStringW");
    ASSERT(getCompositionString);
    setCandidateWindow = reinterpret_cast<setCandidateWindowPtr> LOAD_METHOD("ImmSetCandidateWindow");
    ASSERT(setCandidateWindow);
    setOpenStatus = reinterpret_cast<setOpenStatusPtr> LOAD_METHOD("ImmSetOpenStatus");
    ASSERT(setOpenStatus);
    notifyIME = reinterpret_cast<notifyIMEPtr> LOAD_METHOD("ImmNotifyIME");
    ASSERT(notifyIME);
    associateContextEx = reinterpret_cast<associateContextExPtr> LOAD_METHOD("ImmAssociateContextEx");
    ASSERT(associateContextEx);
}

HIMC WebView::getIMMContext() 
{
    HIMC context = IMMDict::dict().getContext(m_viewWindow);
    return context;
}

void WebView::releaseIMMContext(HIMC hIMC)
{
    if (!hIMC)
        return;
    IMMDict::dict().releaseContext(m_viewWindow, hIMC);
}

void WebView::prepareCandidateWindow(Frame* targetFrame, HIMC hInputContext) 
{
    IntRect caret;
    if (RefPtr<Range> range = targetFrame->selection()->selection().toNormalizedRange()) {
        ExceptionCode ec = 0;
        RefPtr<Range> tempRange = range->cloneRange(ec);
        caret = targetFrame->editor().firstRectForRange(tempRange.get());
    }
    caret = targetFrame->view()->contentsToWindow(caret);
    CANDIDATEFORM form;
    form.dwIndex = 0;
    form.dwStyle = CFS_EXCLUDE;
    form.ptCurrentPos.x = caret.x();
    form.ptCurrentPos.y = caret.y() + caret.height();
    form.rcArea.top = caret.y();
    form.rcArea.bottom = caret.maxY();
    form.rcArea.left = caret.x();
    form.rcArea.right = caret.maxX();
    IMMDict::dict().setCandidateWindow(hInputContext, &form);
}

void WebView::resetIME(Frame* targetFrame)
{
    if (targetFrame)
        targetFrame->editor().cancelComposition();

    if (HIMC hInputContext = getIMMContext()) {
        IMMDict::dict().notifyIME(hInputContext, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        releaseIMMContext(hInputContext);
    }
}

void WebView::updateSelectionForIME()
{
    Frame* targetFrame = m_page->focusController()->focusedOrMainFrame();
    
    if (!targetFrame)
        return;

    if (!targetFrame->editor().cancelCompositionIfSelectionIsInvalid())
        resetIME(targetFrame);
}

void WebView::setInputMethodState(bool enabled)
{
    IMMDict::dict().associateContextEx(m_viewWindow, 0, enabled ? IACE_DEFAULT : 0);
}

void WebView::selectionChanged()
{
    updateSelectionForIME();
}

bool WebView::onIMEStartComposition()
{
    LOG(TextInput, "onIMEStartComposition");
    m_inIMEComposition++;
    Frame* targetFrame = m_page->focusController()->focusedOrMainFrame();
    if (!targetFrame)
        return true;

    HIMC hInputContext = getIMMContext();
    prepareCandidateWindow(targetFrame, hInputContext);
    releaseIMMContext(hInputContext);
    return true;
}

static bool getCompositionString(HIMC hInputContext, DWORD type, String& result)
{
    int compositionLength = IMMDict::dict().getCompositionString(hInputContext, type, 0, 0);
    if (compositionLength <= 0)
        return false;
    Vector<UChar> compositionBuffer(compositionLength / 2);
    compositionLength = IMMDict::dict().getCompositionString(hInputContext, type, (LPVOID)compositionBuffer.data(), compositionLength);
    result = String(compositionBuffer.data(), compositionLength / 2);
    ASSERT(!compositionLength || compositionBuffer[0]);
    ASSERT(!compositionLength || compositionBuffer[compositionLength / 2 - 1]);
    return true;
}

static void compositionToUnderlines(const Vector<DWORD>& clauses, const Vector<BYTE>& attributes, Vector<CompositionUnderline>& underlines)
{
    if (clauses.isEmpty()) {
        underlines.clear();
        return;
    }
  
    const size_t numBoundaries = clauses.size() - 1;
    underlines.resize(numBoundaries);
    for (unsigned i = 0; i < numBoundaries; i++) {
        underlines[i].startOffset = clauses[i];
        underlines[i].endOffset = clauses[i + 1];
        BYTE attribute = attributes[clauses[i]];
        underlines[i].thick = attribute == ATTR_TARGET_CONVERTED || attribute == ATTR_TARGET_NOTCONVERTED;
        underlines[i].color = Color(0,0,0);
    }
}

#if !LOG_DISABLED
#define APPEND_ARGUMENT_NAME(name) \
    if (lparam & name) { \
        if (needsComma) \
            result.appendLiteral(", "); \
        result.appendLiteral(#name); \
        needsComma = true; \
    }

static String imeCompositionArgumentNames(LPARAM lparam)
{
    StringBuilder result;
    bool needsComma = false;

    APPEND_ARGUMENT_NAME(GCS_COMPATTR);
    APPEND_ARGUMENT_NAME(GCS_COMPCLAUSE);
    APPEND_ARGUMENT_NAME(GCS_COMPREADSTR);
    APPEND_ARGUMENT_NAME(GCS_COMPREADATTR);
    APPEND_ARGUMENT_NAME(GCS_COMPREADCLAUSE);
    APPEND_ARGUMENT_NAME(GCS_COMPSTR);
    APPEND_ARGUMENT_NAME(GCS_CURSORPOS);
    APPEND_ARGUMENT_NAME(GCS_DELTASTART);
    APPEND_ARGUMENT_NAME(GCS_RESULTCLAUSE);
    APPEND_ARGUMENT_NAME(GCS_RESULTREADCLAUSE);
    APPEND_ARGUMENT_NAME(GCS_RESULTREADSTR);
    APPEND_ARGUMENT_NAME(GCS_RESULTSTR);
    APPEND_ARGUMENT_NAME(CS_INSERTCHAR);
    APPEND_ARGUMENT_NAME(CS_NOMOVECARET);

    return result.toString();
}

static String imeNotificationName(WPARAM wparam)
{
    switch (wparam) {
    case IMN_CHANGECANDIDATE:
        return "IMN_CHANGECANDIDATE";
    case IMN_CLOSECANDIDATE:
        return "IMN_CLOSECANDIDATE";
    case IMN_CLOSESTATUSWINDOW:
        return "IMN_CLOSESTATUSWINDOW";
    case IMN_GUIDELINE:
        return "IMN_GUIDELINE";
    case IMN_OPENCANDIDATE:
        return "IMN_OPENCANDIDATE";
    case IMN_OPENSTATUSWINDOW:
        return "IMN_OPENSTATUSWINDOW";
    case IMN_SETCANDIDATEPOS:
        return "IMN_SETCANDIDATEPOS";
    case IMN_SETCOMPOSITIONFONT:
        return "IMN_SETCOMPOSITIONFONT";
    case IMN_SETCOMPOSITIONWINDOW:
        return "IMN_SETCOMPOSITIONWINDOW";
    case IMN_SETCONVERSIONMODE:
        return "IMN_SETCONVERSIONMODE";
    case IMN_SETOPENSTATUS:
        return "IMN_SETOPENSTATUS";
    case IMN_SETSENTENCEMODE:
        return "IMN_SETSENTENCEMODE";
    case IMN_SETSTATUSWINDOWPOS:
        return "IMN_SETSTATUSWINDOWPOS";
    default:
        return "Unknown (" + String::number(wparam) + ")";
    }
}

static String imeRequestName(WPARAM wparam)
{
    switch (wparam) {
    case IMR_CANDIDATEWINDOW:
        return "IMR_CANDIDATEWINDOW";
    case IMR_COMPOSITIONFONT:
        return "IMR_COMPOSITIONFONT";
    case IMR_COMPOSITIONWINDOW:
        return "IMR_COMPOSITIONWINDOW";
    case IMR_CONFIRMRECONVERTSTRING:
        return "IMR_CONFIRMRECONVERTSTRING";
    case IMR_DOCUMENTFEED:
        return "IMR_DOCUMENTFEED";
    case IMR_QUERYCHARPOSITION:
        return "IMR_QUERYCHARPOSITION";
    case IMR_RECONVERTSTRING:
        return "IMR_RECONVERTSTRING";
    default:
        return "Unknown (" + String::number(wparam) + ")";
    }
}
#endif

bool WebView::onIMEComposition(LPARAM lparam)
{
    LOG(TextInput, "onIMEComposition %s", imeCompositionArgumentNames(lparam).latin1().data());
    HIMC hInputContext = getIMMContext();
    if (!hInputContext)
        return true;

    Frame* targetFrame = m_page->focusController()->focusedOrMainFrame();
    if (!targetFrame || !targetFrame->editor().canEdit())
        return true;

    prepareCandidateWindow(targetFrame, hInputContext);

    if (lparam & GCS_RESULTSTR || !lparam) {
        String compositionString;
        if (!getCompositionString(hInputContext, GCS_RESULTSTR, compositionString) && lparam)
            return true;
        
        targetFrame->editor().confirmComposition(compositionString);
    } else {
        String compositionString;
        if (!getCompositionString(hInputContext, GCS_COMPSTR, compositionString))
            return true;
        
        // Composition string attributes
        int numAttributes = IMMDict::dict().getCompositionString(hInputContext, GCS_COMPATTR, 0, 0);
        Vector<BYTE> attributes(numAttributes);
        IMMDict::dict().getCompositionString(hInputContext, GCS_COMPATTR, attributes.data(), numAttributes);

        // Get clauses
        int numClauses = IMMDict::dict().getCompositionString(hInputContext, GCS_COMPCLAUSE, 0, 0);
        Vector<DWORD> clauses(numClauses / sizeof(DWORD));
        IMMDict::dict().getCompositionString(hInputContext, GCS_COMPCLAUSE, clauses.data(), numClauses);

        Vector<CompositionUnderline> underlines;
        compositionToUnderlines(clauses, attributes, underlines);

        int cursorPosition = LOWORD(IMMDict::dict().getCompositionString(hInputContext, GCS_CURSORPOS, 0, 0));

        targetFrame->editor().setComposition(compositionString, underlines, cursorPosition, 0);
    }

    return true;
}

bool WebView::onIMEEndComposition()
{
    LOG(TextInput, "onIMEEndComposition");
    // If the composition hasn't been confirmed yet, it needs to be cancelled.
    // This happens after deleting the last character from inline input hole.
    Frame* targetFrame = m_page->focusController()->focusedOrMainFrame();
    if (targetFrame && targetFrame->editor().hasComposition())
        targetFrame->editor().confirmComposition(String());

    if (m_inIMEComposition)
        m_inIMEComposition--;

    return true;
}

bool WebView::onIMEChar(WPARAM wparam, LPARAM lparam)
{
    UNUSED_PARAM(wparam);
    UNUSED_PARAM(lparam);
    LOG(TextInput, "onIMEChar U+%04X %08X", wparam, lparam);
    return true;
}

bool WebView::onIMENotify(WPARAM wparam, LPARAM, LRESULT*)
{
    UNUSED_PARAM(wparam);
    LOG(TextInput, "onIMENotify %s", imeNotificationName(wparam).latin1().data());
    return false;
}

LRESULT WebView::onIMERequestCharPosition(Frame* targetFrame, IMECHARPOSITION* charPos)
{
    if (charPos->dwCharPos && !targetFrame->editor().hasComposition())
        return 0;
    IntRect caret;
    if (RefPtr<Range> range = targetFrame->editor().hasComposition() ? targetFrame->editor().compositionRange() : targetFrame->selection()->selection().toNormalizedRange()) {
        ExceptionCode ec = 0;
        RefPtr<Range> tempRange = range->cloneRange(ec);
        tempRange->setStart(tempRange->startContainer(ec), tempRange->startOffset(ec) + charPos->dwCharPos, ec);
        caret = targetFrame->editor().firstRectForRange(tempRange.get());
    }
    caret = targetFrame->view()->contentsToWindow(caret);
    charPos->pt.x = caret.x();
    charPos->pt.y = caret.y();
    ::ClientToScreen(m_viewWindow, &charPos->pt);
    charPos->cLineHeight = caret.height();
    ::GetWindowRect(m_viewWindow, &charPos->rcDocument);
    return true;
}

LRESULT WebView::onIMERequestReconvertString(Frame* targetFrame, RECONVERTSTRING* reconvertString)
{
    RefPtr<Range> selectedRange = targetFrame->selection()->toNormalizedRange();
    String text = selectedRange->text();
    if (!reconvertString)
        return sizeof(RECONVERTSTRING) + text.length() * sizeof(UChar);

    unsigned totalSize = sizeof(RECONVERTSTRING) + text.length() * sizeof(UChar);
    if (totalSize > reconvertString->dwSize)
        return 0;
    reconvertString->dwCompStrLen = text.length();
    reconvertString->dwStrLen = text.length();
    reconvertString->dwTargetStrLen = text.length();
    reconvertString->dwStrOffset = sizeof(RECONVERTSTRING);
    memcpy(reconvertString + 1, text.characters(), text.length() * sizeof(UChar));
    return totalSize;
}

LRESULT WebView::onIMERequest(WPARAM request, LPARAM data)
{
    LOG(TextInput, "onIMERequest %s", imeRequestName(request).latin1().data());
    Frame* targetFrame = m_page->focusController()->focusedOrMainFrame();
    if (!targetFrame || !targetFrame->editor().canEdit())
        return 0;

    switch (request) {
        case IMR_RECONVERTSTRING:
            return onIMERequestReconvertString(targetFrame, (RECONVERTSTRING*)data);

        case IMR_QUERYCHARPOSITION:
            return onIMERequestCharPosition(targetFrame, (IMECHARPOSITION*)data);
    }
    return 0;
}

bool WebView::onIMESelect(WPARAM wparam, LPARAM lparam)
{
    UNUSED_PARAM(wparam);
    UNUSED_PARAM(lparam);
    LOG(TextInput, "onIMESelect locale %ld %s", lparam, wparam ? "select" : "deselect");
    return false;
}

bool WebView::onIMESetContext(WPARAM wparam, LPARAM)
{
    LOG(TextInput, "onIMESetContext %s", wparam ? "active" : "inactive");
    return false;
}

HRESULT STDMETHODCALLTYPE WebView::inspector(IWebInspector** inspector)
{
#if ENABLE(INSPECTOR)
    if (!m_webInspector)
        m_webInspector.adoptRef(WebInspector::createInstance(this, m_inspectorClient));

    return m_webInspector.copyRefTo(inspector);
#else // !ENABLE(INSPECTOR)
    return S_OK;
#endif // ENABLE(INSPECTOR)
}


HRESULT STDMETHODCALLTYPE WebView::windowAncestryDidChange()
{
#if !OS(WINCE)
    HWND newParent;
    if (m_viewWindow)
        newParent = findTopLevelParent(m_hostWindow);
    else {
        // There's no point in tracking active state changes of our parent window if we don't have
        // a window ourselves.
        newParent = 0;
    }

    if (newParent == m_topLevelParent)
        return S_OK;

    if (m_topLevelParent)
        WindowMessageBroadcaster::removeListener(m_topLevelParent, this);

    m_topLevelParent = newParent;

    if (m_topLevelParent)
        WindowMessageBroadcaster::addListener(m_topLevelParent, this);
#endif

    updateActiveState();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::paintDocumentRectToContext(
    /* [in] */ RECT rect,
    /* [in] */ OLE_HANDLE deviceContext)
{
    if (!deviceContext)
        return E_POINTER;

    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->paintDocumentRectToContext(rect, deviceContext);
}

HRESULT STDMETHODCALLTYPE WebView::paintScrollViewRectToContextAtPoint(
    /* [in] */ RECT rect,
    /* [in] */ POINT pt,
    /* [in] */ OLE_HANDLE deviceContext)
{
    if (!deviceContext)
        return E_POINTER;

    if (!m_mainFrame)
        return E_FAIL;

    return m_mainFrame->paintScrollViewRectToContextAtPoint(rect, pt, deviceContext);
}

HRESULT STDMETHODCALLTYPE WebView::reportException(
    /* [in] */ JSContextRef context,
    /* [in] */ JSValueRef exception)
{
    if (!context || !exception)
        return E_FAIL;

    JSC::ExecState* execState = toJS(context);
    JSC::JSLockHolder lock(execState);

    // Make sure the context has a DOMWindow global object, otherwise this context didn't originate from a WebView.
    if (!toJSDOMWindow(execState->lexicalGlobalObject()))
        return E_FAIL;

    WebCore::reportException(execState, toJS(execState, exception));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::elementFromJS(
    /* [in] */ JSContextRef context,
    /* [in] */ JSValueRef nodeObject,
    /* [retval][out] */ IDOMElement **element)
{
    if (!element)
        return E_POINTER;

    *element = 0;

    if (!context)
        return E_FAIL;

    if (!nodeObject)
        return E_FAIL;

    JSC::ExecState* exec = toJS(context);
    JSC::JSLockHolder lock(exec);
    Element* elt = toElement(toJS(exec, nodeObject));
    if (!elt)
        return E_FAIL;

    *element = DOMElement::createInstance(elt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCustomHTMLTokenizerTimeDelay(
    /* [in] */ double timeDelay)
{
    if (!m_page)
        return E_FAIL;

    m_page->setCustomHTMLTokenizerTimeDelay(timeDelay);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCustomHTMLTokenizerChunkSize(
    /* [in] */ int chunkSize)
{
    if (!m_page)
        return E_FAIL;

    m_page->setCustomHTMLTokenizerChunkSize(chunkSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::backingStore(
    /* [out, retval] */ OLE_HANDLE* hBitmap)
{
    if (!hBitmap)
        return E_POINTER;
    if (!m_backingStoreBitmap)
        return E_FAIL;
    *hBitmap = reinterpret_cast<OLE_HANDLE>(m_backingStoreBitmap->handle());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setTransparent(BOOL transparent)
{
#if OS(WINCE)
    if (transparent == FALSE)
        return S_OK;

    return E_FAIL;
#else
    if (m_transparent == !!transparent)
        return S_OK;

    m_transparent = transparent;
    m_mainFrame->updateBackground();
    return S_OK;
#endif
}

HRESULT STDMETHODCALLTYPE WebView::transparent(BOOL* transparent)
{
#if OS(WINCE)
    *transparent = FALSE;
    return S_OK;
#else
    if (!transparent)
        return E_POINTER;

    *transparent = this->transparent() ? TRUE : FALSE;
    return S_OK;
#endif
}

static bool setWindowStyle(HWND window, int index, LONG_PTR newValue)
{
    // According to MSDN, if the last value of the flag we are setting was zero,
    // then SetWindowLongPtr returns zero, even though the call succeeded. So,
    // we have to clear the error state, then check the last error after
    // setting the value to see if it actually was a failure.
    ::SetLastError(0);
    return ::SetWindowLongPtr(window, index, newValue) || !::GetLastError();
}

HRESULT WebView::setUsesLayeredWindow(BOOL usesLayeredWindow)
{
#if OS(WINCE)
    if (usesLayeredWindow == FALSE)
        return S_OK;

    return E_FAIL;
#else
    if (m_usesLayeredWindow == !!usesLayeredWindow)
        return S_OK;

    if (!m_viewWindow)
        return E_FAIL;

    RECT rect;
    ::GetWindowRect(m_viewWindow, &rect);

    LONG_PTR origExStyle = ::GetWindowLongPtr(m_viewWindow, GWL_EXSTYLE);
    LONG_PTR origStyle = ::GetWindowLongPtr(m_viewWindow, GWL_STYLE);

    // The logic here has to account for the way SetParent works.
    // According to MSDN, to go from a child window to a popup window,
    // you must clear the child bit after setting the parent to 0.
    // On the other hand, to go from a popup window to a child, you
    // must clear the popup state before setting the parent.
    if (usesLayeredWindow) {
        LONG_PTR newExStyle = origExStyle | WS_EX_LAYERED;
        LONG_PTR newStyle = (origStyle & ~(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)) | WS_POPUP;

        HWND origParent = ::SetParent(m_viewWindow, 0);

        if (!setWindowStyle(m_viewWindow, GWL_STYLE, newStyle)) {
            ::SetParent(m_viewWindow, origParent);
            return E_FAIL;
        }

        if (!setWindowStyle(m_viewWindow, GWL_EXSTYLE, newExStyle)) {
            setWindowStyle(m_viewWindow, GWL_STYLE, origStyle);
            ::SetParent(m_viewWindow, origParent);
            return E_FAIL;
        }
    } else {
        LONG_PTR newExStyle = origExStyle & ~WS_EX_LAYERED;
        LONG_PTR newStyle = (origStyle & ~WS_POPUP) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

        if (!setWindowStyle(m_viewWindow, GWL_EXSTYLE, newExStyle))
            return E_FAIL;

        if (!setWindowStyle(m_viewWindow, GWL_STYLE, newStyle)) {
            setWindowStyle(m_viewWindow, GWL_EXSTYLE, origExStyle);
            return E_FAIL;
        }

        ::SetParent(m_viewWindow, m_hostWindow ? m_hostWindow : HWND_MESSAGE);
    }

    // MSDN indicates that SetWindowLongPtr doesn't take effect for some settings until a
    // SetWindowPos is called.
    ::SetWindowPos(m_viewWindow, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    m_usesLayeredWindow = usesLayeredWindow;
    return S_OK;
#endif
}

HRESULT WebView::usesLayeredWindow(BOOL* usesLayeredWindow)
{
    if (!usesLayeredWindow)
        return E_POINTER;

#if OS(WINCE)
    *usesLayeredWindow = FALSE;
#else
    *usesLayeredWindow = this->usesLayeredWindow() ? TRUE : FALSE;
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setCookieEnabled(BOOL enable)
{
    if (!m_page)
        return E_FAIL;

    m_page->settings()->setCookieEnabled(enable);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::cookieEnabled(BOOL* enabled)
{
    if (!enabled)
        return E_POINTER;

    if (!m_page)
        return E_FAIL;

    *enabled = m_page->settings()->cookieEnabled();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setMediaVolume(float volume)
{
    if (!m_page)
        return E_FAIL;

    m_page->setMediaVolume(volume);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::mediaVolume(float* volume)
{
    if (!volume)
        return E_POINTER;

    if (!m_page)
        return E_FAIL;

    *volume = m_page->mediaVolume();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setDefersCallbacks(BOOL defersCallbacks)
{
    if (!m_page)
        return E_FAIL;

    m_page->setDefersLoading(defersCallbacks);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::defersCallbacks(BOOL* defersCallbacks)
{
    if (!defersCallbacks)
        return E_POINTER;

    if (!m_page)
        return E_FAIL;

    *defersCallbacks = m_page->defersLoading();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::globalHistoryItem(IWebHistoryItem** item)
{
    if (!item)
        return E_POINTER;

    if (!m_page)
        return E_FAIL;

    if (!m_globalHistoryItem) {
        *item = 0;
        return S_OK;
    }

    *item = WebHistoryItem::createInstance(m_globalHistoryItem);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::setAlwaysUsesComplexTextCodePath(BOOL complex)
{
    WebCoreSetAlwaysUsesComplexTextCodePath(complex);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::alwaysUsesComplexTextCodePath(BOOL* complex)
{
    if (!complex)
        return E_POINTER;

    *complex = WebCoreAlwaysUsesComplexTextCodePath();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::registerEmbeddedViewMIMEType(BSTR mimeType)
{
    if (!mimeType)
        return E_POINTER;

    if (!m_embeddedViewMIMETypes)
        m_embeddedViewMIMETypes = adoptPtr(new HashSet<String>);

    m_embeddedViewMIMETypes->add(toString(mimeType));
    return S_OK;
}

bool WebView::shouldUseEmbeddedView(const WTF::String& mimeType) const
{
    if (!m_embeddedViewMIMETypes)
        return false;

    return m_embeddedViewMIMETypes->contains(mimeType);
}

#if HAVE(ACCESSIBILITY)
bool WebView::onGetObject(WPARAM wParam, LPARAM lParam, LRESULT& lResult) const
{
    lResult = 0;

    if (lParam != OBJID_CLIENT)
        return false;

    AXObjectCache::enableAccessibility();

    // Get the accessible object for the top-level frame.
    WebFrame* mainFrameImpl = topLevelFrame();
    if (!mainFrameImpl)
        return false;

    COMPtr<IAccessible> accessible = mainFrameImpl->accessible();
    if (!accessible)
        return false;

    if (!accessibilityLib) {
        if (!(accessibilityLib = ::LoadLibraryW(L"oleacc.dll")))
            return false;
    }

    static LPFNLRESULTFROMOBJECT procPtr = reinterpret_cast<LPFNLRESULTFROMOBJECT>(::GetProcAddress(accessibilityLib, "LresultFromObject"));
    if (!procPtr)
        return false;

    // LresultFromObject returns a reference to the accessible object, stored
    // in an LRESULT. If this call is not successful, Windows will handle the
    // request through DefWindowProc.
    return SUCCEEDED(lResult = procPtr(__uuidof(IAccessible), wParam, accessible.get()));
}

STDMETHODIMP WebView::AccessibleObjectFromWindow(HWND hwnd, DWORD objectID, REFIID riid, void** ppObject)
{
    ASSERT(accessibilityLib);
    static LPFNACCESSIBLEOBJECTFROMWINDOW procPtr = reinterpret_cast<LPFNACCESSIBLEOBJECTFROMWINDOW>(::GetProcAddress(accessibilityLib, "AccessibleObjectFromWindow"));
    if (!procPtr)
        return E_FAIL;
    return procPtr(hwnd, objectID, riid, ppObject);
}
#endif

HRESULT WebView::setMemoryCacheDelegateCallsEnabled(BOOL enabled)
{
    m_page->setMemoryCacheClientCallsEnabled(enabled);
    return S_OK;
}

HRESULT WebView::setJavaScriptURLsAreAllowed(BOOL)
{
    return E_NOTIMPL;
}

HRESULT WebView::setCanStartPlugins(BOOL canStartPlugins)
{
    m_page->setCanStartMedia(canStartPlugins);
    return S_OK;
}

void WebView::enterFullscreenForNode(Node* node)
{
    if (!node->hasTagName(HTMLNames::videoTag) || !node->isElementNode())
        return;

#if ENABLE(VIDEO)
    if (!toElement(node)->isMediaElement())
        return;
    HTMLMediaElement* videoElement = static_cast<HTMLMediaElement*>(node);

    if (m_fullScreenVideoController) {
        if (m_fullScreenVideoController->mediaElement() == videoElement) {
            // The backend may just warn us that the underlaying plaftormMovie()
            // has changed. Just force an update.
            m_fullScreenVideoController->setMediaElement(videoElement);
            return; // No more to do.
        }

        // First exit Fullscreen for the old mediaElement.
        m_fullScreenVideoController->mediaElement()->exitFullscreen();
        // This previous call has to trigger exitFullscreen,
        // which has to clear m_fullScreenVideoController.
        ASSERT(!m_fullScreenVideoController);
    }

    m_fullScreenVideoController = adoptPtr(new FullscreenVideoController);
    m_fullScreenVideoController->setMediaElement(videoElement);
    m_fullScreenVideoController->enterFullscreen();
#endif
}

void WebView::exitFullscreen()
{
#if ENABLE(VIDEO)
    if (m_fullScreenVideoController)
        m_fullScreenVideoController->exitFullscreen();
    
#endif
}

static Vector<String> toStringVector(unsigned patternsCount, BSTR* patterns)
{
    Vector<String> patternsVector;
    if (!patternsCount)
        return patternsVector;
    for (unsigned i = 0; i < patternsCount; ++i)
        patternsVector.append(toString(patterns[i]));
    return patternsVector;
}

HRESULT WebView::addUserScriptToGroup(BSTR groupName, IWebScriptWorld* iWorld, BSTR source, BSTR url, 
                                      unsigned whitelistCount, BSTR* whitelist,
                                      unsigned blacklistCount, BSTR* blacklist,
                                      WebUserScriptInjectionTime injectionTime)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->addUserScriptToWorld(world->world(), toString(source), toKURL(url),
                                    toStringVector(whitelistCount, whitelist), toStringVector(blacklistCount, blacklist),
                                    injectionTime == WebInjectAtDocumentStart ? InjectAtDocumentStart : InjectAtDocumentEnd,
                                    InjectInAllFrames);

    return S_OK;
}

HRESULT WebView::addUserStyleSheetToGroup(BSTR groupName, IWebScriptWorld* iWorld, BSTR source, BSTR url,
                                          unsigned whitelistCount, BSTR* whitelist,
                                          unsigned blacklistCount, BSTR* blacklist)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->addUserStyleSheetToWorld(world->world(), toString(source), toKURL(url),
                                        toStringVector(whitelistCount, whitelist), toStringVector(blacklistCount, blacklist),
                                        InjectInAllFrames);

    return S_OK;
}

HRESULT WebView::removeUserScriptFromGroup(BSTR groupName, IWebScriptWorld* iWorld, BSTR url)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->removeUserScriptFromWorld(world->world(), toKURL(url));

    return S_OK;
}

HRESULT WebView::removeUserStyleSheetFromGroup(BSTR groupName, IWebScriptWorld* iWorld, BSTR url)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->removeUserStyleSheetFromWorld(world->world(), toKURL(url));

    return S_OK;
}

HRESULT WebView::removeUserScriptsFromGroup(BSTR groupName, IWebScriptWorld* iWorld)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->removeUserScriptsFromWorld(world->world());
    return S_OK;
}

HRESULT WebView::removeUserStyleSheetsFromGroup(BSTR groupName, IWebScriptWorld* iWorld)
{
    COMPtr<WebScriptWorld> world(Query, iWorld);
    if (!world)
        return E_POINTER;

    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->removeUserStyleSheetsFromWorld(world->world());
    return S_OK;
}

HRESULT WebView::removeAllUserContentFromGroup(BSTR groupName)
{
    String group = toString(groupName);
    if (group.isEmpty())
        return E_INVALIDARG;

    PageGroup* pageGroup = PageGroup::pageGroup(group);
    ASSERT(pageGroup);
    if (!pageGroup)
        return E_FAIL;

    pageGroup->removeAllUserContent();
    return S_OK;
}

HRESULT WebView::invalidateBackingStore(const RECT* rect)
{
    if (!IsWindow(m_viewWindow))
        return S_OK;

    RECT clientRect;
    if (!GetClientRect(m_viewWindow, &clientRect))
        return E_FAIL;

    RECT rectToInvalidate;
    if (!rect)
        rectToInvalidate = clientRect;
    else if (!IntersectRect(&rectToInvalidate, &clientRect, rect))
        return S_OK;

    repaint(rectToInvalidate, true);
    return S_OK;
}

HRESULT WebView::addOriginAccessWhitelistEntry(BSTR sourceOrigin, BSTR destinationProtocol, BSTR destinationHost, BOOL allowDestinationSubdomains)
{
    SecurityPolicy::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(toString(sourceOrigin)), toString(destinationProtocol), toString(destinationHost), allowDestinationSubdomains);
    return S_OK;
}

HRESULT WebView::removeOriginAccessWhitelistEntry(BSTR sourceOrigin, BSTR destinationProtocol, BSTR destinationHost, BOOL allowDestinationSubdomains)
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(toString(sourceOrigin)), toString(destinationProtocol), toString(destinationHost), allowDestinationSubdomains);
    return S_OK;
}

HRESULT WebView::resetOriginAccessWhitelists()
{
    SecurityPolicy::resetOriginAccessWhitelists();
    return S_OK;
}
 
HRESULT WebView::setHistoryDelegate(IWebHistoryDelegate* historyDelegate)
{
    m_historyDelegate = historyDelegate;
    return S_OK;
}

HRESULT WebView::historyDelegate(IWebHistoryDelegate** historyDelegate)
{
    if (!historyDelegate)
        return E_POINTER;

    return m_historyDelegate.copyRefTo(historyDelegate);
}

HRESULT WebView::addVisitedLinks(BSTR* visitedURLs, unsigned visitedURLCount)
{
    PageGroup& group = core(this)->group();
    
    for (unsigned i = 0; i < visitedURLCount; ++i) {
        BSTR url = visitedURLs[i];
        unsigned length = SysStringLen(url);
        group.addVisitedLink(url, length);
    }

    return S_OK;
}

void WebView::downloadURL(const KURL& url)
{
    // It's the delegate's job to ref the WebDownload to keep it alive - otherwise it will be
    // destroyed when this function returns.
    COMPtr<WebDownload> download(AdoptCOM, WebDownload::createInstance(url, m_downloadDelegate.get()));
    download->start();
}

#if USE(ACCELERATED_COMPOSITING)
void WebView::setRootChildLayer(GraphicsLayer* layer)
{
    setAcceleratedCompositing(layer ? true : false);
    if (!m_backingLayer)
        return;
    m_backingLayer->addChild(layer);
}

void WebView::flushPendingGraphicsLayerChangesSoon()
{
    if (!m_layerTreeHost)
        return;
    m_layerTreeHost->flushPendingGraphicsLayerChangesSoon();
}

void WebView::setAcceleratedCompositing(bool accelerated)
{
    if (m_isAcceleratedCompositing == accelerated || !CACFLayerTreeHost::acceleratedCompositingAvailable())
        return;

    if (accelerated) {
        m_layerTreeHost = CACFLayerTreeHost::create();
        if (m_layerTreeHost) {
            m_isAcceleratedCompositing = true;

            m_layerTreeHost->setShouldInvertColors(m_shouldInvertColors);

            m_layerTreeHost->setClient(this);
            ASSERT(m_viewWindow);
            m_layerTreeHost->setWindow(m_viewWindow);

            // FIXME: We could perhaps get better performance by never allowing this layer to
            // become tiled (or choosing a higher-than-normal tiling threshold).
            // <http://webkit.org/b/52603>
            m_backingLayer = GraphicsLayer::create(0, this);
            m_backingLayer->setDrawsContent(true);
            m_backingLayer->setContentsOpaque(true);
            RECT clientRect;
            ::GetClientRect(m_viewWindow, &clientRect);
            m_backingLayer->setSize(IntRect(clientRect).size());
            m_backingLayer->setNeedsDisplay();

            m_layerTreeHost->setRootChildLayer(PlatformCALayer::platformCALayer(m_backingLayer->platformLayer()));

            // We aren't going to be using our backing store while we're in accelerated compositing
            // mode. But don't delete it immediately, in case we switch out of accelerated
            // compositing mode soon (e.g., if we're only compositing for a :hover animation).
            deleteBackingStoreSoon();
        }
    } else {
        ASSERT(m_layerTreeHost);
        m_layerTreeHost->setClient(0);
        m_layerTreeHost->setWindow(0);
        m_layerTreeHost = 0;
        m_backingLayer = nullptr;
        m_isAcceleratedCompositing = false;
    }
}
#endif

#if PLATFORM(WIN) && USE(AVFOUNDATION)
WebCore::GraphicsDeviceAdapter* WebView::graphicsDeviceAdapter() const
{
    if (!m_layerTreeHost)
        return 0;
    return m_layerTreeHost->graphicsDeviceAdapter();
}
#endif

HRESULT WebView::unused1()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebView::unused2()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebView::unused3()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebView::unused4()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebView::unused5()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebView::setGeolocationProvider(IWebGeolocationProvider* locationProvider)
{
#if ENABLE(GEOLOCATION)
    m_geolocationProvider = locationProvider;
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebView::geolocationProvider(IWebGeolocationProvider** locationProvider)
{
#if ENABLE(GEOLOCATION)
    if (!locationProvider)
        return E_POINTER;

    if (!m_geolocationProvider)
        return E_FAIL;

    return m_geolocationProvider.copyRefTo(locationProvider);
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebView::geolocationDidChangePosition(IWebGeolocationPosition* position)
{
#if ENABLE(GEOLOCATION)
    if (!m_page)
        return E_FAIL;
    GeolocationController::from(m_page)->positionChanged(core(position));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebView::geolocationDidFailWithError(IWebError* error)
{
#if ENABLE(GEOLOCATION)
    if (!m_page)
        return E_FAIL;
    if (!error)
        return E_POINTER;

    BString description;
    if (FAILED(error->localizedDescription(&description)))
        return E_FAIL;

    RefPtr<GeolocationError> geolocationError = GeolocationError::create(GeolocationError::PositionUnavailable, toString(description));
    GeolocationController::from(m_page)->errorOccurred(geolocationError.get());
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebView::setDomainRelaxationForbiddenForURLScheme(BOOL forbidden, BSTR scheme)
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, toString(scheme));
    return S_OK;
}

HRESULT WebView::registerURLSchemeAsSecure(BSTR scheme)
{
    SchemeRegistry::registerURLSchemeAsSecure(toString(scheme));
    return S_OK;
}

HRESULT WebView::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(BSTR scheme)
{
    SchemeRegistry::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(toString(scheme));
    return S_OK;
}

HRESULT WebView::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(BSTR scheme)
{
    SchemeRegistry::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(toString(scheme));
    return S_OK;
}

HRESULT WebView::nextDisplayIsSynchronous()
{
    m_nextDisplayIsSynchronous = true;
    return S_OK;
}

#if USE(ACCELERATED_COMPOSITING)
void WebView::notifyAnimationStarted(const GraphicsLayer*, double)
{
    // We never set any animations on our backing layer.
    ASSERT_NOT_REACHED();
}

void WebView::notifyFlushRequired(const GraphicsLayer*)
{
    flushPendingGraphicsLayerChangesSoon();
}

void WebView::paintContents(const GraphicsLayer*, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect& inClip)
{
    Frame* frame = core(m_mainFrame);
    if (!frame)
        return;

    context.save();
    context.clip(inClip);
    frame->view()->paint(&context, inClip);
    context.restore();
}

void WebView::flushPendingGraphicsLayerChanges()
{
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return;
    FrameView* view = coreFrame->view();
    if (!view)
        return;
    if (!m_backingLayer)
        return;

    view->updateLayoutAndStyleIfNeededRecursive();

    // Updating layout might have taken us out of compositing mode.
    if (m_backingLayer)
        m_backingLayer->flushCompositingStateForThisLayerOnly();

    view->flushCompositingStateIncludingSubframes();
}

#endif

class EnumTextMatches : public IEnumTextMatches
{
    long m_ref;
    UINT m_index;
    Vector<IntRect> m_rects;
public:
    EnumTextMatches(Vector<IntRect>* rects) : m_index(0), m_ref(1)
    {
        m_rects = *rects;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if (IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IEnumTextMatches)) {
            *ppv = this;
            AddRef();
        }

        return *ppv?S_OK:E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        return m_ref++;
    }
    
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        if (m_ref == 1) {
            delete this;
            return 0;
        }
        else
            return m_ref--;
    }

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG, RECT* rect, ULONG* pceltFetched)
    {
        if (m_index < m_rects.size()) {
            if (pceltFetched)
                *pceltFetched = 1;
            *rect = m_rects[m_index];
            m_index++;
            return S_OK;
        }

        if (pceltFetched)
            *pceltFetched = 0;

        return S_FALSE;
    }
    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        m_index += celt;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE Reset(void)
    {
        m_index = 0;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumTextMatches**)
    {
        return E_NOTIMPL;
    }
};

HRESULT createMatchEnumerator(Vector<IntRect>* rects, IEnumTextMatches** matches)
{
    *matches = new EnumTextMatches(rects);
    return (*matches)?S_OK:E_OUTOFMEMORY;
}

Page* core(IWebView* iWebView)
{
    Page* page = 0;

    COMPtr<WebView> webView;
    if (SUCCEEDED(iWebView->QueryInterface(&webView)) && webView)
        page = webView->page();

    return page;
}

HRESULT WebView::defaultMinimumTimerInterval(double* interval)
{
    if (!interval)
        return E_POINTER;
    *interval = Settings::defaultMinDOMTimerInterval();
    return S_OK;
}

HRESULT WebView::setMinimumTimerInterval(double interval)
{
    page()->settings()->setMinDOMTimerInterval(interval);
    return S_OK;
}

HRESULT WebView::httpPipeliningEnabled(BOOL* enabled)
{
    if (!enabled)
        return E_POINTER;
#if USE(CFNETWORK) || USE(CURL)
    *enabled = WebCore::ResourceRequest::httpPipeliningEnabled();
#else
    *enabled = FALSE;
#endif
    return S_OK;
}

HRESULT WebView::setHTTPPipeliningEnabled(BOOL enabled)
{
#if USE(CFNETWORK) || USE(CURL)
    WebCore::ResourceRequest::setHTTPPipeliningEnabled(enabled);
#else
    if (enabled)
        return E_FAIL;
#endif
    return S_OK;
}

void WebView::setGlobalHistoryItem(HistoryItem* historyItem)
{
    m_globalHistoryItem = historyItem;
}

#if ENABLE(FULLSCREEN_API)
bool WebView::supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard) const
{
    if (withKeyboard)
        return false;

    BOOL enabled = FALSE;
    if (!m_preferences || FAILED(m_preferences->isFullScreenEnabled(&enabled)))
        return false;

    return enabled;
}

bool WebView::isFullScreen() const 
{
    return m_fullscreenController && m_fullscreenController->isFullScreen();
}

FullScreenController* WebView::fullScreenController()
{
    if (!m_fullscreenController)
        m_fullscreenController = adoptPtr(new FullScreenController(this));
    return m_fullscreenController.get();
}

void WebView::setFullScreenElement(PassRefPtr<Element> element)
{
    m_fullScreenElement = element;
}

HWND WebView::fullScreenClientWindow() const
{
    return m_viewWindow;
}

HWND WebView::fullScreenClientParentWindow() const
{
    return m_hostWindow;
}

void WebView::fullScreenClientSetParentWindow(HWND hostWindow)
{
    setHostWindow(reinterpret_cast<OLE_HANDLE>(hostWindow));
}

void WebView::fullScreenClientWillEnterFullScreen()
{
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document()->webkitWillEnterFullScreenForElement(m_fullScreenElement.get());
}

void WebView::fullScreenClientDidEnterFullScreen()
{
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document()->webkitDidEnterFullScreenForElement(m_fullScreenElement.get());
}

void WebView::fullScreenClientWillExitFullScreen()
{
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document()->webkitWillExitFullScreenForElement(m_fullScreenElement.get());
}

void WebView::fullScreenClientDidExitFullScreen()
{
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document()->webkitDidExitFullScreenForElement(m_fullScreenElement.get());
    m_fullScreenElement = nullptr;
}

void WebView::fullScreenClientForceRepaint()
{
    ASSERT(m_fullScreenElement);
    RECT windowRect = {0};
    frameRect(&windowRect);
    repaint(windowRect, true /*contentChanged*/, true /*immediate*/, false /*contentOnly*/);
    m_fullscreenController->repaintCompleted();
}

#endif
// Used by TextInputController in DumpRenderTree

HRESULT STDMETHODCALLTYPE WebView::setCompositionForTesting(
    /* [in] */ BSTR composition, 
    /* [in] */ UINT from, 
    /* [in] */ UINT length)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return E_FAIL;

    String compositionStr = toString(composition);

    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, compositionStr.length(), Color(Color::black), false));
    frame->editor().setComposition(compositionStr, underlines, from, from + length);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::hasCompositionForTesting(/* [out, retval] */ BOOL* result)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
     if (!frame)
        return E_FAIL;

    *result = frame && frame->editor().hasComposition();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::confirmCompositionForTesting(/* [in] */ BSTR composition)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return E_FAIL;

    String compositionStr = toString(composition);

    if (compositionStr.isNull())
        frame->editor().confirmComposition();

    frame->editor().confirmComposition(compositionStr);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::compositionRangeForTesting(/* [out] */ UINT* startPosition, /* [out] */ UINT* length)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return E_FAIL;

    RefPtr<Range> range = frame->editor().compositionRange();

    if (!range)
        return E_FAIL;

    *startPosition = range->startOffset();
    *length = range->startOffset() + range->endOffset();

    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebView::firstRectForCharacterRangeForTesting(
    /* [in] */ UINT location, 
    /* [in] */ UINT length, 
    /* [out, retval] */ RECT* resultRect)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return E_FAIL;

    IntRect resultIntRect;
    resultIntRect.setLocation(IntPoint(0, 0));
    resultIntRect.setSize(IntSize(0, 0));
    
    if (location > INT_MAX)
        return E_FAIL;
    if (length > INT_MAX || location + length > INT_MAX)
        length = INT_MAX - location;
        
    RefPtr<Range> range = TextIterator::rangeFromLocationAndLength(frame->selection()->rootEditableElementOrDocumentElement(), location, length);

    if (!range)
        return E_FAIL;
    
    ASSERT(range->startContainer());
    ASSERT(range->endContainer());
     
    IntRect rect = frame->editor().firstRectForRange(range.get());
    resultIntRect = frame->view()->contentsToWindow(rect);

    resultRect->left = resultIntRect.x();
    resultRect->top = resultIntRect.y();
    resultRect->right = resultIntRect.x() + resultIntRect.width();
    resultRect->bottom = resultIntRect.y() + resultIntRect.height();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebView::selectedRangeForTesting(/* [out] */ UINT* location, /* [out] */ UINT* length)
{
    if (!m_page)
        return E_FAIL;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return E_FAIL;

    RefPtr<Range> range = frame->editor().selectedRange();

    size_t locationSize;
    size_t lengthSize;
    if (range && TextIterator::getLocationAndLengthFromRange(frame->selection()->rootEditableElementOrDocumentElement(), range.get(), locationSize, lengthSize)) {
        *location = static_cast<UINT>(locationSize);
        *length = static_cast<UINT>(lengthSize);
    }

    return S_OK;
}

#if PLATFORM(HP)
void WebView::OutputLiveWebViewInfo(HPMemoryOutputFunc output)
{
    static const int buff_size = 2048;
    char buff[buff_size];
    char buff2[512];
    size_t l = 0;

    HashSet<WebView*>::iterator end = allWebViewsSet().end();
    for (HashSet<WebView*>::iterator it = allWebViewsSet().begin(); it != end; ++it) {
        snprintf(buff, buff_size, "WebView at 0x%p details:\n", *it);
        output(buff);
        if ((*it)->didClose()){
            snprintf(buff, buff_size, "  URL: ??? (WebView->didClose()=true)\n");
        }
        else{
            BSTR mainFrameURL;
            (*it)->mainFrameURL(&mainFrameURL);
            if (mainFrameURL){
                for (l = 0; mainFrameURL[l] != 0 && l < 511; l++)
                    buff2[l] = mainFrameURL[l];
                SysFreeString(mainFrameURL);
            }
            buff2[l] = '\0';
            snprintf(buff, buff_size, "  URL: %s\n", buff2);
        }
    }
}
#endif
