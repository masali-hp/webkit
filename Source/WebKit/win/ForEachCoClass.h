/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef ForEachCoClass_h
#define ForEachCoClass_h

#include "ProgIDMacros.h"

#if USE(CF)
// These features have deep dependencies on CF currently.
#  define CF_DICTIONARY_PROPERTY_BAG(macro) macro(CFDictionaryPropertyBag)
#  define WEB_ARCHIVE(macro) macro(WebArchive)
#else
#  define CF_DICTIONARY_PROPERTY_BAG(macro)
#  define WEB_ARCHIVE(macro)
#endif

#if ENABLE(SQL_DATABASE)
#define WEB_DATABASE_MANAGER(macro) macro(WebDatabaseManager)
#else
#define WEB_DATABASE_MANAGER(macro)
#endif

#if ENABLE(GEOLOCATION)
#define WEB_GEOLOCATION(macro) macro(WebGeolocationPosition)
#else
#define WEB_GEOLOCATION(macro)
#endif

// Items may only be added to the end of this macro. No items may be removed from it.
#define FOR_EACH_COCLASS(macro) \
    CF_DICTIONARY_PROPERTY_BAG(macro) \
    macro(WebCache) \
    WEB_DATABASE_MANAGER(macro) \
    macro(WebDownload) \
    macro(WebError) \
    macro(WebHistory) \
    macro(WebHistoryItem) \
    macro(WebIconDatabase) \
    macro(WebJavaScriptCollector) \
    macro(WebKitStatistics) \
    macro(WebMutableURLRequest) \
    macro(WebNotificationCenter) \
    macro(WebPreferences) \
    macro(WebScrollBar) \
    macro(WebTextRenderer) \
    macro(WebURLCredential) \
    macro(WebURLProtectionSpace) \
    macro(WebURLRequest) \
    macro(WebURLResponse) \
    macro(WebView) \
    WEB_ARCHIVE(macro) \
    macro(WebCoreStatistics) \
    macro(WebCookieManager) \
    macro(WebWorkersPrivate) \
    macro(WebScriptWorld) \
    WEB_GEOLOCATION(macro) \
    macro(WebSerializedJSValue) \
    macro(WebUserContentURLPattern) \
    macro(WebNetworkConfiguration) \
    // end of macro

// Everything below this point is deprecated. Please do not use.

#define WEBKITCLASS_MEMBER(cls) cls##Class,
enum WebKitClass {
    FOR_EACH_COCLASS(WEBKITCLASS_MEMBER)
    WebKitClassSentinel
};
#undef WEBKITCLASS_MEMBER

#define OPENSOURCE_PROGID(cls) VERSION_INDEPENDENT_OPENSOURCE_PROGID(cls),
static LPCOLESTR openSourceProgIDs[WebKitClassSentinel] = {
    FOR_EACH_COCLASS(OPENSOURCE_PROGID)
};
#undef OPENSOURCE_PROGID

static LPCOLESTR* s_progIDs = openSourceProgIDs;

#define PROGID(className) progIDForClass(className##Class)

void setUseOpenSourceWebKit(bool);
LPCOLESTR progIDForClass(WebKitClass);

#endif // !defined(ForEachCoClass_h)
