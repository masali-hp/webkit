/*
 * Copyright (C) 2012 Hewlett-Packard Development Company, L.P.
 * All rights reserved.
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
#include "SocketStreamHandle.h"

namespace WebCore {

static HWND streamWindowHandle;
static UINT streamFiredMessage;
const LPCWSTR kWindowClassName = L"SocketStreamWindowClass";

LRESULT CALLBACK SocketStreamWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == streamFiredMessage) {
        SocketStreamHandle * handle = (SocketStreamHandle*)wParam;
        handle->processMessageOnMainThread((SocketStreamHandle::StreamMessage) lParam);
    }
    else
        return DefWindowProc(hWnd, message, wParam, lParam);
    return 0;
}

void SocketStreamHandle::initMainThreadMessagesStatic()
{
    if (streamWindowHandle)
        return;

    HWND hWndParent = 0;
#if OS(WINCE)
    WNDCLASS wcex;
    memset(&wcex, 0, sizeof(WNDCLASS));
#else
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
#endif
    wcex.lpfnWndProc    = SocketStreamWindowWndProc;
    wcex.lpszClassName  = kWindowClassName;
#if OS(WINCE)
    RegisterClass(&wcex);
#else
    RegisterClassEx(&wcex);
    hWndParent = HWND_MESSAGE;
#endif

    streamWindowHandle = CreateWindow(kWindowClassName, 0, 0,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWndParent, 0, 0, 0);
    streamFiredMessage = RegisterWindowMessage(L"com.apple.WebKit.SocketStream");
}

void SocketStreamHandle::sendMessageToMainThread(StreamMessage msg)
{
    ref();
    PostMessage(streamWindowHandle, streamFiredMessage, (WPARAM) this, msg);
}

}
