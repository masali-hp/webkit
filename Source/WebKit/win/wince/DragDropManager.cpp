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
// The other problem: since WinCE does not have support for layered windows, drag move operations
// with transparent images will only work dragging around inside the webview.

#include "config.h"
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>

#include "WebKit.h"
#include <wtf/Assertions.h>
#include <map>

#include "DragDropManager.h"

#include <cairo-win32.h>
#include <wtf/PerformanceTrace.h>

typedef std::map<HWND, IDropTarget*> DragWindowMap;
static DragWindowMap * dragMap = NULL;
static SHDRAGIMAGE currentDragImage;
static cairo_surface_t * dragImageCairo = NULL;

static bool dragImageVisible = false;
static bool dragAreaInvalid = false;
static RECT invalidDragRect = {-1};
static const unsigned short HIGH_BIT_MASK_SHORT = 0x8000;

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

static void drawDragImage(cairo_t *crScreen, POINT dest, POINT * prevLoc, IWebViewPrivate * webView, HWND targetWindow, bool drawDragImage, RECT * dirty)
{
    int width = currentDragImage.sizeDragImage.cx;
    int height = currentDragImage.sizeDragImage.cy;

    // If we are dragging over a webview we know how to get its
    // backing store and therefore how to draw over it.

    //1. Determine the union of where the drag image was and where it now is.
    SIZE unionImageSize = {width, height};
    POINT unionPoint = {dest.x, dest.y};
    if (prevLoc) {
        unionImageSize.cx += prevLoc->x > dest.x ? prevLoc->x - dest.x : dest.x - prevLoc->x;
        unionImageSize.cy += prevLoc->y > dest.y ? prevLoc->y - dest.y : dest.y - prevLoc->y;
        if (prevLoc->x < dest.x)
            unionPoint.x = prevLoc->x;
        if (prevLoc->y < dest.y)
            unionPoint.y = prevLoc->y;
    }
    RECT unionRect = {unionPoint.x, unionPoint.y, unionPoint.x + unionImageSize.cx, unionPoint.y + unionImageSize.cy};
    if (dirty) {
        // if there is a dirty rect, we may not need to paint at all.
        if (IntersectRect(&unionRect, dirty, &unionRect) == 0)
            return;
    }

    //2. Get the webview backing store, put it in a cairo surface.
    HBITMAP backingStore;
    HRESULT hr = webView->backingStore((OLE_HANDLE*) &backingStore);
    if (FAILED(hr))
        return;

    BITMAP info;
    if (GetObject(backingStore, sizeof(info), &info) == 0)
        return;

    ASSERT(info.bmBitsPixel == 32);
    cairo_surface_t* webviewImage = cairo_image_surface_create_for_data((unsigned char*)info.bmBits,
                                           CAIRO_FORMAT_ARGB32,
                                           info.bmWidth,
                                           info.bmHeight,
                                           info.bmWidthBytes);

    //3. Create a bitmap the size of the union.
    cairo_surface_t * unionImage = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32, unionImageSize.cx, unionImageSize.cy);

    //4. Copy the webview backing store into the bitmap.
    POINT webViewPoint = unionPoint;
    if (targetWindow)
        ::ScreenToClient(targetWindow, &webViewPoint);
    cairo_t *crUnionImage = cairo_create(unionImage);
    cairo_set_source_surface(crUnionImage, webviewImage, -webViewPoint.x, -webViewPoint.y);
    cairo_rectangle(crUnionImage, 0, 0, unionImageSize.cx, unionImageSize.cy);
    cairo_fill(crUnionImage);
    cairo_surface_destroy(webviewImage);

    //5. Alpha blend the drag image into the bitmap.
    if (drawDragImage) {
        POINT imgPosition = {dest.x - unionPoint.x, dest.y - unionPoint.y};
        cairo_set_source_surface(crUnionImage, dragImageCairo, imgPosition.x, imgPosition.y);
        cairo_rectangle(crUnionImage, imgPosition.x, imgPosition.y, width, height);
        cairo_fill(crUnionImage);
    }
    cairo_destroy(crUnionImage);

    //6. Draw the blended drag image onto the screen.
    cairo_set_source_surface(crScreen, unionImage, unionPoint.x, unionPoint.y);
    if (unionRect.left != unionPoint.x || unionRect.top != unionPoint.y || unionRect.right - unionRect.left != unionImageSize.cx || unionRect.bottom - unionRect.top != unionImageSize.cy) {
        cairo_rectangle(crScreen, unionRect.left, unionRect.top, unionRect.right - unionRect.left, unionRect.bottom - unionRect.top);
        cairo_clip(crScreen);
    }
    cairo_rectangle(crScreen, unionPoint.x, unionPoint.y, unionImageSize.cx, unionImageSize.cy);
    cairo_fill(crScreen);
    cairo_surface_destroy(unionImage);
}

