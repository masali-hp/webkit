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

#include "WebInspectorServer.h"

#include "WebInspectorClient.h"
#include "Page.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "DocumentLoader.h"
#include "KURL.h"
#include "FileSystem.h"
#include <MIMETypeRegistry.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

#define INLINE_INSPECTOR_LANDING_PAGE

#if defined(INLINE_INSPECTOR_LANDING_PAGE)
#include "inspectorPageIndex.h"
#endif

char WebInspectorServer_assetsPath[1024];

void WebInspectorServer::setAssetsPath(const char *set)
{
    if (set) {
#if OS(WINDOWS)
        // FIXME?: remove the drive letter from the URL, otherwise the client
        //  browser will automatically prepend "file:///" (which is wrong).
        if ((strlen(set) > 2) && (set[1] == ':'))
            set += 2;
#endif
        strcpy(WebInspectorServer_assetsPath, set);
    }
    else
        WebInspectorServer_assetsPath[0] = 0;
}

std::string WebInspectorServer::prependAssetsPath(const char *path)
{
    std::string fullpath = WebInspectorServer_assetsPath;
    if (path)
        fullpath.append(path);
    return fullpath;
}

namespace WebCore {

void WebInspectorServer::setFrontendUrl(String & frontendUrl, String & hostUrl)
{
    m_frontendUrl = frontendUrl;
    m_hostUrl = hostUrl;
}

bool WebInspectorServer::platformResourceForPath(const String& path, Vector<char>& data, String& contentType)
{
    long long fileSize;
    String localPath;
    // The page list contains an unformated list of pages that can be inspected with a link to open a session.
    if (path == "/pagelist.json" || path == "/json") {
        buildPageList(data, contentType);
        return true;
    }
    
#if defined(INLINE_INSPECTOR_LANDING_PAGE)
    if (path == "/") {
        data.append(inspectorPageIndex_html, sizeof(inspectorPageIndex_html));
        contentType = WebCore::MIMETypeRegistry::getMIMETypeForExtension("html");
        return true;
    }
#else
    // Point the default path to a formatted page that queries the page list and display them.
    localPath = (path == "/") ? prependAssetsPath("/inspector/inspectorPageIndex.html").c_str() : path;
#endif

    if (path ==  "/favicon.ico") {
        localPath = prependAssetsPath("/inspector/favicon.ico").c_str();
    }

    // All other paths are mapped directly to a resource, if possible.

    // Unescape the file URL
#define IS_HEX_CHAR(c) (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))
    std::string localPathStr = localPath.utf8().data();
    int strPos = localPathStr.find('%');
    while (strPos != std::string::npos) {
        if (((strPos + 2) < localPathStr.length()) &&
            IS_HEX_CHAR(localPathStr[strPos + 1]) &&
            IS_HEX_CHAR(localPathStr[strPos + 2])) {
            std::string charCodeEscaped = localPathStr.substr(strPos + 1, 2);
            char charUnescaped = (char)strtol(charCodeEscaped.c_str(), NULL, 16);
            localPathStr.replace(strPos, 3, 1, charUnescaped);
        }
        strPos = localPathStr.find('%', strPos + 1);
    }
    localPath = localPathStr.c_str();

    PlatformFileHandle index = openFile(localPath, OpenForRead);

    if (isHandleValid(index)) {
        if (!getFileSize(localPath, fileSize)) {
            //printf("Could not get file size: %s\n", localPath.ascii().data());
            return false;
        }
        data.grow(fileSize);

        int readsize = readFromFile(index, data.data(),data.size());
        if (readsize != fileSize) {
            //printf("WebInspectorServer: Did not read full file for file %s\n", localPath.ascii().data());
            return false;
        }
        closeFile(index);
        size_t extStart = localPath.reverseFind('.');
        String ext = localPath.substring(extStart != notFound ? extStart + 1 : 0);
        contentType = WebCore::MIMETypeRegistry::getMIMETypeForExtension(ext);
        return true;

   }
   //printf("WebInspector Server: File Not Found: %s\n", localPath.ascii().data());
   return false;
}

void WebInspectorServer::buildPageList(Vector<char>& data, String& contentType)
{
    StringBuilder builder;
    builder.append("[ ");
    ClientMap::iterator end = m_clientMap.end();
    for (ClientMap::iterator it = m_clientMap.begin(); it != end; ++it) {
        WebInspectorClient * client = (WebInspectorClient *)it->value;
        Page* webPage = client->getInspectedPage();
        if (it != m_clientMap.begin())
           builder.append(", ");
        builder.append("{ \"description\": \"");
        String urlstr = webPage->mainFrame()->loader()->activeDocumentLoader()->originalURL().string();
        urlstr.replace("\\", "\\\\");
        urlstr.replace("\"", "\\\"");
        builder.append(urlstr);
        builder.append("\", \"url\": \"");
        urlstr = webPage->mainFrame()->loader()->activeDocumentLoader()->requestURL().string();
        urlstr.replace("\\", "\\\\");
        urlstr.replace("\"", "\\\"");
        builder.append(urlstr);
        builder.append("\", \"inspectorLocation\": \"");
        if (m_frontendUrl == "") {
            builder.append( prependAssetsPath("/inspector/inspector.html?page=").c_str() + String::number(it->key));
        }
        else {
            builder.append(m_frontendUrl + "?ws=" + m_hostUrl + "/devtools/page/" + String::number(it->key));
        }
        builder.append("\", \"webSocketDebuggerUrl\": \"");
        builder.append("ws://" + m_hostUrl + "/devtools/page/" + String::number(it->key));
        builder.append("\", \"clientConnected\":");
        builder.append(String::format("%s", client->hasRemoteFrontendConnected() ? "true" : "false"));
        builder.append(", \"pageId\":");
        builder.append(String::format("%d", it->key));
        builder.append(", \"remoteClientAddress\": \"" + String(client->remoteClientAddress()) + "\"");
        builder.append("}");
   }
   builder.append(" ]");
   CString cstr = builder.toString().utf8();
   data.append(cstr.data(), cstr.length());
   contentType = "application/json; charset=utf-8";
}
}
