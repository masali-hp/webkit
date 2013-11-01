/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AboutData.h"
#include "InspectorCounters.h"
#include "JSDOMWindow.h"
#include "MemoryCache.h"
#include "MemoryStatistics.h"
#include "WebKitVersion.h"
#include "WebView.h"
#include <heap/Heap.h>
#include <sys/stat.h>
#include <wtf/text/CString.h>
#include <wtf/hp/HPWebkitMalloc.h>

using namespace WebCore;

const int s_kiloByte = 1024;

template<class T> static void numberToHTMLTr(const String& description, T number, StringBuilder & output)
{
    output.append("<tr><td>");
    output.append(description);
    output.append("</td><td>");
    output.append(String::number(number));
    output.append("</td></tr>\n");
}

static void cacheTypeStatisticToHTMLTr(const String& description, const MemoryCache::TypeStatistic& statistic, StringBuilder & output)
{
    output.append("<tr><td>");
    output.append(description);
    output.append("</td><td>");
    output.appendNumber(statistic.count);
    output.append("</td><td>");
    output.appendNumber(statistic.size / s_kiloByte);
    output.append("</td><td>");
    output.appendNumber(statistic.liveSize / s_kiloByte);
    output.append("</td><td>");
    output.appendNumber(statistic.decodedSize / s_kiloByte);
    output.append("</td></tr>\n");
}

template<class T> static void numberToXMLTr(const String& description, T number, StringBuilder & output)
{
    output.append(description);
    output.appendNumber(number);
    output.append("\"/>\n");
}

static void cacheTypeStatisticToXmlTr(const String& description, const MemoryCache::TypeStatistic& statistic, StringBuilder & output)
{
    output.append(description);
    output.append(" Count=\"");
    output.appendNumber(statistic.count);
    output.append("\" Size=\"");
    output.appendNumber(statistic.size / s_kiloByte);
    output.append("\" Living=\"");
    output.appendNumber(statistic.liveSize / s_kiloByte);
    output.append("\" Decoded=\"");
    output.appendNumber(statistic.decodedSize / s_kiloByte);
    output.append("\"/>\n");
}

static void outputCacheDetails(const char * description, CachedResource::Type resourceType, StringBuilder & page, AboutDataFormat format)
{
    if (format == XML) {
        page.append("<");
        page.append(description);
        page.append(">\n");
    }
    int resourceCount = memoryCache()->getCount(resourceType);
    for (int n = 0; n < resourceCount; n++) {
        MemoryCache::TypeStatistic details;
        String url;
        CachedResource::Status status;
        bool inCache;
        memoryCache()->getDetails(resourceType, n, details, url, status, inCache);
        String statusString;
        switch (status) {
        case CachedResource::Unknown:
            statusString = "Unknown";
            break;
        case CachedResource::Pending:
            statusString = "Pending";
            break;
        case CachedResource::Cached:
            statusString = "Cached";
            break;
        case CachedResource::LoadError:
            statusString = "Load Error";
            break;
        case CachedResource::DecodeError:
            statusString = "Decode Error";
            break;
        }
        if (format == HTML) {
            page.append("<tr><td colspan=2>");
            page.append(url);
            if (!inCache || status != CachedResource::Cached) {
                page.append(" (");
                if (!inCache && status != CachedResource::Cached) {
                    page.append("NOT in cache; Load Status: ");
                    page.append(statusString);
                }
                else if (!inCache) {
                    page.append("NOT in cache");
                }
                else {
                    page.append(statusString);
                }
                page.append(")");
            }
            page.append("</td><td>");
            page.appendNumber(details.size / s_kiloByte);
            page.append("</td><td>");
            page.appendNumber(details.liveSize / s_kiloByte);
            page.append("</td><td>");
            page.appendNumber(details.decodedSize / s_kiloByte);
            page.append("</td></tr>\n");
        }
        else {
            page.append("<Resource URL=\"");
            page.append(url);
            page.append("\" InCache=\"");
            page.append(inCache ? "true" : "false");
            page.append("\" LoadStatus=\"");
            page.append(statusString);
            page.append("\" Size=\"");
            page.appendNumber(details.size);
            page.append("\" Living=\"");
            page.appendNumber(details.liveSize);
            page.append("\" Decoded=\"");
            page.appendNumber(details.decodedSize);
            page.append("\"/>\n");
        }
    }
    if (format == XML) {
        page.append("</");
        page.append(description);
        page.append(">\n");
    }
}

