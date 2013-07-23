/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Crank Software Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */


#include "config.h"

#include "WebSocketServer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebCore {

void WebSocketServer::platformInitialize()
{

}

bool WebSocketServer::platformListen(const String& bindAddress, unsigned short port)
{
    struct in_addr addr;
    addr.s_addr = inet_addr(bindAddress.ascii().data());
    if(addr.s_addr == INADDR_NONE)
        return false;
    return m_tcpServerHandler.WinListen(addr.s_addr, port);
}

void WebSocketServer::platformClose()
{
    m_tcpServerHandler.WinClose();
}

}


