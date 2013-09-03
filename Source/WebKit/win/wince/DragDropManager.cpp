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

// Windows CE does not have the methods RegisterDragDrop, RevokeDragDrop, DoDragDrop.
// In order to support drag and drop on Windows CE we have to implement these functions ourselves.
// The clear downside is that drag and drop will only work inside this browser process.

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>

#include <map>

#include "DragDropManager.h"

#include <cairo-win32.h>

typedef std::map<HWND, IDropTarget*> DragWindowMap;

DragWindowMap * dragMap = NULL;
SHDRAGIMAGE currentDragImage;
cairo_surface_t * dragImageCairo = NULL;
IDataObject * currentDataObject;

HRESULT WinCE_RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget) {
    pDropTarget->AddRef();
    if (!dragMap)
        dragMap = new DragWindowMap();
    dragMap->insert ( std::pair<HWND, IDropTarget*>(hwnd, pDropTarget) );
    return S_OK;
}

HRESULT WinCE_RevokeDragDrop(HWND hwnd) {
    if (dragMap) {
        DragWindowMap::iterator it = dragMap->find (hwnd);
        if (it != dragMap->end()) {
            it->second->Release();
            dragMap->erase(it);
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT WinCE_DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect)
{
    MSG msg;
    cairo_surface_t * backupImage = NULL;
    cairo_surface_t * oldBackupImage = NULL;
    cairo_surface_t * unionImage = NULL;
    SIZE unionImageSize = {0,0};
    POINT prevDest;
    POINT dest = {0,0};
    HWND currentWnd = NULL;
    HRESULT result = DRAGDROP_S_CANCEL;
    DWORD effect = 0;
    IDropTarget * currentTarget = NULL;
    bool dragActive = true;

    while (dragActive && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);

        if (msg.message == WM_MOUSEMOVE ||
            msg.message == WM_LBUTTONUP) {

            HDC screenDC = ::GetDC(NULL);

            POINT pt = {GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)};
            ::ClientToScreen(msg.hwnd, &pt);
            POINTL screenPoint = {pt.x, pt.y};

            dest.x = pt.x - currentDragImage.ptOffset.x;
            dest.y = pt.y - currentDragImage.ptOffset.y;

            int width = currentDragImage.sizeDragImage.cx;
            int height = currentDragImage.sizeDragImage.cy;

            //1. Create a screen surface.
            cairo_surface_t * screenSurface = cairo_win32_surface_create(screenDC);
            cairo_t *crScreen = cairo_create(screenSurface);
            
            if (msg.message == WM_MOUSEMOVE) {

                //2. Create a surface (backup) where the drag image will be blitted.
                //   To avoid unnecessary surface creation / destruction, start swapping the images
                //   once we've created both a backup image and old backup image.
                if (!backupImage)
                    backupImage = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_RGB24, width, height);
                else if (!oldBackupImage) {
                    oldBackupImage = backupImage;
                    backupImage = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_RGB24, width, height);
                }
                else {
                    cairo_surface_t * temp = oldBackupImage;
                    oldBackupImage = backupImage;
                    backupImage = temp;
                }

                //3. Copy the screen to the backup.
                //   - copy the screen first
                cairo_t *crBackupImage = cairo_create(backupImage);
                cairo_set_source_surface(crBackupImage, screenSurface, -dest.x, -dest.y);
                cairo_rectangle (crBackupImage, 0, 0, width, height);
                cairo_fill (crBackupImage);

                //   - now paint the old backup image into this backup image to erase the drag image from our current backup
                if (oldBackupImage) {
                    cairo_set_source_surface(crBackupImage, oldBackupImage, prevDest.x - dest.x, prevDest.y - dest.y);
                    cairo_rectangle(crBackupImage, prevDest.x - dest.x, prevDest.y - dest.y, width, height);
                    cairo_fill(crBackupImage);
                }
                cairo_destroy(crBackupImage);

                //4. Create a surface that is the union (in coordinates) of where the
                //   drag image was and where it will be.
                int unionWidth = width;
                int unionHeight = height;
                int screenSrcX = dest.x;
                int screenSrcY = dest.y;
                int oldBackupX = 0;
                int oldBackupY = 0;
                int imageDestX = 0;
                int imageDestY = 0;
                if (oldBackupImage) {
                    if (dest.x > prevDest.x) {
                        imageDestX = dest.x - prevDest.x;
                        unionWidth += imageDestX;
                        screenSrcX = prevDest.x;
                    }
                    else {
                        oldBackupX = prevDest.x - dest.x;
                        unionWidth += oldBackupX;
                    }
                    if (dest.y > prevDest.y) {
                        imageDestY = dest.y - prevDest.y;
                        unionHeight += imageDestY;
                        screenSrcY = prevDest.y;
                    }
                    else {
                        oldBackupY = prevDest.y - dest.y;
                        unionHeight += oldBackupY;
                    }
                }

                if (!unionImage || (unionWidth > unionImageSize.cx || unionHeight > unionImageSize.cy)) {
                    if (unionImage)
                        cairo_surface_destroy(unionImage);
                    unionImage = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_RGB24, unionWidth, unionHeight);
                    unionImageSize.cx = unionWidth;
                    unionImageSize.cy = unionHeight;
                }

                //5. Copy the screen into the union.
                cairo_t *crUnionImage = cairo_create(unionImage);
                cairo_set_source_surface(crUnionImage, screenSurface, -screenSrcX, -screenSrcY);
                cairo_rectangle(crUnionImage, 0, 0, unionWidth, unionHeight);
                cairo_fill(crUnionImage);

                //6. Copy an old backup, if it exists, into the union.
                if (oldBackupImage) {
                    cairo_set_source_surface(crUnionImage, oldBackupImage, oldBackupX, oldBackupY);
                    cairo_rectangle(crUnionImage, oldBackupX, oldBackupY, width, height);
                    cairo_fill(crUnionImage);
                }

                //7. AlphaBlend the drag image into the union.
                cairo_set_source_surface(crUnionImage, dragImageCairo, imageDestX, imageDestY);
                cairo_rectangle(crUnionImage, imageDestX, imageDestY, width, height);
                cairo_fill(crUnionImage);

                //8. Copy the union to the screen.
                cairo_set_source_surface(crScreen, unionImage, screenSrcX, screenSrcY);
                cairo_rectangle(crScreen, screenSrcX, screenSrcY, unionWidth, unionHeight);
                cairo_fill(crScreen);
              
                cairo_destroy(crUnionImage);

                // which window is my mouse over?
                HWND mouseWnd = WindowFromPoint(dest);

                IDropTarget * newTarget = NULL;
                if (mouseWnd != NULL && dragMap) {
                    DragWindowMap::iterator it = dragMap->find(mouseWnd);
                    if (it != dragMap->end())
                        newTarget = it->second;
                }
                if (newTarget) {
                    DWORD keyState = 0; // FIXME: check for shift, alt, etc.
                    effect = 0;
                    if (pdwEffect)
                        effect = *pdwEffect;
                    if (currentTarget != newTarget) {
                        if (currentTarget)
                            currentTarget->DragLeave();
                        if (newTarget)
                            newTarget->DragEnter(currentDataObject, keyState, screenPoint, &effect);
                    }
                    else if (currentTarget) {
                        currentTarget->DragOver(keyState, screenPoint, &effect);
                    }
                }
                else if (currentTarget) {
                    currentTarget->DragLeave();
                }
                
                currentTarget = newTarget;
                prevDest = dest;
            }
            else if (msg.message == WM_LBUTTONUP) {
                // erase the drag image wherever we last put it:
                cairo_set_source_surface(crScreen, backupImage, prevDest.x, prevDest.y);
                cairo_rectangle(crScreen, prevDest.x, prevDest.y, width, height);
                cairo_fill(crScreen);

                if (currentTarget) {
                    DWORD keyState = 0; // FIXME: check for shift, alt, etc.
                    effect = 0;
                    if (pdwEffect)
                        effect = *pdwEffect;
                    HRESULT hr = currentTarget->Drop(currentDataObject, keyState, screenPoint, &effect);
                    if (SUCCEEDED(hr))
                        result = DRAGDROP_S_DROP;
                }
                dragActive = false;
            }

            cairo_destroy(crScreen);
            cairo_surface_destroy(screenSurface);
            ::ReleaseDC(NULL, screenDC);

        }
        else {
            DispatchMessage(&msg);
        }
    }

    if (pdwEffect)
        *pdwEffect = effect;
    if (oldBackupImage) {
        cairo_surface_destroy(oldBackupImage);
        oldBackupImage = NULL;
    }
    if (backupImage) {
        cairo_surface_destroy(backupImage);
        backupImage = NULL;
    }
    if (dragImageCairo) {
        cairo_surface_destroy(dragImageCairo);
        dragImageCairo = NULL;
    }
    if (unionImage) {
        cairo_surface_destroy(unionImage);
        unionImage = NULL;
    }
    return result;
}

