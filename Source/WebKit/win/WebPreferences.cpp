/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Hewlett-Packard Development Company.
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
#include "WebKit.h"
#include "WebKitDLL.h"
#include "WebPreferences.h"

#include "WebNotificationCenter.h"
#include "WebPreferenceKeysPrivate.h"

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#include <WebCore/CACFLayerTreeHost.h>
#endif
#include <WebCore/COMPtr.h>
#include <WebCore/FileSystem.h>
#include <WebCore/Font.h>
#include <WebCore/LocalizedStrings.h>
#include <limits>
#include <shlobj.h>
#include <wchar.h>
#include <wtf/HashMap.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if USE(CG)
#include <CoreGraphics/CoreGraphics.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

#if USE(CF)
#define STRING(x) CFSTR(x)
#else
#define STRING(x) x
#endif

using namespace WebCore;
using std::numeric_limits;

#if USE(CF)
static const String& oldPreferencesPath()
{
    static String path = pathByAppendingComponent(roamingUserSpecificStorageDirectory(), "WebKitPreferences.plist");
    return path;
}

template<typename NumberType> struct CFNumberTraits { static const unsigned Type; };
template<> struct CFNumberTraits<int> { static const unsigned Type = kCFNumberSInt32Type; };
template<> struct CFNumberTraits<LONGLONG> { static const unsigned Type = kCFNumberLongLongType; };
template<> struct CFNumberTraits<float> { static const unsigned Type = kCFNumberFloat32Type; };

template<typename NumberType>
static NumberType numberValueForPreferencesValue(CFPropertyListRef value)
{
    if (!value)
        return 0;

    CFTypeID cfType = CFGetTypeID(value);
    if (cfType == CFStringGetTypeID())
        return static_cast<NumberType>(CFStringGetIntValue(static_cast<CFStringRef>(value)));
    else if (cfType == CFBooleanGetTypeID()) {
        Boolean boolVal = CFBooleanGetValue(static_cast<CFBooleanRef>(value));
        return boolVal ? 1 : 0;
    } else if (cfType == CFNumberGetTypeID()) {
        NumberType val = 0;
        CFNumberGetValue(static_cast<CFNumberRef>(value), CFNumberTraits<NumberType>::Type, &val);
        return val;
    }

    return 0;
}

template<typename NumberType>
static RetainPtr<CFNumberRef> cfNumber(NumberType value)
{
    return adoptCF(CFNumberCreate(0, CFNumberTraits<NumberType>::Type, &value));
}

static bool booleanValueForPreferencesValue(CFPropertyListRef value)
{
    return numberValueForPreferencesValue<int>(value);
}
#endif

// WebPreferences ----------------------------------------------------------------


#if USE(CF)
static CFDictionaryRef defaultSettings;
#else
static HashMap<WTF::String, WTF::String> *defaultSettings = NULL;
#endif
static HashMap<WTF::String, COMPtr<WebPreferences> > *webPreferencesInstances = NULL;


WebPreferences* WebPreferences::sharedStandardPreferences()
{
    static WebPreferences* standardPreferences;
    if (!standardPreferences) {
        standardPreferences = WebPreferences::createInstance();
#if USE(CF)
        standardPreferences->setAutosaves(TRUE);
        standardPreferences->load();
#else
        standardPreferences->initializeDefaultSettings();
#endif
    }

    return standardPreferences;
}

WebPreferences::WebPreferences()
    : m_refCount(0)
#if USE(CF)
    , m_autoSaves(0)
#endif
    , m_automaticallyDetectsCacheModel(true)
    , m_numWebViews(0)
{
    gClassCount++;
    gClassNameCount.add("WebPreferences");
    if (webPreferencesInstances == NULL)
        webPreferencesInstances = new HashMap<WTF::String, COMPtr<WebPreferences>>();
}

WebPreferences::~WebPreferences()
{
    gClassCount--;
    gClassNameCount.remove("WebPreferences");
}

WebPreferences* WebPreferences::createInstance()
{
    WebPreferences* instance = new WebPreferences();
    instance->AddRef();
    return instance;
}

