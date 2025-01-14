/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AboutData.h"
#include "WebKitDLL.h"
#include "WebCoreStatistics.h"

#include "COMPropertyBag.h"
#include <JavaScriptCore/JSLock.h>
#include <WebCore/FontCache.h>
#include <WebCore/GlyphPageTreeNode.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/MemoryCache.h>

using namespace JSC;
using namespace WebCore;

extern int WebFrameCount;

// WebCoreStatistics ---------------------------------------------------------------------------

WebCoreStatistics::WebCoreStatistics()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebCoreStatistics");
}

WebCoreStatistics::~WebCoreStatistics()
{
    gClassCount--;
    gClassNameCount.remove("WebCoreStatistics");
}

WebCoreStatistics* WebCoreStatistics::createInstance()
{
    WebCoreStatistics* instance = new WebCoreStatistics();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCoreStatistics::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<WebCoreStatistics*>(this);
    else if (IsEqualGUID(riid, IID_IWebCoreStatistics))
        *ppvObject = static_cast<WebCoreStatistics*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebCoreStatistics::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebCoreStatistics::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebCoreStatistics ------------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCoreStatistics::javaScriptObjectsCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count)
        return E_POINTER;

    JSLockHolder lock(JSDOMWindow::commonVM());
    *count = (UINT)JSDOMWindow::commonVM()->heap.objectCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::javaScriptGlobalObjectsCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count)
        return E_POINTER;

    JSLockHolder lock(JSDOMWindow::commonVM());
    *count = (UINT)JSDOMWindow::commonVM()->heap.globalObjectCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::javaScriptProtectedObjectsCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count)
        return E_POINTER;

    JSLockHolder lock(JSDOMWindow::commonVM());
    *count = (UINT)JSDOMWindow::commonVM()->heap.protectedObjectCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::javaScriptProtectedGlobalObjectsCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count)
        return E_POINTER;

    JSLockHolder lock(JSDOMWindow::commonVM());
    *count = (UINT)JSDOMWindow::commonVM()->heap.protectedGlobalObjectCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::javaScriptProtectedObjectTypeCounts( 
#if OS(WINCE)
    /* [retval][out] */ IPropertyBag** typeNamesAndCounts)
#else
    /* [retval][out] */ IPropertyBag2** typeNamesAndCounts)
#endif
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    OwnPtr<TypeCountSet> jsObjectTypeNames(JSDOMWindow::commonVM()->heap.protectedObjectTypeCounts());
    typedef TypeCountSet::const_iterator Iterator;
    Iterator end = jsObjectTypeNames->end();
    HashMap<String, int> typeCountMap;
    for (Iterator current = jsObjectTypeNames->begin(); current != end; ++current)
        typeCountMap.set(current->key, current->value);

#if OS(WINCE)
    COMPtr<IPropertyBag> results(AdoptCOM, COMPropertyBag<int>::createInstance(typeCountMap));
#else
    COMPtr<IPropertyBag2> results(AdoptCOM, COMPropertyBag<int>::createInstance(typeCountMap));
#endif
    results.copyRefTo(typeNamesAndCounts);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::iconPageURLMappingCount( 
    /* [retval][out] */ UINT* count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) iconDatabase().pageURLMappingCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::iconRetainedPageURLCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) iconDatabase().retainedPageURLCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::iconRecordCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) iconDatabase().iconRecordCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::iconsWithDataCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) iconDatabase().iconRecordCountWithData();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::cachedFontDataCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) fontCache()->fontDataCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::cachedFontDataInactiveCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) fontCache()->inactiveFontDataCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::purgeInactiveFontData(void)
{
    fontCache()->purgeInactiveFontData();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::glyphPageCount( 
    /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = (UINT) GlyphPageTreeNode::treeGlyphPageCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::webKitData(
     /*[in]*/ AboutDataType typeAboutData,
     /*[in]*/ AboutDataFormat typeFormat,
     /*[out, retval]*/ BSTR *output)
{
    *output = BString(aboutData("/diagnostics/", typeAboutData, typeFormat)).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::resourceCacheImageCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = memoryCache()->getCount(CachedResource::ImageResource);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::resourceCacheCSSCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = memoryCache()->getCount(CachedResource::CSSStyleSheet);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::resourceCacheScriptCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = memoryCache()->getCount(CachedResource::Script);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::resourceCacheFontCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = memoryCache()->getCount(CachedResource::FontResource);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::jsHeapUsageKB(
        /* [retval][out] */ UINT *kb)
{
    if (!kb)
        return E_POINTER;
    JSLockHolder lock(JSDOMWindow::commonVM());
    *kb = (UINT)JSDOMWindow::commonVM()->heap.size() / 1024;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::docCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = InspectorCounters::counterValue(InspectorCounters::DocumentCounter);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::frameCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = WebFrameCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::nodeCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    *count = InspectorCounters::counterValue(InspectorCounters::NodeCounter);
    return S_OK;
}

static void getListenerCount(void * ctx)
{
    *reinterpret_cast<UINT*>(ctx) = ThreadLocalInspectorCounters::current().counterValue(ThreadLocalInspectorCounters::JSEventListenerCounter);
}

HRESULT STDMETHODCALLTYPE WebCoreStatistics::jsListenerCount(
        /* [retval][out] */ UINT *count)
{
    if (!count)
        return E_POINTER;
    if (!WTF::isMainThread()) {
        WTF::callOnMainThreadAndWait(getListenerCount, count);
    }
    else {
        *count = ThreadLocalInspectorCounters::current().counterValue(ThreadLocalInspectorCounters::JSEventListenerCounter);
    }
    return S_OK;
}
