/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (C) 2013 Hewlett-Packard Development Company
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SocketStreamHandle.h"

#include "KURL.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamError.h"
#include "ResourceHandleManager.h"

#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include <wtf/PerformanceTrace.h>

const size_t receiveBufferSize = 16384;

double threadStartTime;

#define LOG_CONNECT LOG

/*#undef LOG
#define LOG(channel, ...) va_output_debug_string(__VA_ARGS__)

static void va_output_debug_string(const char* format, ...)
{
    va_list args;
    va_start(args, format);
#define STATIC_BUF_SIZE 1024
    size_t size = STATIC_BUF_SIZE;
    char static_buffer[STATIC_BUF_SIZE];
    do {
        char* buffer;
        if (size == STATIC_BUF_SIZE)
            buffer = static_buffer;
        else
            buffer = (char*)malloc(size);

        if (buffer == NULL)
            break;

        if (_vsnprintf(buffer, size, format, args) != -1) {
#if OS(WINCE)
            // WinCE only supports wide chars
            wchar_t* wideBuffer;
            wchar_t static_wide[STATIC_BUF_SIZE];
            if (size == STATIC_BUF_SIZE)
                wideBuffer = static_wide;
            else
                wideBuffer = (wchar_t*)malloc(size * sizeof(wchar_t));
            if (wideBuffer == NULL)
                break;
            for (unsigned int i = 0; i < size; ++i) {
                if (!(wideBuffer[i] = buffer[i]))
                    break;
            }
            OutputDebugStringW(wideBuffer);
            if (wideBuffer != static_wide)
                free(wideBuffer);
#else
            OutputDebugStringA(buffer);
#endif
            if (buffer != static_buffer)
                free(buffer);
            break;
        }
        if (buffer != static_buffer)
            free(buffer);
        size *= 2;
    } while (size > 1024);
    va_end(args);
}
*/

// uncomment to debug connect/disconnect issues:
//#undef LOG_CONNECT
//#if PLATFORM(HP)
//#define LOG_CONNECT(x, ...) HP_TRACE_WARN(__VA_ARGS__)
//#else
//#define LOG_CONNECT(x, ...)
//#endif

static int socketCount = 0;

namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
    , m_curlHandle(NULL)
    , m_socket(-1)
    , m_curl_code(CURLE_OK)
    , m_platformCloseRequested(false)
    , m_receive_buffer(NULL)
    , m_use_curl_easy_send_recv(false)
{
    socketCount++;
    initMainThreadMessagesStatic();
    LOG_CONNECT(Network, "SocketStreamHandleCurl: new url (%s) websocket, client = %p, count=%d [%p][thread=%d]\n", url.string().ascii().data(), client, socketCount, this, GetCurrentThreadId());
    m_curl_error_buffer[0] = '\0';
    if (client)
        client->willOpenSocketStream(this);
    ref();
    createThread(recvThreadStart, this, "WebKit: WebSockets");
}

SocketStreamHandle::SocketStreamHandle(int fd, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(KURL(), client)
    , m_curlHandle(NULL)
    , m_socket(fd)
    , m_curl_code(CURLE_OK)
    , m_platformCloseRequested(false)
    , m_receive_buffer(NULL)
    , m_use_curl_easy_send_recv(false)
{
    socketCount++;
    initMainThreadMessagesStatic();
    LOG_CONNECT(Network, "SocketStreamHandleCurl: new fd websocket, fd = %d, client = %p, count=%d [%p][thread=%d]\n", fd, client, socketCount, this, GetCurrentThreadId());
    m_curl_error_buffer[0] = '\0';
    if (client)
        client->willOpenSocketStream(this);
    ref();
    createThread(recvThreadStart, this, "WebKit: WebSockets");
}

SocketStreamHandle::~SocketStreamHandle()
{
    socketCount--;
    LOG_CONNECT(Network, "SocketStreamHandleCurl: delete, count=%d [%p][thread=%d]\n", socketCount, this, GetCurrentThreadId());
    setClient(0);
    // if for some reason we weren't closed before being destroyed, ensure we release resources now.
    platformClose();
}

static curl_socket_t getsocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address) {
    return (curl_socket_t) clientp;
}

static int getsockopts(void *clientp, curl_socket_t curlfd, curlsocktype purpose) {
    return CURL_SOCKOPT_ALREADY_CONNECTED;
}