HRESULT WebPreferences::postPreferencesChangesNotification()
{
    IWebNotificationCenter* nc = WebNotificationCenter::defaultCenterInternal();
    HRESULT hr = nc->postNotificationName(webPreferencesChangedNotification(), static_cast<IWebPreferences*>(this), 0);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

WebPreferences* WebPreferences::getInstanceForIdentifier(BSTR identifier)
{
    if (!identifier)
        return sharedStandardPreferences();

    WTF::String identifierString(identifier, SysStringLen(identifier));
    return webPreferencesInstances->get(identifierString).get();

}

void WebPreferences::setInstance(WebPreferences* instance, BSTR identifier)
{
    if (!identifier || !instance)
        return;
    WTF::String identifierString(identifier, SysStringLen(identifier));
    webPreferencesInstances->add(identifierString, instance);

}

void WebPreferences::removeReferenceForIdentifier(BSTR identifier)
{
    if (!identifier || webPreferencesInstances->isEmpty())
        return;

    WTF::String identifierString(identifier, SysStringLen(identifier));
    WebPreferences* webPreference = webPreferencesInstances->get(identifierString).get();
    if (webPreference && webPreference->m_refCount == 1)
        webPreferencesInstances->remove(identifierString);
}

void WebPreferences::initializeDefaultSettings()
{
    if (defaultSettings)
        return;

#if USE(CF)
    CFMutableDictionaryRef defaults = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFBoolean TRUE_VALUE = kCFBooleanTrue;
    CFBoolean FALSE_VALUE = kCFBooleanFalse;
#define SET_DEFAULT(x, y)  CFDictionaryAddValue(defaults, STRING(x), y)
#else
    defaultSettings = new HashMap<WTF::String, WTF::String>();
    WTF::String TRUE_VALUE("true");
    WTF::String FALSE_VALUE("false");
#define SET_DEFAULT(x, y)  defaultSettings->add(x, y)
#endif

    SET_DEFAULT(WebKitStandardFontPreferenceKey, "Times New Roman");
    SET_DEFAULT(WebKitFixedFontPreferenceKey, "Courier New");
    SET_DEFAULT(WebKitSerifFontPreferenceKey, "Times New Roman");
    SET_DEFAULT(WebKitSansSerifFontPreferenceKey, "Arial");
    SET_DEFAULT(WebKitCursiveFontPreferenceKey, "Comic Sans MS");
    SET_DEFAULT(WebKitFantasyFontPreferenceKey, "Comic Sans MS");
    SET_DEFAULT(WebKitPictographFontPreferenceKey, "Times New Roman");
    SET_DEFAULT(WebKitMinimumFontSizePreferenceKey, "0");
    SET_DEFAULT(WebKitMinimumLogicalFontSizePreferenceKey, "9");
    SET_DEFAULT(WebKitDefaultFontSizePreferenceKey, "16");
    SET_DEFAULT(WebKitDefaultFixedFontSizePreferenceKey, "13");

    String defaultDefaultEncoding(WEB_UI_STRING("ISO-8859-1", "The default, default character encoding on Windows"));
#if USE(CF)
    SET_DEFAULT(WebKitDefaultTextEncodingNamePreferenceKey, defaultDefaultEncoding.createCFString().get());
#else
    SET_DEFAULT(WebKitDefaultTextEncodingNamePreferenceKey, defaultDefaultEncoding);
#endif

    SET_DEFAULT(WebKitUserStyleSheetEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitUserStyleSheetLocationPreferenceKey, "");
    SET_DEFAULT(WebKitShouldPrintBackgroundsPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitTextAreasAreResizablePreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitJavaEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitJavaScriptEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitWebSecurityEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitAllowUniversalAccessFromFileURLsPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitAllowFileAccessFromFileURLsPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitJavaScriptCanAccessClipboardPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitXSSAuditorEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitFrameFlatteningEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitPluginsEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitCSSRegionsEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitDatabasesEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitLocalStorageEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitExperimentalNotificationsEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitZoomsTextOnlyPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitAllowAnimatedImagesPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitAllowAnimatedImageLoopingPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitDisplayImagesKey, TRUE_VALUE);
    SET_DEFAULT(WebKitLoadSiteIconsKey, FALSE_VALUE);
    SET_DEFAULT(WebKitBackForwardCacheExpirationIntervalKey, "1800");
    SET_DEFAULT(WebKitTabToLinksPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitPrivateBrowsingEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitRespectStandardStyleKeyEquivalentsPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitShowsURLsInToolTipsPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitShowsToolTipOverTruncatedTextPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitPDFDisplayModePreferenceKey, "1");
    SET_DEFAULT(WebKitPDFScaleFactorPreferenceKey, "0");
    SET_DEFAULT(WebKitShouldDisplaySubtitlesPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitShouldDisplayCaptionsPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitShouldDisplayTextDescriptionsPreferenceKey, FALSE_VALUE);

#if USE(CF)
    RetainPtr<CFStringRef> linkBehaviorStringRef = adoptCF(CFStringCreateWithFormat(0, 0, CFSTR("%d"), WebKitEditableLinkDefaultBehavior));
    SET_DEFAULT(WebKitEditableLinkBehaviorPreferenceKey, linkBehaviorStringRef.get());
#else
    SET_DEFAULT(WebKitEditableLinkBehaviorPreferenceKey, String::format("%d", WebKitEditableLinkDefaultBehavior));
#endif

    SET_DEFAULT(WebKitHistoryItemLimitKey, "1000");
    SET_DEFAULT(WebKitHistoryAgeInDaysLimitKey, "7");
    SET_DEFAULT(WebKitIconDatabaseLocationKey, "");
    SET_DEFAULT(WebKitIconDatabaseEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitFontSmoothingTypePreferenceKey, "2");
    SET_DEFAULT(WebKitFontSmoothingContrastPreferenceKey, "2");
    SET_DEFAULT(WebKitCookieStorageAcceptPolicyPreferenceKey, "2");
    SET_DEFAULT(WebContinuousSpellCheckingEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebGrammarCheckingEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(AllowContinuousSpellCheckingPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitUsesPageCachePreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitLocalStorageDatabasePathPreferenceKey, "");

#if USE(CF)
    RetainPtr<CFStringRef> cacheModelRef = adoptCF(CFStringCreateWithFormat(0, 0, CFSTR("%d"), WebCacheModelDocumentViewer));
    SET_DEFAULT(WebKitCacheModelPreferenceKey, cacheModelRef.get());
#else
    SET_DEFAULT(WebKitCacheModelPreferenceKey, String::format("%d", WebCacheModelDocumentViewer));
    SET_DEFAULT(WebKitInspectorURLPreferenceKey, "");
#endif

    SET_DEFAULT(WebKitAuthorAndUserStylesEnabledPreferenceKey, TRUE_VALUE);
    SET_DEFAULT(WebKitApplicationChromeModePreferenceKey, FALSE_VALUE);

    SET_DEFAULT(WebKitOfflineWebApplicationCacheEnabledPreferenceKey, FALSE_VALUE);

    SET_DEFAULT(WebKitPaintNativeControlsPreferenceKey, TRUE_VALUE);

    SET_DEFAULT(WebKitUseHighResolutionTimersPreferenceKey, TRUE_VALUE);

    SET_DEFAULT(WebKitAcceleratedCompositingEnabledPreferenceKey, FALSE_VALUE);
    
    SET_DEFAULT(WebKitShowDebugBordersPreferenceKey, FALSE_VALUE);

    SET_DEFAULT(WebKitDNSPrefetchingEnabledPreferenceKey, FALSE_VALUE);

    SET_DEFAULT(WebKitMemoryInfoEnabledPreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitHyperlinkAuditingEnabledPreferenceKey, TRUE_VALUE);

    SET_DEFAULT(WebKitMediaPlaybackRequiresUserGesturePreferenceKey, FALSE_VALUE);
    SET_DEFAULT(WebKitMediaPlaybackAllowsInlinePreferenceKey, TRUE_VALUE);

    SET_DEFAULT(WebKitRequestAnimationFrameEnabledPreferenceKey, TRUE_VALUE);

    SET_DEFAULT(WebKitEmulateTouchEventsKey, FALSE_VALUE);

#if USE(CF)
    defaultSettings = defaults;
#endif
}

#if USE(CF)
RetainPtr<CFPropertyListRef> WebPreferences::valueForKey(CFStringRef key)
{
    RetainPtr<CFPropertyListRef> value = CFDictionaryGetValue(m_privatePrefs.get(), key);
    if (value)
        return value;

    value = adoptCF(CFPreferencesCopyAppValue(key, kCFPreferencesCurrentApplication));
    if (value)
        return value;

    return CFDictionaryGetValue(defaultSettings, key);
}

void WebPreferences::setValueForKey(CFStringRef key, CFPropertyListRef value)
{
    CFDictionarySetValue(m_privatePrefs.get(), key, value);
    if (m_autoSaves) {
        CFPreferencesSetAppValue(key, value, kCFPreferencesCurrentApplication);
        save();
    }
}

BSTR WebPreferences::stringValueForKey(CFStringRef key)
{
    RetainPtr<CFPropertyListRef> value = valueForKey(key);
    
    if (!value || (CFGetTypeID(value.get()) != CFStringGetTypeID()))
        return 0;

    CFStringRef str = static_cast<CFStringRef>(value.get());

    CFIndex length = CFStringGetLength(str);
    const UniChar* uniChars = CFStringGetCharactersPtr(str);
    if (uniChars)
        return SysAllocStringLen((LPCTSTR)uniChars, length);

    BSTR bstr = SysAllocStringLen(0, length);
    if (!bstr)
        return 0;

    if (!CFStringGetCString(str, (char*)bstr, (length+1)*sizeof(WCHAR), kCFStringEncodingUTF16)) {
        SysFreeString(bstr);
        return 0;
    }
        
    bstr[length] = 0;
    return bstr;
}

int WebPreferences::integerValueForKey(CFStringRef key)
{
    return numberValueForPreferencesValue<int>(valueForKey(key).get());
}

BOOL WebPreferences::boolValueForKey(CFStringRef key)
{
    return booleanValueForPreferencesValue(valueForKey(key).get());
}

float WebPreferences::floatValueForKey(CFStringRef key)
{
    return numberValueForPreferencesValue<float>(valueForKey(key).get());
}

LONGLONG WebPreferences::longlongValueForKey(CFStringRef key)
{
    return numberValueForPreferencesValue<LONGLONG>(valueForKey(key).get());
}

void WebPreferences::setStringValue(CFStringRef key, LPCTSTR value)
{
    BString val;
    val.adoptBSTR(stringValueForKey(key));
    if (val && !wcscmp(val, value))
        return;
    
    RetainPtr<CFStringRef> valueRef = adoptCF(CFStringCreateWithCharactersNoCopy(0, (UniChar*)_wcsdup(value), (CFIndex)wcslen(value), kCFAllocatorMalloc));
    setValueForKey(key, valueRef.get());

    postPreferencesChangesNotification();
}

void WebPreferences::setIntegerValue(CFStringRef key, int value)
{
    if (integerValueForKey(key) == value)
        return;

    setValueForKey(key, cfNumber(value).get());

    postPreferencesChangesNotification();
}

void WebPreferences::setFloatValue(CFStringRef key, float value)
{
    if (floatValueForKey(key) == value)
        return;

    setValueForKey(key, cfNumber(value).get());

    postPreferencesChangesNotification();
}

void WebPreferences::setBoolValue(CFStringRef key, BOOL value)
{
    if (boolValueForKey(key) == value)
        return;

    setValueForKey(key, value ? kCFBooleanTrue : kCFBooleanFalse);

    postPreferencesChangesNotification();
}

void WebPreferences::setLongLongValue(CFStringRef key, LONGLONG value)
{
    if (longlongValueForKey(key) == value)
        return;

    setValueForKey(key, cfNumber(value).get());

    postPreferencesChangesNotification();
}

#else
WTF::String WebPreferences::valueForKey(WTF::String key)
{
    HashMap<WTF::String, WTF::String>::iterator it = m_privatePrefs.find(key);
    if (it != m_privatePrefs.end())
        return it->value;

    return defaultSettings->get(key);
}

void WebPreferences::setValueForKey(WTF::String key, WTF::String value)
{
    m_privatePrefs.set(key, value);
}

BSTR WebPreferences::stringValueForKey(WTF::String key)
{
    WTF::String value = valueForKey(key);

    if (value.isEmpty())
        return 0;

    return BString(value).release();
}

int WebPreferences::integerValueForKey(WTF::String key)
{
    return valueForKey(key).toInt();
}

BOOL WebPreferences::boolValueForKey(WTF::String key)
{
    if (valueForKey(key) == "true")
        return TRUE;
    else
        return FALSE;
}

float WebPreferences::floatValueForKey(WTF::String key)
{
    return valueForKey(key).toFloat();
}

LONGLONG WebPreferences::longlongValueForKey(WTF::String key)
{
    return valueForKey(key).toInt64();
}

void WebPreferences::setStringValue(WTF::String key, WTF::String value)
{
    if (valueForKey(key) == value)
        return;

    setValueForKey(key, value);
    postPreferencesChangesNotification();
}

void WebPreferences::setIntegerValue(WTF::String key, int value)
{
    if (integerValueForKey(key) == value)
        return;

    setValueForKey(key, WTF::String::number(value));

    postPreferencesChangesNotification();
}

void WebPreferences::setFloatValue(WTF::String key, float value)
{
    if (floatValueForKey(key) == value)
        return;

    setValueForKey(key, WTF::String::number(value));

    postPreferencesChangesNotification();
}

void WebPreferences::setBoolValue(WTF::String key, BOOL value)
{
    if (boolValueForKey(key) == value)
        return;

    setValueForKey(key, WTF::String(value ? "true" : "false"));

    postPreferencesChangesNotification();
}

void WebPreferences::setLongLongValue(WTF::String key, LONGLONG value)
{
    if (longlongValueForKey(key) == value)
        return;

    setValueForKey(key, WTF::String::number(value));

    postPreferencesChangesNotification();
}
#endif

BSTR WebPreferences::webPreferencesChangedNotification()
{
    static BSTR webPreferencesChangedNotification = SysAllocString(WebPreferencesChangedNotification);
    return webPreferencesChangedNotification;
}

BSTR WebPreferences::webPreferencesRemovedNotification()
{
    static BSTR webPreferencesRemovedNotification = SysAllocString(WebPreferencesRemovedNotification);
    return webPreferencesRemovedNotification;
}

#if USE(CF)
void WebPreferences::save()
{
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

void WebPreferences::load()
{
    initializeDefaultSettings();

    m_privatePrefs = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    migrateWebKitPreferencesToCFPreferences();
}

void WebPreferences::migrateWebKitPreferencesToCFPreferences()
{
    CFStringRef didMigrateKey = CFSTR(WebKitDidMigrateWebKitPreferencesToCFPreferencesPreferenceKey);
    if (boolValueForKey(didMigrateKey))
        return;
    bool oldValue = m_autoSaves;
    m_autoSaves = true;
    setBoolValue(didMigrateKey, TRUE);
    m_autoSaves = oldValue;

    WTF::CString path = oldPreferencesPath().utf8();

    RetainPtr<CFURLRef> urlRef = adoptCF(CFURLCreateFromFileSystemRepresentation(0, reinterpret_cast<const UInt8*>(path.data()), path.length(), false));
    if (!urlRef)
        return;

    RetainPtr<CFReadStreamRef> stream = adoptCF(CFReadStreamCreateWithFile(0, urlRef.get()));
    if (!stream)
        return;

    if (!CFReadStreamOpen(stream.get()))
        return;

    CFPropertyListFormat format = kCFPropertyListBinaryFormat_v1_0 | kCFPropertyListXMLFormat_v1_0;
    RetainPtr<CFPropertyListRef> plist = adoptCF(CFPropertyListCreateFromStream(0, stream.get(), 0, kCFPropertyListMutableContainersAndLeaves, &format, 0));
    CFReadStreamClose(stream.get());

    if (!plist || CFGetTypeID(plist.get()) != CFDictionaryGetTypeID())
        return;

    copyWebKitPreferencesToCFPreferences(static_cast<CFDictionaryRef>(plist.get()));

    deleteFile(oldPreferencesPath());
}

void WebPreferences::copyWebKitPreferencesToCFPreferences(CFDictionaryRef dict)
{
    ASSERT_ARG(dict, dict);

    int count = CFDictionaryGetCount(dict);
    if (count <= 0)
        return;

    CFStringRef didRemoveDefaultsKey = CFSTR(WebKitDidMigrateDefaultSettingsFromSafari3BetaPreferenceKey);
    bool omitDefaults = !booleanValueForPreferencesValue(CFDictionaryGetValue(dict, didRemoveDefaultsKey));

    OwnArrayPtr<CFTypeRef> keys = adoptArrayPtr(new CFTypeRef[count]);
    OwnArrayPtr<CFTypeRef> values = adoptArrayPtr(new CFTypeRef[count]);
    CFDictionaryGetKeysAndValues(dict, keys.get(), values.get());

    for (int i = 0; i < count; ++i) {
        if (!keys[i] || !values[i] || CFGetTypeID(keys[i]) != CFStringGetTypeID())
            continue;

        if (omitDefaults) {
            CFTypeRef defaultValue = CFDictionaryGetValue(defaultSettings, keys[i]);
            if (defaultValue && CFEqual(defaultValue, values[i]))
                continue;
        }

        setValueForKey(static_cast<CFStringRef>(keys[i]), values[i]);
    }
}
#endif

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebPreferences::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebPreferences*>(this);
    else if (IsEqualGUID(riid, IID_IWebPreferences))
        *ppvObject = static_cast<IWebPreferences*>(this);
    else if (IsEqualGUID(riid, IID_IWebPreferencesPrivate))
        *ppvObject = static_cast<IWebPreferencesPrivate*>(this);
    else if (IsEqualGUID(riid, CLSID_WebPreferences))
        *ppvObject = this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebPreferences::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebPreferences::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebPreferences ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebPreferences::standardPreferences( 
    /* [retval][out] */ IWebPreferences** standardPreferences)
{
    if (!standardPreferences)
        return E_POINTER;
    *standardPreferences = sharedStandardPreferences();
    (*standardPreferences)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::initWithIdentifier( 
        /* [in] */ BSTR anIdentifier,
        /* [retval][out] */ IWebPreferences** preferences)
{
    WebPreferences *instance = getInstanceForIdentifier(anIdentifier);
    if (instance) {
        *preferences = instance;
        instance->AddRef();
        return S_OK;
    }

#if USE(CF)
    load();
#else
    initializeDefaultSettings();
#endif

    *preferences = this;
    AddRef();

    if (anIdentifier) {
        m_identifier = anIdentifier;
        setInstance(this, m_identifier);
    }

    this->postPreferencesChangesNotification();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::identifier( 
    /* [retval][out] */ BSTR* ident)
{
    if (!ident)
        return E_POINTER;
    *ident = m_identifier ? SysAllocString(m_identifier) : 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::standardFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitStandardFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setStandardFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitStandardFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::fixedFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitFixedFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setFixedFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitFixedFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::serifFontFamily( 
    /* [retval][out] */ BSTR* fontFamily)
{
    *fontFamily = stringValueForKey(STRING(WebKitSerifFontPreferenceKey));
    return (*fontFamily) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setSerifFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitSerifFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::sansSerifFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitSansSerifFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setSansSerifFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitSansSerifFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::cursiveFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitCursiveFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setCursiveFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitCursiveFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::fantasyFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitFantasyFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setFantasyFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitFantasyFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::pictographFontFamily( 
    /* [retval][out] */ BSTR* family)
{
    *family = stringValueForKey(STRING(WebKitPictographFontPreferenceKey));
    return (*family) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setPictographFontFamily( 
    /* [in] */ BSTR family)
{
    setStringValue(STRING(WebKitPictographFontPreferenceKey), family);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::defaultFontSize( 
    /* [retval][out] */ int* fontSize)
{
    *fontSize = integerValueForKey(STRING(WebKitDefaultFontSizePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setDefaultFontSize( 
    /* [in] */ int fontSize)
{
    setIntegerValue(STRING(WebKitDefaultFontSizePreferenceKey), fontSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::defaultFixedFontSize( 
    /* [retval][out] */ int* fontSize)
{
    *fontSize = integerValueForKey(STRING(WebKitDefaultFixedFontSizePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setDefaultFixedFontSize( 
    /* [in] */ int fontSize)
{
    setIntegerValue(STRING(WebKitDefaultFixedFontSizePreferenceKey), fontSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::minimumFontSize( 
    /* [retval][out] */ int* fontSize)
{
    *fontSize = integerValueForKey(STRING(WebKitMinimumFontSizePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setMinimumFontSize( 
    /* [in] */ int fontSize)
{
    setIntegerValue(STRING(WebKitMinimumFontSizePreferenceKey), fontSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::minimumLogicalFontSize( 
    /* [retval][out] */ int* fontSize)
{
    *fontSize = integerValueForKey(STRING(WebKitMinimumLogicalFontSizePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setMinimumLogicalFontSize( 
    /* [in] */ int fontSize)
{
    setIntegerValue(STRING(WebKitMinimumLogicalFontSizePreferenceKey), fontSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::defaultTextEncodingName( 
    /* [retval][out] */ BSTR* name)
{
    *name = stringValueForKey(STRING(WebKitDefaultTextEncodingNamePreferenceKey));
    return (*name) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setDefaultTextEncodingName( 
    /* [in] */ BSTR name)
{
    setStringValue(STRING(WebKitDefaultTextEncodingNamePreferenceKey), name);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::userStyleSheetEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitUserStyleSheetEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setUserStyleSheetEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitUserStyleSheetEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::userStyleSheetLocation( 
    /* [retval][out] */ BSTR* location)
{
    *location = stringValueForKey(STRING(WebKitUserStyleSheetLocationPreferenceKey));
    return (*location) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setUserStyleSheetLocation( 
    /* [in] */ BSTR location)
{
    setStringValue(STRING(WebKitUserStyleSheetLocationPreferenceKey), location);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isJavaEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitJavaEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setJavaEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitJavaEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isJavaScriptEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitJavaScriptEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setJavaScriptEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitJavaScriptEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isWebSecurityEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitWebSecurityEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setWebSecurityEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitWebSecurityEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::allowUniversalAccessFromFileURLs(
    /* [retval][out] */ BOOL* allowAccess)
{
    *allowAccess = boolValueForKey(STRING(WebKitAllowUniversalAccessFromFileURLsPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAllowUniversalAccessFromFileURLs(
    /* [in] */ BOOL allowAccess)
{
    setBoolValue(STRING(WebKitAllowUniversalAccessFromFileURLsPreferenceKey), allowAccess);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::allowFileAccessFromFileURLs(
    /* [retval][out] */ BOOL* allowAccess)
{
    *allowAccess = boolValueForKey(STRING(WebKitAllowFileAccessFromFileURLsPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAllowFileAccessFromFileURLs(
    /* [in] */ BOOL allowAccess)
{
    setBoolValue(STRING(WebKitAllowFileAccessFromFileURLsPreferenceKey), allowAccess);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::javaScriptCanAccessClipboard(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitJavaScriptCanAccessClipboardPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setJavaScriptCanAccessClipboard(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitJavaScriptCanAccessClipboardPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isXSSAuditorEnabled(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitXSSAuditorEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setXSSAuditorEnabled(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitXSSAuditorEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isFrameFlatteningEnabled(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitFrameFlatteningEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setFrameFlatteningEnabled(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitFrameFlatteningEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::javaScriptCanOpenWindowsAutomatically( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setJavaScriptCanOpenWindowsAutomatically( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitJavaScriptCanOpenWindowsAutomaticallyPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::arePlugInsEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitPluginsEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setPlugInsEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitPluginsEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::isCSSRegionsEnabled(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitCSSRegionsEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setCSSRegionsEnabled(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitCSSRegionsEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::allowsAnimatedImages( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitAllowAnimatedImagesPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAllowsAnimatedImages( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitAllowAnimatedImagesPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::allowAnimatedImageLooping( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitAllowAnimatedImageLoopingPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAllowAnimatedImageLooping( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitAllowAnimatedImageLoopingPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setLoadsImagesAutomatically( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitDisplayImagesKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::loadsImagesAutomatically( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitDisplayImagesKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setLoadsSiteIconsIgnoringImageLoadingPreference(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitLoadSiteIconsKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::loadsSiteIconsIgnoringImageLoadingPreference(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitLoadSiteIconsKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setHixie76WebSocketProtocolEnabled(
    /* [in] */ BOOL enabled)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::hixie76WebSocketProtocolEnabled(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = false;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setMediaPlaybackRequiresUserGesture(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitMediaPlaybackRequiresUserGesturePreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::mediaPlaybackRequiresUserGesture(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitMediaPlaybackRequiresUserGesturePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setMediaPlaybackAllowsInline(
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitMediaPlaybackAllowsInlinePreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::mediaPlaybackAllowsInline(
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitMediaPlaybackAllowsInlinePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAutosaves( 
    /* [in] */ BOOL enabled)
{
#if USE(CF)
    m_autoSaves = !!enabled;
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE WebPreferences::autosaves( 
    /* [retval][out] */ BOOL* enabled)
{
#if USE(CF)
    *enabled = m_autoSaves ? TRUE : FALSE;
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE WebPreferences::setShouldPrintBackgrounds( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitShouldPrintBackgroundsPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::shouldPrintBackgrounds( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitShouldPrintBackgroundsPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setPrivateBrowsingEnabled( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitPrivateBrowsingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::privateBrowsingEnabled( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitPrivateBrowsingEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setTabsToLinks( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitTabToLinksPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::tabsToLinks( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitTabToLinksPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setUsesPageCache( 
        /* [in] */ BOOL usesPageCache)
{
    setBoolValue(STRING(WebKitUsesPageCachePreferenceKey), usesPageCache);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::usesPageCache( 
    /* [retval][out] */ BOOL* usesPageCache)
{
    *usesPageCache = boolValueForKey(STRING(WebKitUsesPageCachePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::textAreasAreResizable( 
    /* [retval][out] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitTextAreasAreResizablePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setTextAreasAreResizable( 
    /* [in] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitTextAreasAreResizablePreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::historyItemLimit(int* limit)
{
    *limit = integerValueForKey(STRING(WebKitHistoryItemLimitKey));
    return S_OK;
}

HRESULT WebPreferences::setHistoryItemLimit(int limit)
{
    setIntegerValue(STRING(WebKitHistoryItemLimitKey), limit);
    return S_OK;
}

HRESULT WebPreferences::historyAgeInDaysLimit(int* limit)
{
    *limit = integerValueForKey(STRING(WebKitHistoryAgeInDaysLimitKey));
    return S_OK;
}

HRESULT WebPreferences::setHistoryAgeInDaysLimit(int limit)
{
    setIntegerValue(STRING(WebKitHistoryAgeInDaysLimitKey), limit);
    return S_OK;
}

HRESULT WebPreferences::unused1()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebPreferences::unused2()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebPreferences::iconDatabaseLocation(
    /* [out] */ BSTR* location)
{
    *location = stringValueForKey(STRING(WebKitIconDatabaseLocationKey));
    return (*location) ? S_OK : E_FAIL;
}

HRESULT WebPreferences::setIconDatabaseLocation(
    /* [in] */ BSTR location)
{
    setStringValue(STRING(WebKitIconDatabaseLocationKey), location);
    return S_OK;
}

HRESULT WebPreferences::iconDatabaseEnabled(BOOL* enabled)//location)
{
    *enabled = boolValueForKey(STRING(WebKitIconDatabaseEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setIconDatabaseEnabled(BOOL enabled )//location)
{
    setBoolValue(STRING(WebKitIconDatabaseEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::fontSmoothing( 
    /* [retval][out] */ FontSmoothingType* smoothingType)
{
    *smoothingType = (FontSmoothingType) integerValueForKey(STRING(WebKitFontSmoothingTypePreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setFontSmoothing( 
    /* [in] */ FontSmoothingType smoothingType)
{
    setIntegerValue(STRING(WebKitFontSmoothingTypePreferenceKey), smoothingType);
    if (smoothingType == FontSmoothingTypeWindows)
        smoothingType = FontSmoothingTypeMedium;
#if USE(CG)
    wkSetFontSmoothingLevel((int)smoothingType);
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::fontSmoothingContrast( 
    /* [retval][out] */ float* contrast)
{
    *contrast = floatValueForKey(STRING(WebKitFontSmoothingContrastPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setFontSmoothingContrast( 
    /* [in] */ float contrast)
{
    setFloatValue(STRING(WebKitFontSmoothingContrastPreferenceKey), contrast);
#if USE(CG)
    wkSetFontSmoothingContrast(contrast);
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::editableLinkBehavior(
    /* [out, retval] */ WebKitEditableLinkBehavior* editableLinkBehavior)
{
    WebKitEditableLinkBehavior value = (WebKitEditableLinkBehavior) integerValueForKey(STRING(WebKitEditableLinkBehaviorPreferenceKey));
    switch (value) {
        case WebKitEditableLinkDefaultBehavior:
        case WebKitEditableLinkAlwaysLive:
        case WebKitEditableLinkOnlyLiveWithShiftKey:
        case WebKitEditableLinkLiveWhenNotFocused:
        case WebKitEditableLinkNeverLive:
            *editableLinkBehavior = value;
            break;
        default: // ensure that a valid result is returned
            *editableLinkBehavior = WebKitEditableLinkDefaultBehavior;
            break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setEditableLinkBehavior(
    /* [in] */ WebKitEditableLinkBehavior behavior)
{
    setIntegerValue(STRING(WebKitEditableLinkBehaviorPreferenceKey), behavior);
    return S_OK;
}

HRESULT WebPreferences::unused5()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebPreferences::unused6()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::hyperlinkAuditingEnabled(
    /* [in] */ BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitHyperlinkAuditingEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setHyperlinkAuditingEnabled(
    /* [retval][out] */ BOOL enabled)
{
    setBoolValue(STRING(WebKitHyperlinkAuditingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::cookieStorageAcceptPolicy( 
        /* [retval][out] */ WebKitCookieStorageAcceptPolicy *acceptPolicy )
{
    if (!acceptPolicy)
        return E_POINTER;

    *acceptPolicy = (WebKitCookieStorageAcceptPolicy)integerValueForKey(STRING(WebKitCookieStorageAcceptPolicyPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setCookieStorageAcceptPolicy( 
        /* [in] */ WebKitCookieStorageAcceptPolicy acceptPolicy)
{
    setIntegerValue(STRING(WebKitCookieStorageAcceptPolicyPreferenceKey), acceptPolicy);
    return S_OK;
}


HRESULT WebPreferences::continuousSpellCheckingEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebContinuousSpellCheckingEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setContinuousSpellCheckingEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebContinuousSpellCheckingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::grammarCheckingEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebGrammarCheckingEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setGrammarCheckingEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebGrammarCheckingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::allowContinuousSpellChecking(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(AllowContinuousSpellCheckingPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setAllowContinuousSpellChecking(BOOL enabled)
{
    setBoolValue(STRING(AllowContinuousSpellCheckingPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::areSeamlessIFramesEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(SeamlessIFramesPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setSeamlessIFramesEnabled(BOOL enabled)
{
    setBoolValue(STRING(SeamlessIFramesPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::isDOMPasteAllowed(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitDOMPasteAllowedPreferenceKey));
    return S_OK;
}
    
HRESULT WebPreferences::setDOMPasteAllowed(BOOL enabled)
{
    setBoolValue(STRING(WebKitDOMPasteAllowedPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::cacheModel(WebCacheModel* cacheModel)
{
    if (!cacheModel)
        return E_POINTER;

    *cacheModel = (WebCacheModel)integerValueForKey(STRING(WebKitCacheModelPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setCacheModel(WebCacheModel cacheModel)
{
    setIntegerValue(STRING(WebKitCacheModelPreferenceKey), cacheModel);
    return S_OK;
}

HRESULT WebPreferences::unused3()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebPreferences::unused4()
{
    ASSERT_NOT_REACHED();
    return E_FAIL;
}

HRESULT WebPreferences::shouldPaintNativeControls(BOOL* shouldPaint)
{
    *shouldPaint = boolValueForKey(STRING(WebKitPaintNativeControlsPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setShouldPaintNativeControls(BOOL shouldPaint)
{
    setBoolValue(STRING(WebKitPaintNativeControlsPreferenceKey), shouldPaint);
    return S_OK;
}

HRESULT WebPreferences::setDeveloperExtrasEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitDeveloperExtrasEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::developerExtrasEnabled(BOOL* enabled)
{
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitDeveloperExtrasEnabledPreferenceKey));
    return S_OK;
}

bool WebPreferences::developerExtrasDisabledByOverride()
{
    return !!boolValueForKey(STRING(DisableWebKitDeveloperExtrasPreferenceKey));
}

HRESULT WebPreferences::setAutomaticallyDetectsCacheModel(BOOL automaticallyDetectsCacheModel)
{
    m_automaticallyDetectsCacheModel = !!automaticallyDetectsCacheModel;
    return S_OK;
}

HRESULT WebPreferences::automaticallyDetectsCacheModel(BOOL* automaticallyDetectsCacheModel)
{
    if (!automaticallyDetectsCacheModel)
        return E_POINTER;

    *automaticallyDetectsCacheModel = m_automaticallyDetectsCacheModel;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setAuthorAndUserStylesEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitAuthorAndUserStylesEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::authorAndUserStylesEnabled(BOOL* enabled)
{
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitAuthorAndUserStylesEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::inApplicationChromeMode(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitApplicationChromeModePreferenceKey));
    return S_OK;
}
    
HRESULT WebPreferences::setApplicationChromeMode(BOOL enabled)
{
    setBoolValue(STRING(WebKitApplicationChromeModePreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setOfflineWebApplicationCacheEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitOfflineWebApplicationCacheEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::offlineWebApplicationCacheEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitOfflineWebApplicationCacheEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setDatabasesEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitDatabasesEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::databasesEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitDatabasesEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setLocalStorageEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitLocalStorageEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::localStorageEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitLocalStorageEnabledPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::localStorageDatabasePath(BSTR* location)
{
    *location = stringValueForKey(STRING(WebKitLocalStorageDatabasePathPreferenceKey));
    return (*location) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setLocalStorageDatabasePath(BSTR location)
{
    setStringValue(STRING(WebKitLocalStorageDatabasePathPreferenceKey), location);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setExperimentalNotificationsEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitExperimentalNotificationsEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::experimentalNotificationsEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitExperimentalNotificationsEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setZoomsTextOnly(BOOL zoomsTextOnly)
{
    setBoolValue(STRING(WebKitZoomsTextOnlyPreferenceKey), zoomsTextOnly);
    return S_OK;
}

HRESULT WebPreferences::zoomsTextOnly(BOOL* zoomsTextOnly)
{
    *zoomsTextOnly = boolValueForKey(STRING(WebKitZoomsTextOnlyPreferenceKey));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::setShouldUseHighResolutionTimers(BOOL useHighResolutionTimers)
{
    setBoolValue(STRING(WebKitUseHighResolutionTimersPreferenceKey), useHighResolutionTimers);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebPreferences::shouldUseHighResolutionTimers(BOOL* useHighResolutionTimers)
{
    *useHighResolutionTimers = boolValueForKey(STRING(WebKitUseHighResolutionTimersPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setPreferenceForTest(BSTR key, BSTR value)
{
    if (!SysStringLen(key) || !SysStringLen(value))
        return E_FAIL;
#if USE(CF)
    RetainPtr<CFStringRef> keyString = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<UniChar*>(key), SysStringLen(key)));
    RetainPtr<CFStringRef> valueString = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<UniChar*>(value), SysStringLen(value)));
    setValueForKey(keyString.get(), valueString.get());
#else
    setValueForKey(key, value);
#endif
    postPreferencesChangesNotification();
    return S_OK;
}

HRESULT WebPreferences::setAcceleratedCompositingEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitAcceleratedCompositingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::acceleratedCompositingEnabled(BOOL* enabled)
{
#if USE(ACCELERATED_COMPOSITING)
    *enabled = CACFLayerTreeHost::acceleratedCompositingAvailable() && boolValueForKey(STRING(WebKitAcceleratedCompositingEnabledPreferenceKey));
#else
    *enabled = FALSE;
#endif
    return S_OK;
}

HRESULT WebPreferences::showDebugBorders(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitShowDebugBordersPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setShowDebugBorders(BOOL enabled)
{
    setBoolValue(STRING(WebKitShowDebugBordersPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::showRepaintCounter(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitShowRepaintCounterPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setShowRepaintCounter(BOOL enabled)
{
    setBoolValue(STRING(WebKitShowRepaintCounterPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::setCustomDragCursorsEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitCustomDragCursorsEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::customDragCursorsEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitCustomDragCursorsEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setDNSPrefetchingEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitDNSPrefetchingEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::isDNSPrefetchingEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitDNSPrefetchingEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::memoryInfoEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitMemoryInfoEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setMemoryInfoEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitMemoryInfoEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::isFullScreenEnabled(BOOL* enabled)
{
#if ENABLE(FULLSCREEN_API)
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitFullScreenEnabledPreferenceKey));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setFullScreenEnabled(BOOL enabled)
{
#if ENABLE(FULLSCREEN_API)
    setBoolValue(STRING(WebKitFullScreenEnabledPreferenceKey), enabled);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::avFoundationEnabled(BOOL* enabled)
{
#if USE(AVFOUNDATION)
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitAVFoundationEnabledPreferenceKey));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setAVFoundationEnabled(BOOL enabled)
{
#if USE(AVFOUNDATION)
    setBoolValue(STRING(WebKitAVFoundationEnabledPreferenceKey), enabled);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::showsToolTipOverTruncatedText(BOOL* showsToolTip)
{
    if (!showsToolTip)
        return E_POINTER;

    *showsToolTip = boolValueForKey(STRING(WebKitShowsToolTipOverTruncatedTextPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setShowsToolTipOverTruncatedText(BOOL showsToolTip)
{
    setBoolValue(STRING(WebKitShowsToolTipOverTruncatedTextPreferenceKey), showsToolTip);
    return S_OK;
}

HRESULT WebPreferences::shouldInvertColors(BOOL* shouldInvertColors)
{
    if (!shouldInvertColors)
        return E_POINTER;

    *shouldInvertColors = boolValueForKey(STRING(WebKitShouldInvertColorsPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::setShouldInvertColors(BOOL shouldInvertColors)
{
    setBoolValue(STRING(WebKitShouldInvertColorsPreferenceKey), shouldInvertColors);
    return S_OK;
}

void WebPreferences::willAddToWebView()
{
    ++m_numWebViews;
}

void WebPreferences::didRemoveFromWebView()
{
    ASSERT(m_numWebViews);
    if (--m_numWebViews == 0) {
        IWebNotificationCenter* nc = WebNotificationCenter::defaultCenterInternal();
        nc->postNotificationName(webPreferencesRemovedNotification(), static_cast<IWebPreferences*>(this), 0);
    }
}

HRESULT WebPreferences::shouldDisplaySubtitles(BOOL* enabled)
{
#if ENABLE(VIDEO_TRACK)
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitShouldDisplaySubtitlesPreferenceKey));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setShouldDisplaySubtitles(BOOL enabled)
{
#if ENABLE(VIDEO_TRACK)
    setBoolValue(STRING(WebKitShouldDisplaySubtitlesPreferenceKey), enabled);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::shouldDisplayCaptions(BOOL* enabled)
{
#if ENABLE(VIDEO_TRACK)
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitShouldDisplayCaptionsPreferenceKey));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setShouldDisplayCaptions(BOOL enabled)
{
#if ENABLE(VIDEO_TRACK)
    setBoolValue(STRING(WebKitShouldDisplayCaptionsPreferenceKey), enabled);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::shouldDisplayTextDescriptions(BOOL* enabled)
{
#if ENABLE(VIDEO_TRACK)
    if (!enabled)
        return E_POINTER;

    *enabled = boolValueForKey(STRING(WebKitShouldDisplayTextDescriptionsPreferenceKey));
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setShouldDisplayTextDescriptions(BOOL enabled)
{
#if ENABLE(VIDEO_TRACK)
    setBoolValue(STRING(WebKitShouldDisplayTextDescriptionsPreferenceKey), enabled);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT WebPreferences::setRequestAnimationFrameEnabled(BOOL enabled)
{
    setBoolValue(STRING(WebKitRequestAnimationFrameEnabledPreferenceKey), enabled);
    return S_OK;
}

HRESULT WebPreferences::requestAnimationFrameEnabled(BOOL* enabled)
{
    *enabled = boolValueForKey(STRING(WebKitRequestAnimationFrameEnabledPreferenceKey));
    return S_OK;
}

HRESULT WebPreferences::emulateTouchEvents(BOOL* emulateTouchEvents)
{
    *emulateTouchEvents = boolValueForKey(STRING(WebKitEmulateTouchEventsKey));
    return S_OK;
}

HRESULT WebPreferences::setEmulateTouchEvents(BOOL emulateTouchEvents)
{
    setBoolValue(STRING(WebKitEmulateTouchEventsKey), emulateTouchEvents);
    return S_OK;
}

HRESULT WebPreferences::inspectorURL(BSTR *url)
{
    *url = stringValueForKey(STRING(WebKitInspectorURLPreferenceKey));
    return (*url) ? S_OK : E_FAIL;
}

HRESULT WebPreferences::setInspectorURL(BSTR url)
{
    setStringValue(STRING(WebKitInspectorURLPreferenceKey), url);
    return S_OK;
}

HRESULT WebPreferences::inspectorServerAddress(BSTR *url)
{
    *url = stringValueForKey(STRING(WebKitInspectorServerAddressKey));
    return (*url) ? S_OK : E_FAIL;
}

HRESULT WebPreferences::setInspectorServerAddress(BSTR address)
{
    setStringValue(STRING(WebKitInspectorServerAddressKey), address);
    return S_OK;
}