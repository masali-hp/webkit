/*
* (C) Copyright 2013 Hewlett-Packard Development Company, L.P.
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Library General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Library General Public License for more details.
*
*  You should have received a copy of the GNU Library General Public License
*  along with this library; see the file COPYING.LIB.  If not, write to
*  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
*  Boston, MA 02110-1301, USA.
*
*/

#ifndef MemoryOutManager_h
#define MemoryOutManager_h

#include <wtf/Platform.h>
#include <wtf/Vector.h>

namespace WTF {

    enum MemoryOutPhase {
        // Memory freed in this stage will degrade performance if 
        // cached resources are needed for future page loads.
        FreeInactiveCacheMemory, 

        // Memory freed in this stage will cause resources currently 
        // in use to be freed.  The page will not be rendered correctly 
        // but it should still load.
        FreeActiveCacheMemory
    };

    // Classes which are capable of freeing memory or taking other actions to
    // keep webkit running when the system is in a low memory state
    // should implement the MemoryOutClient interface.
    class MemoryOutClient
    {
    public:
        // The client will return true if it was able to take action (free memory)
        // for the requested phase.  It is extremely important that the client
        // does not attempt to allocate memory during this callback, as that allocation
        // is likely to fail.  If the client is able to free any memory, it should
        // free memory and then return true.  If the callback is invoked again
        // and no further memory can be freed it should return false.
        virtual bool FreeMemory(MemoryOutPhase phase) { return false; }

        // If calls to FreeMemory did not yield any new memory, then MemoryOutAbort 
        // will be invoked.  At this point the client should abort loading if possible.
        virtual void MemoryOutAbort() {}

        // After MemoryOutManager has entered an abort mode, loading has been
        // aborted, and we are now recovering, MemoryOutManager::Reset will be called.
        // MemoryOutManager will tell all clients to all reset to a normal state.
        virtual void MemoryOutReset() {}
    };

    class MemoryOutManager {
    private:
        MemoryOutManager();
    public:
        // Anyone can register as a memory client.  
        // It is important that any client that registers
        // will unregister before they are destroyed.
        static void RegisterMemoryClient(MemoryOutClient *);
        static void UnregisterMemoryClient(MemoryOutClient *);

        // If memory out callbacks have reached AbortLoading
        // then this will be true until Reset is called.
        static bool AbortReached();
        
        // This should be called when we run out of memory.
        static bool FreeMemory();

        // Resets MemoryOutManager and tells each client
        // to return to a regular operating state.
        static void Reset();

        // The size of the buffer that MemoryOutManager
        // will hold until the abort phase is reached.
        // Once the abort phase is reached it will release
        // this buffer with the intent that freeing this
        // memory will increase the chances of a clean
        // exit.
        static void SetAbortBufferSize(size_t size);

    private:
        static void init();

        static bool FreeMemory(MemoryOutPhase phase);
        static void AbortClients();
        
        static Vector<MemoryOutClient*> * s_clients;
        static bool s_abort_active;
        static unsigned int s_abort_buffer_size;
        static void * s_abort_buffer;
        static void * s_free_mem_buffer;
    };

}

#endif