int SocketStreamHandle::progressFunctionStatic(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    return static_cast<SocketStreamHandle*>(clientp)->progressFunction(dltotal, dlnow, ultotal, ulnow);
}

int SocketStreamHandle::progressFunction(double dltotal, double dlnow, double ultotal, double ulnow) {
    if (m_platformCloseRequested)
        return 1; // returning a non-zero value will cause the connect to be aborted with result CURLE_ABORTED_BY_CALLBACK
    return 0;
}

static int cURL_debug(CURL *handle, curl_infotype type,
               char *ptr, size_t size,
               void *userptr)
{
    SocketStreamHandle* streamHandle;
    curl_easy_getinfo(handle, CURLINFO_PRIVATE, &streamHandle);
    char buff[256];
    strncpy(buff, ptr, 255);
    buff[255] = '\0';
    LOG(Network, "SocketStreamHandleCurl: [%p][thread=%d] {%s} %s%s", streamHandle, GetCurrentThreadId(), type == CURLINFO_TEXT ? "TEXT" : "Other", buff, 
        (size < 256 && buff[size-1] == '\n') ? "" : "\n");
    return 0;
}

bool SocketStreamHandle::connect() {
    ASSERT(state() == Connecting);

    MutexLocker lock(m_close_mutex);

    m_curlHandle = curl_easy_init();
    if ( !m_curlHandle ) {
        m_curl_code = CURLE_OUT_OF_MEMORY;
        sendMessageToMainThread(DidFail);
        return false;
    }

    curl_easy_setopt(m_curlHandle, CURLOPT_PRIVATE, this);
    curl_easy_setopt(m_curlHandle, CURLOPT_ERRORBUFFER, m_curl_error_buffer);

    if (m_socket == -1) {
        // Curl connect and setup
        const String & proxy = ResourceHandleManager::sharedInstance()->getProxyString();

        if (proxy.length()) {
            bool isLocalHttp = (m_url.host() == "localhost" || m_url.host().startsWith("127.0.0.") || m_url.host().startsWith("::1"));
            if (!isLocalHttp) {
                curl_easy_setopt(m_curlHandle, CURLOPT_PROXY, proxy.utf8().data());
                curl_easy_setopt(m_curlHandle, CURLOPT_PROXYTYPE, ResourceHandleManager::sharedInstance()->getProxyType());
            }
        }
        // curl doesn't understand a ws:// protocol, so switch it to http
        if ( m_url.protocolIs("ws") ) {
            KURL urlcopy = m_url;
            urlcopy.setProtocol("http");
            curl_easy_setopt(m_curlHandle, CURLOPT_URL, urlcopy.string().latin1().data());
        }
        else if ( m_url.protocolIs("wss") ) {
            KURL urlcopy = m_url;
            urlcopy.setProtocol("https");
            curl_easy_setopt(m_curlHandle, CURLOPT_URL, urlcopy.string().latin1().data());
            m_use_curl_easy_send_recv = true;
        }
        else {
            curl_easy_setopt(m_curlHandle, CURLOPT_URL,  m_url.string().latin1().data());
        }
        curl_easy_setopt(m_curlHandle, CURLOPT_PORT, m_url.port());
        curl_easy_setopt(m_curlHandle, CURLOPT_CONNECTTIMEOUT, 15); // we'll wait for up to 15 seconds for the connection to be made.
        curl_easy_setopt(m_curlHandle, CURLOPT_TCP_NODELAY, TRUE);

        // in case we want to close the connection while it's being made:
        curl_easy_setopt(m_curlHandle, CURLOPT_PROGRESSFUNCTION, progressFunctionStatic);
        curl_easy_setopt(m_curlHandle, CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 0);
    }
    else {
        // The connection has already been made.  Initialize the curl handle with the
        // provided fd.  We're not really connecting to 127.0.0.1, but have to provide
        // a URL or CURL will fail the handle almost immediately.  The URL needs
        // to be unique from other active SocketStreamHandle instances or CURL
        // will try to re-use a different connection.
        char url_buf[32];
        sprintf(url_buf, "http://127.0.0.1:%d", m_socket % 0x10000);
        curl_easy_setopt(m_curlHandle, CURLOPT_URL, url_buf);
        curl_easy_setopt(m_curlHandle, CURLOPT_OPENSOCKETFUNCTION, getsocket);
        curl_easy_setopt(m_curlHandle, CURLOPT_SOCKOPTFUNCTION, getsockopts);
        curl_easy_setopt(m_curlHandle, CURLOPT_OPENSOCKETDATA, m_socket);
    }
    curl_easy_setopt(m_curlHandle, CURLOPT_CONNECT_ONLY, 1L);

    //comment this
    // curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1);
    // curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, cURL_debug);

    m_curl_code = curl_easy_perform(m_curlHandle);
    if ( m_curl_code != CURLE_OK ) {
        LOG_CONNECT(Network, "SocketStreamHandleCurl: startConnect failed to connect [%p][thread=%d]\n", this, GetCurrentThreadId());
        sendMessageToMainThread(DidFail);
        return false;
    }

    if (m_socket == -1) {
        m_curl_code = curl_easy_getinfo(m_curlHandle, CURLINFO_LASTSOCKET, &m_socket);
        if ( m_curl_code != CURLE_OK ) {
            LOG_CONNECT(Network, "SocketStreamHandleCurl: startConnect failed to get socket from curl [%p][thread=%d]\n", this, GetCurrentThreadId());
            sendMessageToMainThread(DidFail);
            return false;
        }
    }

    if (!m_platformCloseRequested) {
        LOG_CONNECT(Network, "SocketStreamHandleCurl: startConnect success, changing state to open [%p][thread=%d]\n", this, GetCurrentThreadId());
        m_state = Open;
        sendMessageToMainThread(DidOpen);
        return true;
    }
    else {
        return false;
    }
}

