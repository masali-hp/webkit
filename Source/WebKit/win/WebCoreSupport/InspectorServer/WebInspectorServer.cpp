/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#if ENABLE(INSPECTOR_SERVER)

#include "WebInspectorServer.h"

#include "InspectorClient.h"
#include "WebSocketServerConnection.h"
#include "HTTPRequest.h"

#include <ws2tcpip.h>
#include "WebView.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebCore {

class WebInspectorHttpRequest : public IWebInspectorHttpRequest {
    WTF_MAKE_NONCOPYABLE(WebInspectorHttpRequest);
public:
    static WebInspectorHttpRequest* create(PassRefPtr<HTTPRequest> request)
    {
        return new WebInspectorHttpRequest(request);
    };

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        *ppvObject = 0;
        if (IsEqualGUID(riid, IID_IWebInspectorHttpRequest))
            *ppvObject = static_cast<IWebInspectorHttpRequest*>(this);
        else if (IsEqualGUID(riid, IID_IUnknown))
            *ppvObject = static_cast<IWebInspectorHttpRequest*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return ++m_refCount;
    }

    ULONG STDMETHODCALLTYPE Release(void)
    {
        ULONG newRef = --m_refCount;
        if (!newRef)
            delete this;
        return newRef;
    }

    // IWebInspectorHttpRequest
    virtual HRESULT STDMETHODCALLTYPE header(BSTR name, BSTR * value)
    {
        String nameStr(name);
        if (m_request->headerFields().contains(nameStr.ascii().data())) {
            BString result(m_request->headerFields().get(nameStr.ascii().data()));
            *value = result.release();
            return S_OK;
        }
        return E_FAIL;
    }

    virtual HRESULT STDMETHODCALLTYPE path(BSTR * path)
    {
        // The URL is not a complete URL
        const String & urlpath = m_request->url();
        *path = SysAllocStringLen(urlpath.characters(), urlpath.length());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE method(BSTR * method)
    {
        *method = SysAllocStringLen(m_request->requestMethod().characters(), m_request->requestMethod().length());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE body(BSTR * body)
    {
        return E_NOTIMPL;
    }

private:
    WebInspectorHttpRequest(PassRefPtr<HTTPRequest> request)
       : m_refCount(1)
       , m_request(request) {}
    long m_refCount;
    RefPtr<HTTPRequest> m_request;
};

static unsigned pageIdFromRequestPath(const String& path)
{
    size_t start = path.reverseFind('/');
    String numberString = path.substring(start + 1, path.length() - start - 1);

    bool ok = false;
    unsigned number = numberString.toUIntStrict(&ok);
    if (!ok)
        return 0;
    return number;
}

WebInspectorServer& WebInspectorServer::shared()
{
    static WebInspectorServer& server = *new WebInspectorServer;
    return server;
}

WebInspectorServer::WebInspectorServer()
    : WebSocketServer(this)
    , m_nextAvailablePageId(1)
    , m_frontendUrl("")
{
}

WebInspectorServer::~WebInspectorServer()
{
    // Close any remaining open connections.
    HashMap<unsigned, WebSocketServerConnection*>::iterator end = m_connectionMap.end();
    for (HashMap<unsigned, WebSocketServerConnection*>::iterator it = m_connectionMap.begin(); it != end; ++it) {
        WebSocketServerConnection* connection = it->value;
        InspectorClient* client = m_clientMap.get(connection->identifier());
        closeConnection(client, connection);
    }
}

int WebInspectorServer::registerPage(WebInspectorClient* client)
{
#ifndef ASSERT_DISABLED
    ClientMap::iterator end = m_clientMap.end();
    for (ClientMap::iterator it = m_clientMap.begin(); it != end; ++it)
        ASSERT(it->second != client);
#endif

    int pageId = m_nextAvailablePageId++;
    m_clientMap.set(pageId, client);
    return pageId;
}

void WebInspectorServer::unregisterPage(int pageId)
{
    m_clientMap.remove(pageId);
    WebSocketServerConnection* connection = m_connectionMap.get(pageId);
    if (connection)
        closeConnection(0, connection);
}

void WebInspectorServer::sendMessageOverConnection(unsigned pageIdForConnection, const String& message)
{
    WebSocketServerConnection* connection = m_connectionMap.get(pageIdForConnection);
    if (connection)
        connection->sendWebSocketMessage(message);
}

void WebInspectorServer::didReceiveUnrecognizedHTTPRequest(WebSocketServerConnection* connection, PassRefPtr<HTTPRequest> prp)
{
    RefPtr<HTTPRequest> request(prp);

    if (m_inspectorDelegate) {
        // First check to see if the inspector delegate can handle the request.
        COMPtr<IWebInspectorHttpRequest> inspectorRequest = WebInspectorHttpRequest::create(request);
        IWebInspectorHttpResponse * inspectorResponse = NULL;
        HRESULT hr = m_inspectorDelegate->didReceiveUnrecognizedHTTPRequest(inspectorRequest.get(), &inspectorResponse);
        if (SUCCEEDED(hr)) {
            int responseCode;
            IWebInspectorHttpHeader * headers;
            int headerCount;
            char * body;
            unsigned int bodySize;
            inspectorResponse->responseCode(&responseCode);
            inspectorResponse->headers(&headers, &headerCount);
            HTTPHeaderMap headerFields;
            for (int i = 0; i < headerCount; i++) {
                BSTR name, value;
                headers[i].name(&name);
                headers[i].value(&value);
                headerFields.set(String(name), String(value));
            }
            inspectorResponse->responseBody((unsigned char **)&body, &bodySize);
            headerFields.set("Content-Length", String::number(bodySize));
            BOOL shutdown;
            inspectorResponse->shutdownConnection(&shutdown);
            if (shutdown)
                headerFields.set("Connection", "close");
            connection->sendHTTPResponseHeader(responseCode, responseCode == 200 ? "OK" : "Error", headerFields);
            if (bodySize)
                connection->sendRawData(body, bodySize);
            if (shutdown)
                connection->shutdownAfterSendOrNow();
            inspectorResponse->Release();
            return;
        }
    }

    // request->url() contains only the path extracted from the HTTP request line
    // and KURL is poor at parsing incomplete URLs, so extract the interesting parts manually.
    String path = request->url();
    size_t pathEnd = path.find('?');
    if (pathEnd == notFound)
        pathEnd = path.find('#');
    if (pathEnd != notFound)
        path.truncate(pathEnd);

    if (path.startsWith("/inspector/disconnect/")) {
        unsigned int pageId = pageIdFromRequestPath(path);
        WebSocketServerConnection* connectionToClose = m_connectionMap.get(pageId);
        if (connectionToClose) {
            closeConnection(m_clientMap.get(pageId), connectionToClose);
        }
        HTTPHeaderMap headerFields;
        headerFields.add("Location", "/");
        connection->sendHTTPResponseHeader(302, "Resource Moved", headerFields);
        return;
    }

    // Ask for the complete payload in memory for the sake of simplicity. A more efficient way would be
    // to ask for header data and then let the platform abstraction write the payload straight on the connection.
    Vector<char> body;
    String contentType;
    bool found = platformResourceForPath(path, body, contentType);
    HTTPHeaderMap headerFields;
    headerFields.set("Connection", "close");
    headerFields.set("Content-Length", String::number(body.size()));
    if (found) {
        if ((path != "/pagelist.json") && (path != "/")) {
            headerFields.set("Cache-Control","max-age=28800");
        }
        headerFields.set("Content-Type", contentType);
    }

    // Send when ready and close immediately afterwards.
    connection->sendHTTPResponseHeader(found ? 200 : 404, found ? "OK" : "Not Found", headerFields);
    if (!body.isEmpty())
        connection->sendRawData(body.data(), body.size());
    connection->shutdownAfterSendOrNow();
}

bool WebInspectorServer::didReceiveWebSocketUpgradeHTTPRequest(WebSocketServerConnection*, PassRefPtr<HTTPRequest> request)
{
    String path = request->url();

    // NOTE: Keep this in sync with WebCore/inspector/front-end/inspector.js.
    DEFINE_STATIC_LOCAL(const String, inspectorWebSocketConnectionPathPrefix, ("/devtools/page/"));

    // Unknown path requested.
    if (!path.startsWith(inspectorWebSocketConnectionPathPrefix))
        return false;

    int pageId = pageIdFromRequestPath(path);
    // Invalid page id.
    if (!pageId)
        return false;

    // There is no client for that page id.
    InspectorClient* client = m_clientMap.get(pageId);
    if (!client)
        return false;

    return true;
}

void WebInspectorServer::didEstablishWebSocketConnection(WebSocketServerConnection* connection, PassRefPtr<HTTPRequest> request)
{
    String path = request->url();
    unsigned pageId = pageIdFromRequestPath(path);
    ASSERT(pageId);

    // Ignore connections to a page that already have a remote inspector connected.
    if (m_connectionMap.contains(pageId)) {
        LOG_ERROR("A remote inspector connection already exist for page ID %d. Ignoring.", pageId);
        connection->shutdownNow();
        return;
    }

    WebInspectorClient* client = m_clientMap.get(pageId);

    struct sockaddr address = connection->remoteAddress();
    char name_buffer[128];
    char port_buffer[16];
    getnameinfo(&address, sizeof(address), name_buffer, 128, port_buffer, 16, NI_NUMERICSERV);
    wchar_t full_name_and_port[144];
    _snwprintf_s(full_name_and_port, 144, 144, L"%S:%S", name_buffer, port_buffer);
    BSTR addrstr = SysAllocString(full_name_and_port);
    if (m_inspectorDelegate) {
        BOOL allow = TRUE;
        COMPtr<IWebView> webView(client->inspectedWebView());
        m_inspectorDelegate->connectionAccepted(webView.get(), (int) pageId, addrstr, &allow);
        if (!allow) {
            LOG_ERROR("A remote inspector connection was denied for page %d. Ignoring.", pageId);
            connection->shutdownNow();
            return;
        }
    }

    // Map the pageId to the connection in case we need to close the connection locally.
    connection->setIdentifier(pageId);
    m_connectionMap.set(pageId, connection);

    client->remoteFrontendConnected(String(addrstr, SysStringLen(addrstr)));
    SysFreeString(addrstr);
}

void WebInspectorServer::didReceiveWebSocketMessage(WebSocketServerConnection* connection, const String& message)
{
    // Dispatch incoming remote message locally.
    unsigned pageId = connection->identifier();
    ASSERT(pageId);
    InspectorClient* client = m_clientMap.get(pageId);
    client->dispatchMessageFromRemoteFrontend(message);
}

void WebInspectorServer::didCloseWebSocketConnection(WebSocketServerConnection* connection)
{
    // Connection has already shut down.
    unsigned pageId = connection->identifier();
    if (!pageId)
        return;

    // The socket closing means the remote side has caused the close.
    InspectorClient* client = m_clientMap.get(pageId);
    closeConnection(client, connection);
}

void WebInspectorServer::closeConnection(InspectorClient* client, WebSocketServerConnection* connection)
{
    m_connectionMap.remove(connection->identifier());

    // Local side cleanup.
    if (client)
        client->remoteFrontendDisconnected();

    if (m_inspectorDelegate)
        m_inspectorDelegate->connectionClosed((int) connection->identifier());

    // Remote side cleanup.
    connection->setIdentifier(0);
    connection->shutdownNow();
}

void WebInspectorServer::setDelegate(IWebRemoteInspectorDelegate * inspectorDelegate)
{
    m_inspectorDelegate = inspectorDelegate;
}

void WebInspectorServer::didSendData(unsigned connectionId, int bytes)
{
    if (m_inspectorDelegate)
        m_inspectorDelegate->connectionBytesSent((int) connectionId, bytes);
}

void WebInspectorServer::didReceiveData(unsigned connectionId, int bytes)
{
    if (m_inspectorDelegate)
        m_inspectorDelegate->connectionBytesReceived((int) connectionId, bytes);
}

}

#endif // ENABLE(INSPECTOR_SERVER)
