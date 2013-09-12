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

#include "config.h"

#if ENABLE(MEMORY_OUT_HANDLING)
#include <MemoryOutManager.h>
#if PLATFORM(HP)
#include <HPWebkitMalloc.h>
#endif

namespace WTF {

// The free mem buffer is a buffer that is freed 
// to allow initial memory callbacks to process successfully.
// They should not need to allocate much memory.
#define FREE_MEM_BUFFER_SIZE 10240

Vector<MemoryOutClient*> * MemoryOutManager::s_clients = NULL;
bool MemoryOutManager::s_abort_active = false;
unsigned int MemoryOutManager::s_abort_buffer_size = 0;
void * MemoryOutManager::s_abort_buffer = NULL;
void * MemoryOutManager::s_free_mem_buffer = NULL;

void MemoryOutManager::init()
{
    if (!s_clients) {
        if (s_abort_buffer_size == 0) {
            SetAbortBufferSize(1024 * 1024); // 1MB
        }
        if (!s_free_mem_buffer)
            s_free_mem_buffer = fastMalloc(FREE_MEM_BUFFER_SIZE);
        s_clients = new Vector<MemoryOutClient*>();
    }
}

void MemoryOutManager::RegisterMemoryClient(MemoryOutClient * client)
{
    init();
    if (s_clients->find(client) != notFound)
        return;
    s_clients->append(client);
}

void MemoryOutManager::UnregisterMemoryClient(MemoryOutClient * client)
{
    size_t pos = s_clients->find(client);
    if (pos == notFound)
        return;
    s_clients->remove(pos);
}

bool MemoryOutManager::AbortReached()
{
    return s_abort_active;
}

bool MemoryOutManager::FreeMemory()
{
    bool result = false;

    static bool inFreeMemory = false;
    // If during memory out callbacks a client decides to allocate memory,
    // and that fails, we may end up with some recursion that will lead
    // to a stack overflow.
    if (inFreeMemory)
        return false;
    inFreeMemory = true;

    // Some of the FreeMemory callbacks (like Cache when it prunes resources) 
    // need memory to run correctly.  Free s_free_mem_buffer now and reallocate
    // it (if possible) after the FreeMemory callbacks have completed.
    if (s_free_mem_buffer) {
        fastFree(s_free_mem_buffer);
        s_free_mem_buffer = NULL;
    }

#if PLATFORM(HP)
    HPOutputMemoryDebug("WebKit: MemoryOutManager::FreeMemory(), attempting to recover from memory out.\n");
#endif

    result = FreeMemory(FreeInactiveCacheMemory);

    if (!result)
    {
        result = FreeMemory(FreeActiveCacheMemory);
    }

    tryFastMalloc(FREE_MEM_BUFFER_SIZE).getValue(s_free_mem_buffer);

    if (!result && !s_abort_active)
    {
        result = true;
        s_abort_active = true;

#if PLATFORM(HP)
        HPOutputMemoryDebug("=======================================================\n");
        HPOutputMemoryDebug("WebKit: MemoryOutManager.cpp, freeing emergency buffer,\n");
        HPOutputMemoryDebug("aborting currently loaded page(s). \n");
#endif

        fastFree(s_abort_buffer);
        s_abort_buffer = NULL;

#if PLATFORM(HP)
        HPOutputMemoryDebug("Memory Usage: \n");
        HPOutputMemoryUsage(true, true, HPOutputMemoryDebug);
        HPOutputMemoryDebug("=======================================================\n");
#endif

        for (Vector<MemoryOutClient *>::iterator itr = s_clients->begin();
             itr < s_clients->end();
             itr++)
        {
            (*itr)->MemoryOutAbort();
        }
    }
    inFreeMemory = false;
    return result;
}

// Tells each client to return to a regular operating state
void MemoryOutManager::Reset()
{
    init();
    s_abort_active = false;
    SetAbortBufferSize(s_abort_buffer_size);
    if (!s_free_mem_buffer)
        s_free_mem_buffer = fastMalloc(FREE_MEM_BUFFER_SIZE);
    for (Vector<MemoryOutClient *>::iterator itr = s_clients->begin();
        itr < s_clients->end();
        itr++)
    {
        (*itr)->MemoryOutReset();
    }
}

void MemoryOutManager::SetAbortBufferSize(size_t size)
{
    if (size != s_abort_buffer_size
        && s_abort_buffer != NULL)
    {
        fastFree(s_abort_buffer);
        s_abort_buffer = NULL;
    }

    s_abort_buffer_size = size;

    if (s_abort_buffer == NULL)
    {
        s_abort_buffer = (void*) fastMalloc(s_abort_buffer_size);
    }
}

//private methods:

bool MemoryOutManager::FreeMemory(MemoryOutPhase phase)
{
    bool result = false;
    for (Vector<MemoryOutClient *>::iterator itr = s_clients->begin();
        itr < s_clients->end();
        itr++)
    {
        if ((*itr)->FreeMemory(phase))
        {
            result = true;
            break;
        }
    }
    return result;
}


}
#endif // #if ENABLE(MEMORY_OUT_HANDLING)