int SocketStreamHandle::platformSend(const char* buf, int length)
{
    if (!isConnected() || length == 0) {
        return 0;
    }

    //LOG(Network, "SocketStreamHandleCurl: platformSend(), sending %d bytes. [%p][thread=%d]\n", length, this, GetCurrentThreadId()); // comment this
    int bytes_written = 0;
    if (m_use_curl_easy_send_recv) {
        CURLcode rc = curl_easy_send(m_curlHandle, buf, length, (size_t*) &bytes_written);
        if (rc == CURLE_AGAIN)
            bytes_written = 0;
        else
            m_curl_code = rc;
    }
    else {
        bytes_written = ::send(m_socket, buf, length, 0);
        if (bytes_written == -1) {
            int error = WSAGetLastError();
            LOG_CONNECT(Network, "SocketStreamHandleCurl: platformSend(), error on send (%d) [%p][thread=%d]\n", error, this, GetCurrentThreadId());
            if (WSAEWOULDBLOCK == error)
                bytes_written = 0;
            else
                m_curl_code = CURLE_SEND_ERROR;
        }
    }

    if (m_curl_code != CURLE_OK) {
        LOG_CONNECT(Network, "SocketStreamHandleCurl: platformSend, error sending data [%p][thread=%d]\n", this, GetCurrentThreadId());
        sendMessageToMainThread(DidFail);
        return 0;
    }

    if (bytes_written < length) {
        LOG_CONNECT(Network, "SocketStreamHandleCurl: platformSend(), starting send thread (bytes written=%d) [%p][thread=%d]\n", bytes_written, this, GetCurrentThreadId()); // comment this
        // after we return, SocketStreamHandleBase will buffer up the data.
        // signal our send thread that we have buffered data.
        threadStartTime = WTF::currentTimeMS();
        ref();
        createThread(sendThreadStart, this, "WebKit: WebSocket Send");
    }

    return bytes_written;
}

void SocketStreamHandle::platformClose()
{
    if (m_platformCloseRequested)
        return;

    LOG_CONNECT(Network, "SocketStreamHandleCurl: platformClose [%p][thread=%d]\n", this, GetCurrentThreadId());
    m_platformCloseRequested = true;

    MutexLocker lock(m_close_mutex);

    if (m_curlHandle != NULL) {
        LOG_CONNECT(Network, "SocketStreamHandleCurl: platformClose, closing curl handle at %p [%p][thread=%d]\n", m_curlHandle, this, GetCurrentThreadId());
        curl_easy_cleanup(m_curlHandle);
        m_curlHandle = NULL;
    }
    if (m_client)
        m_client->didCloseSocketStream(this);

    // after platformClose() returns it is very possible the client is deleted, without informing us.
    setClient(0);

    if (m_receive_buffer)
        fastFree(m_receive_buffer);
}

