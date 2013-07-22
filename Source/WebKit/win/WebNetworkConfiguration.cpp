#include "config.h"
#include "WebKitDLL.h"
#include "WebNetworkConfiguration.h"

using namespace WebCore;

// WebNetworkConfiguration ----------------------------------------------------------------

WebNetworkConfiguration::WebNetworkConfiguration()
    : m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebNetworkConfiguration");
}

WebNetworkConfiguration::~WebNetworkConfiguration()
{
    gClassCount--;
    gClassNameCount.remove("WebNetworkConfiguration");
}

WebNetworkConfiguration* WebNetworkConfiguration::createInstance()
{
    WebNetworkConfiguration* instance = new WebNetworkConfiguration();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebNetworkConfiguration::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebNetworkConfiguration*>(this);
    else if (IsEqualGUID(riid, IID_IWebNetworkConfiguration))
        *ppvObject = static_cast<IWebNetworkConfiguration*>(this);
    else if (IsEqualGUID(riid, CLSID_WebNetworkConfiguration))
        *ppvObject = static_cast<WebNetworkConfiguration*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebNetworkConfiguration::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebNetworkConfiguration::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebNetworkConfiguration -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebNetworkConfiguration::setProxyInfo(BSTR proxyHost, int port, BSTR username, BSTR password)
{
#if USE(CURL)
    ResourceHandleManager::sharedInstance()->setProxyInfo(proxyHost, port, ResourceHandleManager::HTTP, username, password);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE WebNetworkConfiguration::setDebugDelegate(IWebNetworkDebugDelegate *delegate)
{
#if USE(CURL)
    bool delegateIsSet = m_delegate;
    m_delegate = delegate;
    if (delegate && !delegateIsSet)
        AddRef();
    else if (!delegate && delegateIsSet)
        Release();
    ResourceHandleManager::sharedInstance()->setCurlDebugCallback(delegate ? this : NULL);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

#if USE(CURL)
// CurlDebugCallback -------------------------------------------------------------------
void WebNetworkConfiguration::jobStarted(int jobId, const String & url, bool synchronous)
{
    if (m_delegate) {
        BSTR bstrUrl = SysAllocStringLen(url.characters(), url.length());
        m_delegate->jobStarted(jobId, bstrUrl, synchronous);
        SysFreeString(bstrUrl);
    }
}

void WebNetworkConfiguration::jobDataComplete(int jobId, int httpCode)
{
    if (m_delegate) {
        m_delegate->jobDataComplete(jobId, httpCode);
    }
}

void WebNetworkConfiguration::jobFinished(int jobId, BOOL success, int errorCode)
{
    if (m_delegate) {
        m_delegate->jobFinished(jobId, success, errorCode);
    }
}

void WebNetworkConfiguration::jobDebug(int jobId, int debugType, unsigned char * data, int totalSize)
{
    if (m_delegate) {
        m_delegate->jobDebug(jobId, (DebugInformation) debugType, data, totalSize);
    }
}

#endif
