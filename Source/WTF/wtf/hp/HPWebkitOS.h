/*
 * (C) Copyright 2010 Hewlett-Packard Development Company, L.P.
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

#ifndef _HP_WEBKIT_OS
#define _HP_WEBKIT_OS

#if PLATFORM(HP)

#include "HPCommonSystemOS/Diagnostics/HPTrace.h"

// Contains non-memory related definitions for methods in the
// HP OS layer (HP.Common.System.OS.dll).
// Memory related definitions are in HPWebkitMalloc.h
// Be sure to notify David Smith and update
// <HP SCM Root>\Source\HP\Mfp\App\Framework\ServiceLogs\MasterConfig\FirmwareErrorStatus.xlsx
// if any new usages of HPFatalError occur within WebKit.
// Currently these are the usages:
//  0x49DE01 - HP Memory Manager Pool Exhausted
//  0x49DE02 - General WebKit Assert Failure

#define HP_WEBKIT_FATAL_ERROR_BASE_CODE      (0x49de00ul)

#define HP_WEBKIT_FATAL_ERROR_MEMORYOUT      (HP_WEBKIT_FATAL_ERROR_BASE_CODE | 0x01)
#define HP_WEBKIT_FATAL_ERROR_ASSERT_FAILURE (HP_WEBKIT_FATAL_ERROR_BASE_CODE | 0x02)
#define HP_WEBKIT_FATAL_ERROR_UNKNOWN        (HP_WEBKIT_FATAL_ERROR_BASE_CODE | 0x03)
#define HP_WEBKIT_FATAL_ERROR_OVERFLOW       (HP_WEBKIT_FATAL_ERROR_BASE_CODE | 0x04)

void HPFatalError(unsigned long crashcode, const char * fileName, int lineNumber);
#define HP_FATAL_ERROR(code) HPFatalError(code, __FILE__, __LINE__);

// Custom HP webkit method that we can use to continue in the product 
// (because many assertions occur because we can't guarantee
//  thread IDs will be consistent between calls) but break
// in the standalone webkit development environment.
void HPWebkitAssertionFailed(const char* file, int line, const char* function, const char* assertion);
void HPWebkitArgumentAssertionFailed(const char* file, int line, const char* function, const char* argument, const char* assertion);

// This function will write to the HP log and will properly escape
// % characters in buffer.  This is important because the regular
// HP trace functions expect a format string to be passed.
#define HP_WEBKIT_TRACE_DEBUG 1
#define HP_WEBKIT_TRACE_INFO  2
#define HP_WEBKIT_TRACE_PERF  3
#define HP_WEBKIT_TRACE_WARN  4
#define HP_WEBKIT_TRACE_ERROR 5
#define HP_WEBKIT_TRACE_FATAL 6

void HPWriteToLogA(const char * buffer, int trace_level);
void HPWriteToLogW(const wchar_t * buffer, int trace_level);

extern HP::Common::System::OS::Diagnostics::HPTrace _TraceFactory;

// These methods are also available to write to the HP system main log file,
// but more care must be taken in using them - the format string must not
// contain unintended % characters.
#define HP_TRACE_DEBUG(...) _TraceFactory.WriteDebugV(__VA_ARGS__)
#define HP_TRACE_INFO(...)  _TraceFactory.WriteInfoV(__VA_ARGS__)
#define HP_TRACE_PERF(...)  _TraceFactory.WritePerfV(__VA_ARGS__)

//Currently WARN, ERROR and FATAL are all avaliable, but they
//  may be OFF.
#define HP_TRACE_WARN(...)  _TraceFactory.WriteWarnV(__VA_ARGS__)
#define HP_TRACE_ERROR(...) _TraceFactory.WriteErrorV(__VA_ARGS__)
#define HP_TRACE_FATAL(...) _TraceFactory.WriteFatalV(__VA_ARGS__)

#endif
#endif