// private methods
void SocketStreamHandle::processMessageOnMainThread(StreamMessage msg)
{
    switch (msg) {
        case DidOpen:
            ASSERT(m_state == Open);
            if (m_client)
                m_client->didOpenSocketStream(this);
            break;
        case DidFail:
            ASSERT(m_curl_code != CURLE_OK);
            LOG_CONNECT(Network, "SocketStreamHandleCurl: DidFail, error %d (%s), curl error buffer: %s [%p][thread=%d]\n", m_curl_code, curl_easy_strerror(m_curl_code), m_curl_error_buffer, this, GetCurrentThreadId());
            if (m_client)
                m_client->didFailSocketStream(this, SocketStreamError(m_curl_code, m_url.isEmpty() ? String() : m_url.string()));
            break;
        case DidReceiveData:
            didReceiveData();
            break;
        case DidSelectForWrite: {
            deref(); // this balances the ref() when spinning up the send wait thread.
            // LOG(Network, "SocketStreamHandleCurl: DidSelectForWrite [%p][thread=%d]\n", this, GetCurrentThreadId());
            if (!m_platformCloseRequested) {
                if (m_curl_code == CURLE_OK) {
                    sendPendingData();
                }
                else if (m_client) {
                    m_client->didFailSocketStream(this, SocketStreamError(m_curl_code, m_url.isEmpty() ? String() : m_url.string()));
                }
            }
            } break;
        case DidClose:
            disconnect();
            break;
        case DidStopRecvLoop:
            deref(); // this balances the ref() in the constructor.
            break;
    }
    deref();
}

void SocketStreamHandle::didReceiveData() {
    size_t totalSize = 0;
    char * dataBuffer = NULL;
    {
        MutexLocker lock(m_receive_buffer_mutex);
        ASSERT(m_receive_buffers.size() > 0);
        if (m_receive_buffers.size() == 1) {
            dataBuffer = m_receive_buffers[0].m_data;
            totalSize = m_receive_buffers[0].m_size;
        }
        else {
            for (int i = 0; i < m_receive_buffers.size(); i++)
                totalSize += m_receive_buffers[i].m_size;
            dataBuffer = (char*) fastMalloc(totalSize);
            int p = 0;
            for (int i = 0; i < m_receive_buffers.size(); i++) {
                memcpy(dataBuffer + p, m_receive_buffers[i].m_data, m_receive_buffers[i].m_size);
                fastFree(m_receive_buffers[i].m_data);
                p += m_receive_buffers[i].m_size;
            }
        }
        m_receive_buffers.clear();
    }
    if (m_client && m_state == Open) {
        //LOG(Network, "SocketStreamHandleCurl: didReceiveDataStatic(), received %d bytes [%p][thread=%d]\n", totalSize, this, GetCurrentThreadId()); // comment this
        PERFORMANCE_START(WTF::PerformanceTrace::WebSocketData);
        m_client->didReceiveSocketStreamData(this, dataBuffer, totalSize);
        PERFORMANCE_END(WTF::PerformanceTrace::WebSocketData);
    }
    fastFree(dataBuffer);
}

