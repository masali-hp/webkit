/**
 * Copyright 2012 Crank Software Inc.
 */
#include "config.h"

#if OS(WINDOWS)
#include <windows.h>
#include <stdio.h>
#else
#error This file is not be compiled for non windows platforms
#endif

#include "TcpServerWin.h"
#include "SocketStreamHandle.h"
#include "SocketStreamHandleClient.h"
#include "WebInspectorServer.h"

#include <wtf/MainThread.h>

namespace WebCore {

#define STACK_SIZE 8 * 1024 * 1204

int TcpServerWin::m_state = STATE_INIT;
CRITICAL_SECTION TcpServerWin::m_stateMutex;

TcpServerWin::TcpServerWin()
    : m_serverFd(INVALID_SOCKET)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int ret;

    wVersionRequested = MAKEWORD(2, 2);

    ret = WSAStartup(wVersionRequested, &wsaData);
    if(ret != 0) {
        printf("WSAStartup failed %d \n", ret);
    }

    m_serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_serverFd == INVALID_SOCKET) {
        printf("Failed to initialize socket %d\n", WSAGetLastError());
    }

    InitializeCriticalSection(&m_stateMutex);

    m_threadId = CreateThread(NULL, STACK_SIZE, (LPTHREAD_START_ROUTINE)TcpServerWin::ClientLoop, &m_serverFd, 0, NULL);
}

TcpServerWin::~TcpServerWin()
{
    WinClose();

    WaitForSingleObject(m_threadId, INFINITE);
}

bool
TcpServerWin::WinListen(unsigned long bindAddress, unsigned short port)
{
    struct sockaddr_in serverAddr;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port  = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Web Server Bind failed %d\n", WSAGetLastError());
        return false;
    }

    if (listen(m_serverFd, 10) < 0) {
        printf("Web Server Listen Failed %d\n", WSAGetLastError());
        return false;
    }

    EnterCriticalSection(&m_stateMutex);
    m_state = STATE_LISTENING;
    LeaveCriticalSection(&m_stateMutex);

    return true;
}

void TcpServerWin::WinClose()
{
    closesocket(m_serverFd);
    EnterCriticalSection(&m_stateMutex);
    m_state = STATE_CLOSE;
    LeaveCriticalSection(&m_stateMutex);
}

int global_inspector_server_done = 0;


static void mainThreadAcceptFunction(void *context) {
    WebCore::WebInspectorServer::shared().didAcceptConnection((ClientAcceptInfo*)context);
}

DWORD
__stdcall TcpServerWin::ClientLoop(void * data)
{
    SOCKET *serverFd = (SOCKET*)data;

    //CRANK-ME Need to protect the m_state since it called from call backs too.
    // Fixme: 
    //  This thread will not exit until STATE_CLOSE.
    //  STATE_CLOSE will not happen until the destructor is called.
    //  This instance is statically initialized.
    //  On windows, the destructor will not be called until all the threads exits.
    while (!global_inspector_server_done) {
        EnterCriticalSection(&m_stateMutex);
        int state = m_state;
        LeaveCriticalSection(&m_stateMutex);

        switch (state) {
        case STATE_INIT:
        {
            Sleep(2000);
            break;
        }
        case STATE_LISTENING:
        {
            int connectionFd = -1;
            struct timeval tv;
            fd_set rfds;

            memset(&rfds, 0, sizeof(rfds));

            FD_SET(*serverFd, &rfds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int n = select(*serverFd + 1, &rfds, NULL, NULL, &tv);

            ClientAcceptInfo acceptInfo = {0, 0};
            int sizeOfAddress = sizeof(acceptInfo.address);
            if (n > 0)
                acceptInfo.sockfd = accept(*serverFd, &acceptInfo.address, &sizeOfAddress);

            if (acceptInfo.sockfd > 0) {
                //Send Events
//CRANK TF: Let someone know we've got things going on here
#if 0
                WinWebInspectorServerAcceptEvent *event = new WinWebInspectorServerAcceptEvent((int)connectionFd);
                if (event)
                    WebKitSendEvent(event);
                else
                    closesocket(connectionFd);
#else
                callOnMainThreadAndWait(mainThreadAcceptFunction, &acceptInfo);
#endif
            }
            break;
        }
        case STATE_CLOSE:
        {
            printf("TcpServerWin, shutting down\n");
            //closesocket(m_serverFd);
            EnterCriticalSection(&m_stateMutex);
            m_state = STATE_CLOSE;
            LeaveCriticalSection(&m_stateMutex);
            return true;
        }
        default:
        {
            Sleep(2000);
            break;
        }
        }
    }

    //Should never get here ...
    return true;
}

}


