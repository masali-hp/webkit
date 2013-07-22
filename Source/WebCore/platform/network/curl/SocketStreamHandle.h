/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

#ifndef SocketStreamHandle_h
#define SocketStreamHandle_h

#include "SocketStreamHandleBase.h"

#include <wtf/PassRefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#endif

#include <curl/curl.h>

namespace WebCore {

    class AuthenticationChallenge;
    class Credential;
    class SocketStreamHandleClient;

    class SocketStreamHandle : public ThreadSafeRefCounted<SocketStreamHandle>, public SocketStreamHandleBase {
    public:
        static PassRefPtr<SocketStreamHandle> create(const KURL& url, SocketStreamHandleClient* client) { return adoptRef(new SocketStreamHandle(url, client)); }
        static PassRefPtr<SocketStreamHandle> create(int fd, SocketStreamHandleClient* client) { return adoptRef(new SocketStreamHandle(fd, client)); }

        virtual ~SocketStreamHandle();

    protected:
        virtual int platformSend(const char* data, int length);
        virtual void platformClose();

    private:
        SocketStreamHandle(const KURL&, SocketStreamHandleClient*);
        SocketStreamHandle(int fd, SocketStreamHandleClient*);

        bool isConnected() { return (m_state == Open || m_state == Closing) && m_curl_code == CURLE_OK && m_curlHandle != NULL; }
        bool connect();

#if OS(WINDOWS)
        friend LRESULT CALLBACK SocketStreamWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
        enum StreamMessage {
            DidFail,
            DidOpen,
            DidReceiveData,
            DidClose,
            DidSelectForWrite,
            DidStopRecvLoop,
        };

        // This method needs to be implemented in a platform dependent
        // manner.  It MUST NOT use WTF::callFunctionOnMainThread,
        // as calls marshalled to the main thread this way are paused
        // when the JS debugger is paused.
        void sendMessageToMainThread(StreamMessage msg);

        static void initMainThreadMessagesStatic();

        // The platform implementation of sendMessageToMainThread
        // will invoke this function when it processes messages:
        void processMessageOnMainThread(StreamMessage msg);

        void didReceiveData();
        int privateSend(char * buf, int length);
        void privateReceive();

        // No authentication for streams per se, but proxy may ask for credentials.
        /*
        void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
        void receivedCredential(const AuthenticationChallenge&, const Credential&);
        void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
        void receivedCancellation(const AuthenticationChallenge&);
        */

        static void recvThreadStart(void * thread);
        void runRecvThread();

        static void sendThreadStart(void * thread);
        void runSendThread();

        static int progressFunctionStatic(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
        int progressFunction(double dltotal, double dlnow, double ultotal, double ulnow);

    private:
        CURL * m_curlHandle;
        long m_socket;
        CURLcode m_curl_code;
        char m_curl_error_buffer[256];
        bool m_platformCloseRequested;

        struct SocketBuffer {
            char * m_data;
            size_t m_size;
            size_t m_sent;
        };

        Mutex m_close_mutex;
        Vector<SocketBuffer> m_receive_buffers;
        Mutex m_receive_buffer_mutex;

        char * m_receive_buffer;
        bool m_use_curl_easy_send_recv;
    };

}  // namespace WebCore

#endif  // SocketStreamHandle_h
