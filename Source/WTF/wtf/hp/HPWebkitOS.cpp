/*
 * (C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of version 2.1 of the GNU Lesser General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to:
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */


#include <wtf/Platform.h>

#if PLATFORM(HP)
#include <windows.h>
#include <Diagnostics/HPTrace.h> // from HP.Common.System.OS.dll
#include <Diagnostics/ErrorHandling.h> // from HP.Common.System.OS.dll
#include <stdio.h>

#include <Diagnostics/HPTrace.h>
#include <hp/HPWebkitOS.h>

HP::Common::System::OS::Diagnostics::HPTrace _TraceFactory("WebKit");

void HPFatalError(DWORD crashcode, const char * fileName, int line)
{
    wchar_t wbuff[512];
    _snwprintf(wbuff, 512, L"WebKit crashing with fatal error %x at %S line %d\n", crashcode, fileName, line);
    wbuff[511] = 0;
    OutputDebugStringW(wbuff);
    HP_TRACE_FATAL("WebKit crashing with fatal error %x at %s line %d\n", crashcode, fileName, line);
	HP_TRACE_FATAL("Send this log and possible defect to HP Boise UI Team\n");
    HP::Common::System::OS::Diagnostics::DeviceFatalError(crashcode, fileName, line);
}

void HPWebkitAssertionFailed(const char* file, int line, const char* function, const char* assertion)
{
    wchar_t wbuff[256];
    _snwprintf(wbuff, 256, L"WebKit ASSERT FAILED %S\n", assertion);
    wbuff[255] = 0;
	OutputDebugStringW(wbuff);
    HPWriteToLogW(wbuff, HP_WEBKIT_TRACE_ERROR);
    _snwprintf(wbuff, 256, L"(%S:%d %S)\n", file, line, function);
    wbuff[255] = 0;
    OutputDebugStringW(wbuff);
    HPWriteToLogW(wbuff, HP_WEBKIT_TRACE_ERROR);

    // How do we detect if we're running under HP?
	// If we are not running under HP we want to assert here.
	// If we are running under HP we want to continue.
}

void HPWebkitArgumentAssertionFailed(const char* file, int line, const char* function, const char* argument, const char* assertion)
{
    wchar_t wbuff[256];
    _snwprintf(wbuff, 256, L"WebKit ASSERT ARGUMENT BAD: %S, %S\n", argument, assertion);
    wbuff[255] = 0;
	OutputDebugStringW(wbuff);
    HPWriteToLogW(wbuff, HP_WEBKIT_TRACE_ERROR);
    _snwprintf(wbuff, 256, L"(%S:%d %S)\n", file, line, function);
    wbuff[255] = 0;
    OutputDebugStringW(wbuff);
    HPWriteToLogW(wbuff, HP_WEBKIT_TRACE_ERROR);

    // How do we detect if we're running under HP?
	// If we are not running under HP we want to assert here.
	// If we are running under HP we want to continue.
}

void HPWriteToLogA(const char * buffer, int trace_level)
{
    char buff[2048];
    const char * ptr = buffer;
    int i = 0;
    for (; i < 2047 && *ptr != 0; i++)
    {
        buff[i] = *ptr++;
        // we need to escape '%' characters because HP_TRACE() is a type of printf function.
        if (buff[i] == '%')
            buff[++i] = '%';
    }
    buff[i] = '\0';
    switch (trace_level)
    {
    case HP_WEBKIT_TRACE_DEBUG: HP_TRACE_DEBUG(buff); break;
    case HP_WEBKIT_TRACE_INFO:  HP_TRACE_INFO(buff); break;
    case HP_WEBKIT_TRACE_PERF:  HP_TRACE_PERF(buff); break;
    case HP_WEBKIT_TRACE_WARN:  HP_TRACE_WARN(buff); break;
    case HP_WEBKIT_TRACE_ERROR: HP_TRACE_ERROR(buff); break;
    case HP_WEBKIT_TRACE_FATAL: HP_TRACE_FATAL(buff); break;
    }
}

void HPWriteToLogW(const wchar_t * buffer, int trace_level)
{
    char buff[2048];
    const wchar_t * wptr = buffer;
    int i = 0;
    for (; i < 2047 && *wptr != 0; i++)
    {
        buff[i] = (char) (*wptr++);
        // we need to escape '%' characters because HP_TRACE() is a type of printf function.
        if (buff[i] == '%')
            buff[++i] = '%';
    }
    buff[i] = '\0';
    switch (trace_level)
    {
    case HP_WEBKIT_TRACE_DEBUG: HP_TRACE_DEBUG(buff); break;
    case HP_WEBKIT_TRACE_INFO:  HP_TRACE_INFO(buff); break;
    case HP_WEBKIT_TRACE_PERF:  HP_TRACE_PERF(buff); break;
    case HP_WEBKIT_TRACE_WARN:  HP_TRACE_WARN(buff); break;
    case HP_WEBKIT_TRACE_ERROR: HP_TRACE_ERROR(buff); break;
    case HP_WEBKIT_TRACE_FATAL: HP_TRACE_FATAL(buff); break;
    }
}

#endif