void SocketStreamHandle::privateReceive()
{
    SocketBuffer buffer;
    buffer.m_data = m_receive_buffer;

    // The downside of using curl_easy_recv is that it does a read on the socket with MSG_PEEK
    // before doing the real read each time (inside easy_connection) to see if the socket is still
    // connected.  This is extra unnecessary overhead.
    if (m_use_curl_easy_send_recv) {
        MutexLocker lock(m_close_mutex);
        m_curl_code = curl_easy_recv(m_curlHandle, buffer.m_data, receiveBufferSize, &buffer.m_size);
        if (m_curl_code == CURLE_AGAIN) {
            m_curl_code = CURLE_OK;
            return;
        }
    }
    else {
        MutexLocker lock(m_close_mutex);
        int nread = recv(m_socket, buffer.m_data, receiveBufferSize, 0);
        if (-1 == nread) {
            int error = WSAGetLastError();
            LOG_CONNECT(Network, "SocketStreamHandleCurl: privateReceive, error on recv (%d) [%p][thread=%d]\n", error, this, GetCurrentThreadId());
            // If we get WOULDBLOCK, then we can't read data now, so go back to the select loop.
            if (WSAEWOULDBLOCK == error)
                return;
            else
                m_curl_code = CURLE_RECV_ERROR;
        }
        else if (0 == nread) {
            // the remote side has closed the connection
            m_curl_code = CURLE_UNSUPPORTED_PROTOCOL;
        }
        else
            buffer.m_size = nread;
    }

    if (m_curl_code != CURLE_OK) {
        if (m_curl_code == CURLE_UNSUPPORTED_PROTOCOL) {
            LOG_CONNECT(Network, "SocketStreamHandleCurl: privateReceive, remote side closed connection [%p][thread=%d]\n", this, GetCurrentThreadId());
            sendMessageToMainThread(DidClose);
        }
        else {
            LOG_CONNECT(Network, "SocketStreamHandleCurl: privateReceive, error receiving data [%p][thread=%d]\n", this, GetCurrentThreadId());
            sendMessageToMainThread(DidFail);
        }
        return;
    }

    //LOG(Network, "SocketStreamHandleCurl: privateReceive, read %d bytes [%p][thread=%d]\n", buffer.m_size, this, GetCurrentThreadId()); // comment this
    MutexLocker lock(m_receive_buffer_mutex);
    m_receive_buffer = NULL;
    m_receive_buffers.append(buffer);
    if (m_receive_buffers.size() == 1) {
        //LOG(Network, "SocketStreamHandleCurl: privateReceive, received data [%p]\n", this); // comment this
        sendMessageToMainThread(DidReceiveData);
    }
}

void SocketStreamHandle::recvThreadStart(void * data)
{
    static_cast<SocketStreamHandle*>(data)->runRecvThread();
}

void SocketStreamHandle::runRecvThread()
{
    static int runRecvThreadCounter = 0;

    runRecvThreadCounter++;
    LOG_CONNECT(Network, "SocketStreamHandleCurl: starting runRecvThread, count = %d [%p][thread=%d]\n", runRecvThreadCounter, this, GetCurrentThreadId());

    if (connect()) {
        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;

        while (isConnected() && !m_platformCloseRequested) {
            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            FD_SET(m_socket, &fdexcep);
            FD_SET(m_socket, &fdread);

            if (m_receive_buffer == NULL)
                m_receive_buffer = (char*)fastMalloc(receiveBufferSize);

            int rc = ::select(m_socket + 1, &fdread, &fdwrite, &fdexcep, NULL);
            if (rc > 0 && !m_platformCloseRequested) {
                if (FD_ISSET(m_socket, &fdexcep)) {
                    LOG_CONNECT(Network, "SocketStreamHandleCurl: processActiveJobs, socket exception occured [%p][thread=%d]\n", this, GetCurrentThreadId());
                    // The remote side probably closed the connection.
                    m_curl_code = CURLE_UNSUPPORTED_PROTOCOL;
                    sendMessageToMainThread(DidClose);
                    continue;
                }
                if (FD_ISSET(m_socket, &fdread)) {
                    privateReceive();
                }
            }
        }
    }

    runRecvThreadCounter--;
    LOG_CONNECT(Network, "SocketStreamHandleCurl: exiting runRecvThread, count = %d [%p][thread=%d]\n", runRecvThreadCounter, this, GetCurrentThreadId());

    sendMessageToMainThread(DidStopRecvLoop);
}

void SocketStreamHandle::sendThreadStart(void * data)
{
    static_cast<SocketStreamHandle*>(data)->runSendThread();
}

void SocketStreamHandle::runSendThread()
{
    static int sendThreadCounter = 0;
    int threadStartMS = (int)(WTF::currentTimeMS() - threadStartTime);

    sendThreadCounter++;
    LOG_CONNECT(Network, "SocketStreamHandleCurl: send thread (count=%d) launched in %d MS [%p][thread=%d]\n", sendThreadCounter, threadStartMS, this, GetCurrentThreadId());

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    FD_SET(m_socket, &fdexcep);
    FD_SET(m_socket, &fdwrite);

    if (!m_platformCloseRequested) {
        int rc = ::select(m_socket + 1, &fdread, &fdwrite, &fdexcep, NULL);
        if (rc <= 0 || !FD_ISSET(m_socket, &fdwrite)) {
            m_curl_code = CURLE_SEND_ERROR;
        }
    }
    sendMessageToMainThread(DidSelectForWrite);
    sendThreadCounter--;
}


}  // namespace WebCore