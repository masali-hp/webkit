/*
 * Copyright (C) 2012 Hewlett-Packard Development Company Inc.  All rights reserved.
 * <insert license here>
 */

#ifndef WebNetworkConfiguration_h
#define WebNetworkConfiguration_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>

#if USE(CURL)
#include <WebCore/ResourceHandleManager.h>
#endif

class WebNetworkConfiguration
    : public IWebNetworkConfiguration
#if USE(CURL)
    , public WebCore::CurlDebugCallback
#endif
{
public:
    static WebNetworkConfiguration* createInstance();
private:
    WebNetworkConfiguration();
    ~WebNetworkConfiguration();
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebNetworkConfiguration
    virtual HRESULT STDMETHODCALLTYPE setProxyInfo(
        /* [in] */ BSTR proxyHost,
        /* [in] */ int port,
        /* [in] */ BSTR username,
        /* [in] */ BSTR password);

    virtual HRESULT STDMETHODCALLTYPE setDebugDelegate(
        /* [in] */ IWebNetworkDebugDelegate *delegate);

protected:

#if USE(CURL)
    virtual void jobStarted(int jobId, const String & url, bool synchronous);
    virtual void jobDataComplete(int jobId, int httpCode);
    virtual void jobFinished(int jobId, BOOL success, int errorCode);
    virtual void jobDebug(int jobId, int debugType, unsigned char * data, int totalSize);
#endif

private:
#if USE(CURL)
    COMPtr<IWebNetworkDebugDelegate> m_delegate;
#endif
    ULONG m_refCount;
};


#endif
