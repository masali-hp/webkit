/*
 * Copyright 2012 Crank Software Inc.
 */

#ifndef TcpServerWin_h
#define TcpServerWin_h

#if OS(WINDOWS)
#include <winsock2.h>
#include <windows.h>
#else
#error This file is not be compiled for non windows platforms
#endif

namespace WebCore {
class TcpServerWin {
public:
    enum
    {
        STATE_INIT = 0,
        STATE_LISTENING,
        STATE_CLOSE
    };

    TcpServerWin();
    ~TcpServerWin();

    bool WinListen(unsigned long bindAddress, unsigned short port);
    void WinClose();

private:
    static DWORD __stdcall ClientLoop(void * data);

    static int m_state;

    SOCKET m_serverFd;
    HANDLE m_threadId;

    static CRITICAL_SECTION  m_stateMutex;
};
}
#endif //TcpServerWin_h