HRESULT WinCE_DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect)
{
    MSG msg;
    POINT dest = {0,0};
    POINT lastDraw = {-1,-1};
    DWORD effect = DROPEFFECT_NONE;
    DWORD lastEffect = -1;
    IDropTarget * currentTarget = NULL;
    IWebViewPrivate * webView = NULL;
    IDataObject * dataObject = pDataObj;
    IDropSource * dropSource = pDropSource;
    HRESULT feedbackHR = DRAGDROP_S_USEDEFAULTCURSORS;
    HCURSOR startCursor = ::GetCursor();
    HCURSOR noCursor = LoadCursor(NULL, IDC_NO);

    // If there is a change in the keyboard or mouse button state, DoDragDrop calls
    // IDropSource::QueryContinueDrag and determines whether to continue the drag, to
    // drop the data, or to cancel the operation based on the return value.
    BOOL escapePressed = FALSE;
    DWORD keyState = MK_LBUTTON;
    if (GetKeyState(VK_CONTROL) & HIGH_BIT_MASK_SHORT)
        keyState |= MK_CONTROL;
    if (GetKeyState(VK_SHIFT) & HIGH_BIT_MASK_SHORT)
        keyState |= MK_SHIFT;

    HRESULT result = dropSource->QueryContinueDrag(escapePressed, keyState);
    bool dragActive = (result == S_OK);
    POINTL screenPoint;

    while (dragActive && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);

        // Filter all mouse and key events during the drag and drop:
        if (msg.message == WM_LBUTTONUP ||
            msg.message == WM_LBUTTONDOWN ||
            msg.message == WM_RBUTTONUP ||
            msg.message == WM_RBUTTONDOWN ||
            msg.message == WM_MOUSEMOVE ||
            msg.message == WM_KEYDOWN ||
            msg.message == WM_KEYUP ||
            msg.message == WM_CHAR ||
            msg.message == WM_SYSKEYDOWN ||
            msg.message == WM_SYSKEYUP ||
            msg.message == WM_SYSCHAR)
        {
            if (msg.message == WM_MOUSEMOVE) {
                PERFORMANCE_START(WTF::PerformanceTrace::InputEvent,"DragDropManager: Mouse Move");
            }
            else if (msg.message == WM_LBUTTONUP) {
                PERFORMANCE_START(WTF::PerformanceTrace::InputEvent,"DragDropManager: Mouse Up");
            }
            else {
                PERFORMANCE_START(WTF::PerformanceTrace::InputEvent,"DragDropManager: Other Input");
            }
            bool stateChanged = false;
            if (msg.message == WM_MOUSEMOVE ||
                msg.message == WM_LBUTTONUP) {

                POINT pt = {GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)};
                ::ClientToScreen(msg.hwnd, &pt);
                screenPoint.x = pt.x;
                screenPoint.y = pt.y;

                dest.x = pt.x - currentDragImage.ptOffset.x;
                dest.y = pt.y - currentDragImage.ptOffset.y;

                if (msg.message == WM_MOUSEMOVE) {

                    // Which window is my mouse over?
                    HWND mouseWnd = WindowFromPoint(dest);

                    IDropTarget * newTarget = NULL;
                    if (mouseWnd != NULL && dragMap) {
                        DragWindowMap::iterator it = dragMap->find(mouseWnd);
                        if (it != dragMap->end())
                            newTarget = it->second;
                    }
                    if (currentTarget && currentTarget != newTarget) {
                        currentTarget->DragLeave();
                        feedbackHR = dropSource->GiveFeedback(DROPEFFECT_NONE);
                    }
                    if (newTarget) {
                        effect = DROPEFFECT_NONE; //is this the correct way to set effect?
                        if (pdwEffect)
                            effect = *pdwEffect;
                        if (currentTarget != newTarget) {
                            newTarget->DragEnter(dataObject, keyState, screenPoint, &effect);
                            feedbackHR = dropSource->GiveFeedback(effect);
                            // Find out if the target is a webview now.
                            if (webView)
                                webView->Release();
                            newTarget->QueryInterface(IID_IWebViewPrivate, (void**) &webView);
                        }
                        else if (currentTarget) {
                            currentTarget->DragOver(keyState, screenPoint, &effect);
                            feedbackHR = dropSource->GiveFeedback(effect);
                        }
                    }
                    if (webView) {
                        dragImageVisible = true;
                        dragAreaInvalid = true;
                    }
                    if (feedbackHR == DRAGDROP_S_USEDEFAULTCURSORS && effect != lastEffect) {
                        switch (effect) {
                            case DROPEFFECT_NONE:
                                ::SetCursor(noCursor);
                                break;
                            /* MS doesn't give us these cursors!!!
                            case DROPEFFECT_COPY:
                                break;
                            case DROPEFFECT_MOVE:
                                break;
                            case DROPEFFECT_LINK:
                                break;
                                */
                            default:
                                ::SetCursor(startCursor);
                                break;
                        }
                        lastEffect = effect;
                    }
                    currentTarget = newTarget;
                }
                else if (msg.message == WM_LBUTTONUP) {
                    keyState = 0;
                    stateChanged = true;
                }
            }
            else if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_ESCAPE) {
                    escapePressed = TRUE;
                    stateChanged = true;
                }
            }
            else if (msg.message == WM_KEYUP) {
                if (msg.wParam == VK_ESCAPE) {
                    escapePressed = FALSE;
                    stateChanged = true;
                }
            }

            if (stateChanged) {
                result = dropSource->QueryContinueDrag(escapePressed, keyState);
                if (result != S_OK) {
                    dragImageVisible = false;
                    dragAreaInvalid = true;
                    if (currentTarget && effect && result == DRAGDROP_S_DROP) {
                        effect = DROPEFFECT_NONE; //FIXME: should we reset effect or not?
                        result = currentTarget->Drop(dataObject, keyState, screenPoint, &effect);
                        if (SUCCEEDED(result)) {
                            result = DRAGDROP_S_DROP;
                            if (pdwEffect)
                                *pdwEffect = effect;
                        }
                    }
                    else {
                        result = DRAGDROP_S_CANCEL;
                    }
                    dragActive = false;
                }
            }
            PERFORMANCE_END(WTF::PerformanceTrace::InputEvent);
        }
        else {
            DispatchMessage(&msg);
        }
        if (dragAreaInvalid && webView && dragImageCairo) {
            PERFORMANCE_START(WTF::PerformanceTrace::Paint, "DragDropManager::Paint");
            HDC screenDC = ::GetDC(NULL);
            cairo_surface_t * screenSurface = cairo_win32_surface_create(screenDC);
            cairo_t *crScreen = cairo_create(screenSurface);
            HWND wnd;
            webView->viewWindow((OLE_HANDLE*) &wnd);
            RECT dirty;
            if (invalidDragRect.left != -1) {
                dirty = invalidDragRect;
            }
            else {
                POINT p = {0, 0};
                ::ClientToScreen(wnd, &p);
                ::GetClientRect(wnd, &dirty);
                dirty.left += p.x;
                dirty.top += p.y;
                dirty.right += p.x;
                dirty.bottom += p.y;
            }
            if (dragImageVisible) {
                POINT * prevDraw = NULL;
                if (lastDraw.x != -1 && (lastDraw.x != dest.x || lastDraw.y != dest.y))
                    prevDraw = &lastDraw;
                drawDragImage(crScreen, dest, prevDraw, webView, wnd, true, &dirty);
                lastDraw = dest;
                invalidDragRect.left = -1;
            }
            else {
                // erase the drag image wherever we last put it:
                drawDragImage(crScreen, lastDraw, NULL, webView, wnd, false, &dirty);
            }
            cairo_destroy(crScreen);
            cairo_surface_destroy(screenSurface);
            ::ReleaseDC(NULL, screenDC);
            dragAreaInvalid = false;
            PERFORMANCE_END(WTF::PerformanceTrace::Paint);
        }
    }

    if (currentTarget && result != DRAGDROP_S_DROP) {
        currentTarget->DragLeave();
    }
    ::SetCursor(startCursor);

    if (webView)
        webView->Release();
    if (dragImageCairo) {
        cairo_surface_destroy(dragImageCairo);
        dragImageCairo = NULL;
    }
    return result;
}