static void cachePage(AboutDataFormat format, StringBuilder & page)
{
    MemoryCache::Statistics cacheStat = memoryCache()->getStatistics();

    if (format == HTML) {
        page.append("<h1>WebKit Cache Statistics</h1><hr>");
        page.append("<div class=\"box\"><div class=\"box-title\">Cache Information<br><div style='font-size:11px;color:#A8A8A8'>Size, Living, and Decoded are expressed in KB.</div><br></div><table class='fixed-table'><col width=75%><col width=25%>");
        page.append("<tr> <th align=left>Item</th> <th align=left>Count</th> <th align=left>Size</th> <th align=left>Living</th> <th align=left>Decoded</th></tr>\n");
        cacheTypeStatisticToHTMLTr("Images", cacheStat.images, page);
        outputCacheDetails("Images", CachedResource::ImageResource, page, format);
        
        cacheTypeStatisticToHTMLTr("CSS", cacheStat.cssStyleSheets, page);
        outputCacheDetails("CSS", CachedResource::CSSStyleSheet, page, format);

        cacheTypeStatisticToHTMLTr("Scripts", cacheStat.scripts, page);
        outputCacheDetails("Scripts", CachedResource::Script, page, format);

#if ENABLE(XSLT)
        cacheTypeStatisticToHTMLTr("XSL", cacheStat.xslStyleSheets, page);
        outputCacheDetails("XSL", CachedResource::XSLStyleSheet, page, format);
#endif
        cacheTypeStatisticToHTMLTr("Fonts", cacheStat.fonts, page);
        outputCacheDetails("Fonts", CachedResource::FontResource, page, format);

        cacheTypeStatisticToHTMLTr("Total", memoryCache()->aggregateCacheStats(), page);
        page.append("</table></div><br>");
    }
    else {//xml
        page.append("<WebKitResourceCache>\n<Statistics>\n");
        cacheTypeStatisticToXmlTr("<Statistic Type=\"Images\"", cacheStat.images, page);
        cacheTypeStatisticToXmlTr("<Statistic Type=\"CSS\"", cacheStat.cssStyleSheets, page);
        cacheTypeStatisticToXmlTr("<Statistic Type=\"Scripts\"", cacheStat.scripts, page);
#if ENABLE(XSLT)
        cacheTypeStatisticToXmlTr("<Statistic Type=\"XSL\"", cacheStat.xslStyleSheets, page);
#endif
        cacheTypeStatisticToXmlTr("<Statistic Type=\"Fonts\"", cacheStat.fonts, page);
        cacheTypeStatisticToXmlTr("<Statistic Type=\"Total\"", memoryCache()->aggregateCacheStats(), page);
        page.append("</Statistics>\n<Resources>\n");

        outputCacheDetails("Images", CachedResource::ImageResource, page, format);
        outputCacheDetails("CSS", CachedResource::CSSStyleSheet, page, format);
        outputCacheDetails("Scripts", CachedResource::Script, page, format);
#if ENABLE(XSLT)
        outputCacheDetails("XSL", CachedResource::XSLStyleSheet, page, format);
#endif
        outputCacheDetails("Fonts", CachedResource::FontResource, page, format);

        page.append("</Resources>\n</WebKitResourceCache>\n");
    }
}

static void memoryPage(AboutDataFormat format, StringBuilder & page)
{
#if PLATFORM(HP)
    size_t consumed;
    size_t available;
    size_t highWater;
    size_t objectsAllocated;
    HPGetMemoryStats(&consumed, &available, &highWater, &objectsAllocated, NULL);
    if (format == HTML) {
        page.append("<h1>WebKit Memory Pool Statistics</h1><hr><table>");
        numberToHTMLTr("Consumed (bytes):", consumed, page);
        numberToHTMLTr("Consumed (MB):", (float)consumed/(1024.0*1024.0), page);
        numberToHTMLTr("Available (bytes):", available, page);
        numberToHTMLTr("Available (MB):", (float)available/(1024.0*1024.0), page);
        numberToHTMLTr("High Water (bytes):", highWater, page);
        numberToHTMLTr("High Water (MB):", (float)highWater/(1024.0*1024.0), page);
        numberToHTMLTr("Objects Allocated:", objectsAllocated, page);
        page.append("</table>");
    }
    else {//xml
        page.append("<WebKitMemory>\n<Statistics>\n");
        page.append("<Statistic Type=\"Consumed\" Bytes=\"");
        page.appendNumber(consumed);
        page.append("\" MB=\"");
        page.append(String::number((float)consumed/(1024.0*1024.0)));
        page.append("\"/>\n");
        page.append("<Statistic Type=\"Available\" Bytes=\"");
        page.appendNumber(available);
        page.append("\" MB=\"");
        page.append(String::number((float)available/(1024.0*1024.0)));
        page.append("\"/>\n");
        page.append("<Statistic Type=\"BytesHighWater\" Bytes=\"");
        page.appendNumber(highWater);
        page.append("\" MB=\"");
        page.append(String::number((float)highWater/(1024.0*1024.0)));
        page.append("\"/>\n");
        page.append("<Statistic Type=\"ObjectsAllocated\" Count=\"");
        page.appendNumber(objectsAllocated);
        page.append("\"/>\n");
        page.append("</Statistics>\n</WebKitMemory>\n");
    }
#endif
}

