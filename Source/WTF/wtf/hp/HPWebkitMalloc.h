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

#ifndef _HP_WEBKIT_MALLOC_H
#define _HP_WEBKIT_MALLOC_H

#include <HPWebkitAllocIds.h>

#if PLATFORM(HP)

void * HPAlloc(size_t n, unsigned int alloc_id);
void * HPTryAlloc(size_t n, unsigned int alloc_id);
void * HPRealloc(void * oldBuffer, size_t newSize, unsigned int alloc_id);
void * HPTryRealloc(void * oldBuffer, size_t newSize, unsigned int alloc_id);
char * HPStrdup(const char *str, unsigned int alloc_id);
void HPFree(void * p);

typedef void(* HPMemoryOutputFunc)(const char * text);
typedef void(* HPMemoryDebugFunc)(HPMemoryOutputFunc);

void HPGetMemoryStats(size_t * consumed, size_t * available, size_t * highWater, size_t * objectsAllocated, size_t * largestFree);
void HPOutputMemoryDebug(const char * info);
void HPOutputMemoryUsage(bool showPoolStats, bool showOther, HPMemoryOutputFunc method);
void HPRegisterMemoryDebug(HPMemoryDebugFunc method);
#endif
#endif