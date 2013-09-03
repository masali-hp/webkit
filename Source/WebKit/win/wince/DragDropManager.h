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

#ifndef _WEBKIT_DRAG_DROP_MANAGER_H
#define _WEBKIT_DRAG_DROP_MANAGER_H

#define RegisterDragDrop WinCE_RegisterDragDrop
#define RevokeDragDrop   WinCE_RevokeDragDrop
#define DoDragDrop       WinCE_DoDragDrop

HRESULT WinCE_RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget);

HRESULT WinCE_RevokeDragDrop(HWND hwnd);

HRESULT WinCE_DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect);

void WinCE_SetDragImageForDataObject(SHDRAGIMAGE * dragImage, IDataObject * dataObject);

#endif