// Method: SetDragImage
// Not part of Win32 API.
// Notes in http://msdn.microsoft.com/en-us/library/windows/desktop/bb762034(v=vs.85).aspx say:
//   The drag-and-drop helper object calls IDataObject::SetData to load private formats—used for
//   cross-process support—into the data object. It later retrieves these formats by calling
//   IDataObject::GetData. To support the drag-and-drop helper object, the data object's SetData
//   and GetData implementations must be able to accept and return arbitrary private formats.
// Because we don't know how the drag-and-drop helper is going to call SetData and GetData,
// and we don't need to support drag and drop across processes, we'll just manage this ourselves.
void WinCE_SetDragImage(SHDRAGIMAGE * dragImage)
{
    memcpy(&currentDragImage, dragImage, sizeof(SHDRAGIMAGE));

    BITMAP info;
    if (GetObject(currentDragImage.hbmpDragImage, sizeof(info), &info) == 0)
        return;

    unsigned char * bits = (unsigned char *) info.bmBits;
    // Windows flips bitmaps.  Flip it back.
    // See Top-Down vs. Bottom-Up DIBs
    // http://msdn.microsoft.com/en-us/library/windows/desktop/dd407212(v=vs.85).aspx
    if (info.bmHeight > 0) {
        LONG bmSize = info.bmWidthBytes * info.bmHeight;
        bits = (unsigned char *)fastMalloc(bmSize);
        for (int y = 0; y < info.bmHeight; y++) {
            unsigned char * dst = bits + (info.bmHeight - y - 1) * info.bmWidthBytes;
            unsigned char * src = (unsigned char *)info.bmBits + y * info.bmWidthBytes;
            memcpy(dst, src, info.bmWidthBytes);
        }
    }

    cairo_surface_t* imageSurface = cairo_image_surface_create_for_data((unsigned char*)bits,
        info.bmBitsPixel == 32 ? CAIRO_FORMAT_ARGB32 :
        info.bmBitsPixel == 24 ? CAIRO_FORMAT_RGB24 :
        info.bmBitsPixel == 16 ? CAIRO_FORMAT_RGB16_565 : CAIRO_FORMAT_INVALID,
        info.bmWidth,
        info.bmHeight > 0 ? info.bmHeight : -info.bmHeight,
        info.bmWidthBytes);

    dragImageCairo = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32, dragImage->sizeDragImage.cx, dragImage->sizeDragImage.cy);
    cairo_t *cr = cairo_create(dragImageCairo);

    cairo_set_source_surface(cr, imageSurface, 0, 0);
    cairo_paint_with_alpha(cr, 0.5);

    cairo_destroy(cr);
    cairo_surface_destroy(imageSurface);

    if (bits != info.bmBits)
        fastFree(bits);
}

void WinCE_PaintDragImage(HDC bitmapDC, IWebViewPrivate * webView, RECT * rect)
{
    if (!dragImageCairo || !dragImageVisible)
        return;
    dragAreaInvalid = true;
    POINT p = {rect->left, rect->top};
    HWND wnd;
    webView->viewWindow((OLE_HANDLE*) &wnd);
    ::ClientToScreen(wnd, &p);
    invalidDragRect.left = p.x;
    invalidDragRect.right = p.x + (rect->right - rect->left);
    invalidDragRect.top = p.y;
    invalidDragRect.bottom = p.y + (rect->bottom - rect->top);
}