// Method: SetDragImageForDataObject
// Not part of Win32 API.
// Notes in http://msdn.microsoft.com/en-us/library/windows/desktop/bb762034(v=vs.85).aspx say:
//   The drag-and-drop helper object calls IDataObject::SetData to load private formats—used for
//   cross-process support—into the data object. It later retrieves these formats by calling
//   IDataObject::GetData. To support the drag-and-drop helper object, the data object's SetData
//   and GetData implementations must be able to accept and return arbitrary private formats.
// Because we don't know how the drag-and-drop helper is going to call SetData and GetData,
// and we don't need to support drag and drop across processes, we'll just manage this ourselves.
void WinCE_SetDragImageForDataObject(SHDRAGIMAGE * dragImage, IDataObject * dataObject)
{
    currentDataObject = dataObject; // this is dangerous because we're not changing the ref count.
    memcpy(&currentDragImage, dragImage, sizeof(SHDRAGIMAGE));

    HDC screenDC = ::GetDC(NULL);
    HDC bitmapDC = ::CreateCompatibleDC(screenDC);
    HGDIOBJ oldBitmap = ::SelectObject(bitmapDC, currentDragImage.hbmpDragImage);
    cairo_surface_t * imageSurface = cairo_win32_surface_create(bitmapDC);

    dragImageCairo = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32, dragImage->sizeDragImage.cx, dragImage->sizeDragImage.cy);
    cairo_t *cr = cairo_create(dragImageCairo);

    cairo_set_source_surface(cr, imageSurface, 0, 0);
    cairo_paint_with_alpha(cr, 0.5);

    cairo_destroy(cr);
    cairo_surface_destroy(imageSurface);

    ::SelectObject(bitmapDC, oldBitmap);
    ::DeleteDC(bitmapDC);
    ::ReleaseDC(NULL, screenDC);
}