static void heapPage(AboutDataFormat format, StringBuilder & page)
{
    // JS engine memory usage.
    JSC::Heap& mainHeap = JSDOMWindow::commonVM()->heap;
    OwnPtr<JSC::TypeCountSet> objectTypeCounts = mainHeap.objectTypeCounts();
    OwnPtr<JSC::TypeCountSet> protectedObjectTypeCounts = mainHeap.protectedObjectTypeCounts();
    if (format == HTML) {
        page.append("<h1>JavaScript Heap Statistics</h1><hr>");
        page.append("<div class='box'><div class='box-title'>JS engine memory usage</div><table class='fixed-table'><col width=75%><col width=25%>");
        numberToHTMLTr("Main heap size (bytes):", mainHeap.size(), page);
        numberToHTMLTr("Main heap size (MB):", (float)mainHeap.size()/(1024.0*1024.0), page);
        numberToHTMLTr("Main heap capacity (bytes):", mainHeap.capacity(), page);
        numberToHTMLTr("Main heap capacity (MB):", (float)mainHeap.capacity()/(1024.0*1024.0), page);
        numberToHTMLTr("Object count:", mainHeap.objectCount(), page);
        numberToHTMLTr("Global object count:", mainHeap.globalObjectCount(), page);
        numberToHTMLTr("Protected object count:", mainHeap.protectedObjectCount(), page);
        numberToHTMLTr("Protected global object count:", mainHeap.protectedGlobalObjectCount(), page);
        page.append("</table></div><br>");
    }
    else {// xml
        page.append("<WebKitJavaScriptHeap>\n<Statistics>\n");
        numberToXMLTr("<Statistic Type=\"Main heap size\" Bytes=\"", mainHeap.size(), page);
        numberToXMLTr("<Statistic Type=\"Main heap capacity\" Bytes=\"", mainHeap.capacity(), page);
        page.append("</Statistics>\n<Objects>\n");
        numberToXMLTr("<Object Type=\"Object\" Count=\"", mainHeap.objectCount(), page);
        numberToXMLTr("<Object Type=\"Global object\" Count=\"", mainHeap.globalObjectCount(), page);
        numberToXMLTr("<Object Type=\"Protected object\" Count=\"", mainHeap.protectedObjectCount(), page);
        numberToXMLTr("<Object Type=\"Protected global object\" Count=\"", mainHeap.protectedGlobalObjectCount(), page);
        page.append("</Objects>\n</WebKitJavaScriptHeap>\n");
    }
}

static void webkitCountersPage(AboutDataFormat format, StringBuilder & page)
{
    if (format == HTML) {
        page.append("<h1>WebKit Counters</h1><hr>");
        page.append("<div class='box'><table class='fixed-table'><col width=75%><col width=25%>");
        numberToHTMLTr("Document count:   ", InspectorCounters::counterValue(InspectorCounters::DocumentCounter), page);
        numberToHTMLTr("Node count:   ", InspectorCounters::counterValue(InspectorCounters::NodeCounter), page);
        numberToHTMLTr("JS Event Listeners:   ", ThreadLocalInspectorCounters::current().counterValue(ThreadLocalInspectorCounters::JSEventListenerCounter), page);
        page.append("</table></div><br>");
    }
    else {// xml
        page.append("<WebKitCounters>\n<Statistics>\n");
        numberToXMLTr("<Statistic Type=\"Document Count\" Count=\"", InspectorCounters::counterValue(InspectorCounters::DocumentCounter), page);
        numberToXMLTr("<Statistic Type=\"Node Count\" Count=\"", InspectorCounters::counterValue(InspectorCounters::NodeCounter), page);
        numberToXMLTr("<Statistic Type=\"JS Event Listener Count\" Count=\"", ThreadLocalInspectorCounters::current().counterValue(ThreadLocalInspectorCounters::JSEventListenerCounter), page);
        page.append("</Statistics>\n</WebKitCounters>\n");
    }
}

static void appendSite(const String & prefixSite, const String & site, const String & description, AboutDataType aboutWhat, AboutDataType current, StringBuilder & output)
{
    if (aboutWhat != current)
        output.append("<a href=\"" + prefixSite + site + "\">" + description + "</a>   ");
    else
        output.append(description + "   ");
}

static void appendSites(String prefixSite, AboutDataType aboutWhat, StringBuilder & output)
{
    output.append("<hr><pre>");
    appendSite(prefixSite, "memory", "Memory", AboutDataMemory, aboutWhat, output);
    appendSite(prefixSite, "cache", "Resource Cache", AboutDataCache, aboutWhat, output);
    appendSite(prefixSite, "heap", "JavaScript Heap", AboutDataJSHeap, aboutWhat, output);
    appendSite(prefixSite, "counters", "WebKit Counters", AboutDataCounters, aboutWhat, output);
    appendSite(prefixSite, "all", "All Data", AboutDataAll, aboutWhat, output);
    output.append("</pre>");
}

String aboutData(String prefix, AboutDataType aboutWhat, AboutDataFormat format)
{
    StringBuilder output;
    if (aboutWhat == AboutDataMemory || aboutWhat == AboutDataAll)
        memoryPage(format, output);
    if (aboutWhat == AboutDataCache || aboutWhat == AboutDataAll)
        cachePage(format, output);
    if (aboutWhat == AboutDataJSHeap || aboutWhat == AboutDataAll)
        heapPage(format, output);
    if (aboutWhat == AboutDataCounters || aboutWhat == AboutDataAll)
        webkitCountersPage(format, output);
    if (format == HTML)
        appendSites(prefix, aboutWhat, output);
    return output.toString();
}
