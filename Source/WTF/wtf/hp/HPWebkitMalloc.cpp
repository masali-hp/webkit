/*
* Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
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

#include <config.h>

#if PLATFORM(HP)
#include <windows.h>

// from HP.Common.System.OS:
#include <Memory/JediMemoryManager.h>
#include <Diagnostics/HPTrace.h>

#include <wtf/hp/HPWebkitMalloc.h>
#include <wtf/hp/HPWebkitOS.h>
#include <wtf/FastMalloc.h>

#if ENABLE(MEMORY_OUT_HANDLING)
#include <wtf/MemoryOutManager.h>
#endif

#include <wtf/StringExtras.h>
#include <new>

using namespace HP::Common::System::OS::Memory;

void* operator new(size_t size)
{
    void *p = fastMalloc(size);
    return p;
}

void * operator new[](size_t size)
{
    void *p = fastMalloc(size);
    return p;
}

// We also need to to override the "no-throw" operator new, used
// by STL.  Normally it is implemented here:
// c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\crt\src\newopnt.cpp
// http://msdn.microsoft.com/en-us/magazine/cc164087.aspx
void * __CRTDECL operator new(size_t count, const std::nothrow_t&)
	_THROW0()
{
	void *p;
	_TRY_BEGIN
	p = operator new(count);
	_CATCH_ALL
	p = 0;
	_CATCH_END
	return (p);
}

void * __CRTDECL operator new[](::size_t count, const std::nothrow_t& x)
	_THROW0()
{	// try to allocate count bytes for an array
    return (operator new(count, x));
}

void operator delete(void* p)
{
    HPFree(p);
}

void operator delete[](void * p)
{
    HPFree(p);
}

#include "FastMallocPieces.cpp"

// These methods come from HP.Common.System.OS.dll.  They are
// implemented in StaticMemoryAPI.cpp and are not defined in
// a header file.
extern "C" {
    HRESULT MemoryManagerAlloc(void** pptr, ULONG size, ULONG pool, LifeSpan span, ULONG allocId);
    HRESULT MemoryManagerRealloc(void** pptr, ULONG size);
    HRESULT MemoryManagerFree(void* ptr);
    HRESULT MemoryAccountingGetPoolInfo(ULONG pool, PoolAccountingInfo* info);
    LONG MemoryManagerGetSize(const void * ptr);
}

static void HandleOutOfMemory(size_t bytes_requested, int line);

#ifdef HP_WEBKIT_MALLOC_DEBUG
static CRITICAL_SECTION mem_debug_mutex;
class MemDebugInitHelper {
public:
    MemDebugInitHelper() {
        InitializeCriticalSection(&mem_debug_mutex);
    }
};
static MemDebugInitHelper m;
class MemDebugMutexLocker {
public:
    MemDebugMutexLocker(CRITICAL_SECTION * ptr) : m_ptr(ptr) { EnterCriticalSection(m_ptr); }
    ~MemDebugMutexLocker() { LeaveCriticalSection(m_ptr); }
private:
    CRITICAL_SECTION * m_ptr;
};
#endif

static inline void * hp_try_malloc(size_t n, unsigned int alloc_id)
{
    void * result;
    if (n == 0)
        n = 1;
#if ENABLE(MEMORY_OUT_HANDLING)
    do {
#endif
        HRESULT hr = MemoryManagerAlloc(&result,
            n,
            HP::Common::System::OS::Memory::JediMemoryManager::PoolExtensibility,
            HP::Common::System::OS::Memory::LifespanIntermediate,
            alloc_id);
        if (SUCCEEDED(hr))
            return result;
#if ENABLE(MEMORY_OUT_HANDLING)
    } while ( WTF::MemoryOutManager::FreeMemory() );
#endif
    char buff[128];
    sprintf(buff, "hp_try_malloc, FAILED TO ALLOCATE %u bytes, alloc_id = %u.\n", n, alloc_id);
    HPOutputMemoryDebug(buff);
    return 0;
}

static inline void * hp_malloc(size_t n, unsigned int alloc_id)
{
    void * result = hp_try_malloc(n, alloc_id);
    if (!result)
        HandleOutOfMemory(n, __LINE__);
    return result;
}

static inline void * hp_try_realloc(void * oldBuffer, size_t newSize, unsigned int alloc_id)
{
    void * result = oldBuffer;
    if (newSize == 0)
        newSize = 1;
#if ENABLE(MEMORY_OUT_HANDLING)
    do {
#endif
        HRESULT hr = S_OK;
        if (oldBuffer)
            hr = MemoryManagerRealloc(&result, newSize);
        else
            hr = MemoryManagerAlloc(&result,
            newSize,
            HP::Common::System::OS::Memory::JediMemoryManager::PoolExtensibility,
            HP::Common::System::OS::Memory::LifespanIntermediate,
            alloc_id);
        if (SUCCEEDED(hr))
            return result;
#if ENABLE(MEMORY_OUT_HANDLING)
    } while ( WTF::MemoryOutManager::FreeMemory() );
#endif
    char buff[128];
    sprintf(buff, "hp_try_realloc, FAILED TO ALLOCATE %u bytes, alloc_id = %u.\n", newSize, alloc_id);
    HPOutputMemoryDebug(buff);
    return 0;
}

static inline void * hp_realloc(void * oldBuffer, size_t newSize, unsigned int alloc_id)
{
    void * result = hp_try_realloc(oldBuffer, newSize, alloc_id);
    if (!result)
        HandleOutOfMemory(newSize, __LINE__);
    return result;

}

static inline void hp_free(void * p)
{
    if (p)
        MemoryManagerFree(p);
}

static inline LONG hp_get_size(const void *p)
{
    return MemoryManagerGetSize(p);
}

//--------------------------------------------------------------------
// HPAlloc
//  Allocates the requested amount of memory.
//  If the requested amount of memory is not initially available,
//  MemoryOutManager::FreeMemory will be invoked to attempt to
//  free memory.  If this fails to free enough memory
//  then a fatal error will occur.
//--------------------------------------------------------------------
void * HPAlloc(size_t n, unsigned int alloc_id)
{
    return hp_malloc(n, alloc_id);
}

void * HPTryAlloc(size_t n, unsigned int alloc_id)
{
    return hp_try_malloc(n, alloc_id);
}

void * HPRealloc(void * oldBuffer, size_t newSize, unsigned int alloc_id)
{
    return hp_realloc(oldBuffer, newSize, alloc_id);
}

void * HPTryRealloc(void * oldBuffer, size_t newSize, unsigned int alloc_id)
{
    return hp_try_realloc(oldBuffer, newSize, alloc_id);
}

char * HPStrdup(const char *str, unsigned int alloc_id)
{
    if (!str)
        return NULL;
    size_t len = strlen(str) + 1;
    char * result = (char*) HPAlloc(len, alloc_id);
    strcpy(result, str);
    return result;
}

void HPFree(void * p)
{
    hp_free(p);
}

static HPMemoryDebugFunc memDebugMethod[5] = {0};
static int memDebugMethodCount = 0;
void HPRegisterMemoryDebug(HPMemoryDebugFunc method)
{
    for (int i = 0; i < memDebugMethodCount; i++){
        if (memDebugMethod[i] == method)
            return;
    }
    if (memDebugMethodCount == 5)
        CRASH_WITH_ERROR_CODE(0x49DE02);

    memDebugMethod[memDebugMethodCount++] = method;
}

void HPOutputMemoryDebug(const char * info)
{
#if OS(WINCE)
    wchar_t buffer[1024];
    size_t len = strlen(info);
    if (len > 1023)
        len = 1023;
    for (size_t i = 0; i < len; i++)
        buffer[i] = info[i];
    buffer[len] = 0;
    OutputDebugStringW(buffer);
#else
    OutputDebugStringA(info);
#endif
    HPWriteToLogA(info, HP_WEBKIT_TRACE_WARN);
}

void HPGetMemoryStats(size_t * consumed, size_t * available, size_t * highWater, size_t *objectsAllocated, size_t * largestFree)
{
    PoolAccountingInfo info;
    MemoryAccountingGetPoolInfo(HP::Common::System::OS::Memory::JediMemoryManager::PoolExtensibility, &info);
    if (consumed)
        *consumed = info.BytesConsumed;
    if (available)
        *available = info.BytesAvailable;
    if (highWater)
        *highWater = info.BytesHighWater;
    if (objectsAllocated)
        *objectsAllocated = info.ObjectsAllocated;
    if (largestFree)
        *largestFree = 0; // unknown
}

void HPOutputMemoryUsage(bool showPoolStats, bool showOther, HPMemoryOutputFunc func)
{
    char msg_buf[512];

    if (showPoolStats)
    {
        PoolAccountingInfo info;
        MemoryAccountingGetPoolInfo(HP::Common::System::OS::Memory::JediMemoryManager::PoolExtensibility, &info);

        snprintf(msg_buf, 512, "HP Extensibility Memory Pool Stats:\n      Bytes Consumed: %5.2f (MB) %u (Bytes)\n     Bytes Available: %5.2f (MB) %u (Bytes)\n    Bytes High Water: %5.2f (MB) %u (Bytes)\n   Objects Allocated: %u\n",
            (float) info.BytesConsumed / (1024.0 * 1024.0), info.BytesConsumed,
            (float) info.BytesAvailable / (1024.0 * 1024.0), info.BytesAvailable,
            (float) info.BytesHighWater / (1024.0 * 1024.0), info.BytesHighWater,
            info.ObjectsAllocated);

        func(msg_buf);
    }

    if (showOther)
    {
        for (int i = 0; i < memDebugMethodCount; i++)
        {
            memDebugMethod[i](func);
        }
    }
}

static void HandleOutOfMemory(size_t bytes_requested, int line)
{
    static bool handlingOutOfMemory = false;
    // We are being re-entered.  Unless we stop it now it could lead to
    // a stack overflow.
    if (handlingOutOfMemory)
    {
        return;
    }
    handlingOutOfMemory = true;

    HPOutputMemoryDebug("---------------------------------------------------\n");
    HPOutputMemoryDebug("FATAL ERROR - WebKit failed to allocate requested\n");
    HPOutputMemoryDebug("memory!!  This will result in a 49.DE01 error.\n");
    HPOutputMemoryDebug("Send this log to HP Boise UI team for analysis.\n");
    char buff[32];
    sprintf(buff, "HPWebkitMalloc.cpp, line %d\n", line);
    HPOutputMemoryDebug(buff);
    sprintf(buff, "Bytes Requested: %u\n", bytes_requested);
    HPOutputMemoryDebug(buff);

    HPOutputMemoryUsage(true, true, HPOutputMemoryDebug);

    handlingOutOfMemory = false;

    CRASH_WITH_ERROR_CODE(HP_WEBKIT_FATAL_ERROR_MEMORYOUT);
}

namespace WTF {

TryMallocReturnValue tryFastMalloc(size_t n)
{
    ASSERT(!isForbidden());
    return hp_try_malloc(n, WEBKIT_TRY_FAST_MALLOC_ALLOC_ID);
}

void* fastMalloc(size_t n)
{
    ASSERT(!isForbidden());
    return hp_malloc(n, WEBKIT_FAST_MALLOC_ALLOC_ID);
}

TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size)
{
    ASSERT(!isForbidden());

    size_t totalBytes = n_elements * element_size;
    void* result = hp_try_malloc(totalBytes, WEBKIT_TRY_FAST_CALLOC_ALLOC_ID);
    if (!result)
        return 0;

    memset(result, 0, totalBytes);
    return result;
}

void* fastCalloc(size_t n_elements, size_t element_size)
{
    ASSERT(!isForbidden());

    size_t totalBytes = n_elements * element_size;
    void* result = hp_malloc(totalBytes, WEBKIT_TRY_FAST_CALLOC_ALLOC_ID);
    if (!result)
        HandleOutOfMemory(totalBytes, __LINE__);

    memset(result, 0, totalBytes);
    return result;
}

void fastFree(void* p)
{
    ASSERT(!isForbidden());
    hp_free(p);
}

TryMallocReturnValue tryFastRealloc(void* p, size_t n)
{
    ASSERT(!isForbidden());
    return hp_try_realloc(p, n, WEBKIT_TRY_FAST_REALLOC_ALLOC_ID);
}

void* fastRealloc(void* p, size_t n)
{
    ASSERT(!isForbidden());
    return hp_realloc(p, n, WEBKIT_FAST_REALLOC_ALLOC_ID);
}

void releaseFastMallocFreeMemory() { }

FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

size_t fastMallocSize(const void* p)
{
#if ENABLE(WTF_MALLOC_VALIDATION)
    return Internal::fastMallocValidationHeader(const_cast<void*>(p))->m_size;
#else
    return hp_get_size(p);
#endif
}

} // namespace WTF

#endif