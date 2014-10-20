/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "resource.h"
#include <WebKit/WebKit.h>

class WinLauncherWebHost
    : public IWebFrameLoadDelegate
    , public IWebNetworkDebugDelegate
    , public IWebUIDelegate
    , public IWebUIDelegatePrivate
{
public:
    WinLauncherWebHost() : m_refCount(1) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebFrameLoadDelegate
    virtual HRESULT STDMETHODCALLTYPE didStartProvisionalLoadForFrame(
        /* [in] */ IWebView* webView,
        /* [in] */ IWebFrame* /*frame*/) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didReceiveServerRedirectForProvisionalLoadForFrame(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didFailProvisionalLoadWithError(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame*);

    virtual HRESULT STDMETHODCALLTYPE didCommitLoadForFrame(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return updateAddressBar(webView); }

    virtual HRESULT STDMETHODCALLTYPE didReceiveTitle(
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR title,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didChangeIcons(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didReceiveIcon(
        /* [in] */ IWebView *webView,
        /* [in] */ OLE_HANDLE hBitmap,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didFinishLoadForFrame(
        /* [in] */ IWebView* webView,
        /* [in] */ IWebFrame* /*frame*/);

    virtual HRESULT STDMETHODCALLTYPE didFailLoadWithError(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame *forFrame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didChangeLocationWithinPageForFrame(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE willPerformClientRedirectToURL(
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR url,
        /* [in] */ double delaySeconds,
        /* [in] */ DATE fireDate,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE didCancelClientRedirectForFrame(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE willCloseFrame(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE windowScriptObjectAvailable(
        /* [in] */ IWebView *webView,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowScriptObject)  { return S_OK; }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE didClearWindowObject(
        /* [in] */ IWebView *webView,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowScriptObject,
        /* [in] */ IWebFrame *frame) { return S_OK; }

    // IWebNetworkDebugDelegate
    virtual HRESULT STDMETHODCALLTYPE jobStarted(
        /* [in] */ int jobId,
        BSTR url,
        BOOL synchronous);

    virtual HRESULT STDMETHODCALLTYPE jobDataComplete(
        /* [in] */ int jobId,
        /* [in] */ int httpCode);

    virtual HRESULT STDMETHODCALLTYPE jobFinished(
        /* [in] */ int jobId,
        /* [in] */ BOOL success,
        /* [in] */ int errorCode);

    virtual HRESULT STDMETHODCALLTYPE jobDebug(
        /* [in] */ int jobId,
        /* [in] */ DebugInformation debugType,
        /* [in] */ unsigned char *data,
        /* [in] */ int totalSize);

    // IWebUIDelegate
    virtual HRESULT STDMETHODCALLTYPE createWebViewWithRequest(IWebView*, IWebURLRequest*, IWebView**) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewShow(IWebView*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewClose(IWebView*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewFocus(IWebView*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewUnfocus(IWebView*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewFirstResponder(IWebView*, OLE_HANDLE*)  { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE makeFirstResponder(IWebView*, OLE_HANDLE) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setStatusText(IWebView*, BSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewStatusText(IWebView*, BSTR*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewAreToolbarsVisible(IWebView*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setToolbarsVisible(IWebView*, BOOL) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewIsStatusBarVisible(IWebView*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setStatusBarVisible(IWebView*, BOOL) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewIsResizable(IWebView*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setResizable(IWebView*, BOOL) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setFrame(IWebView*, RECT*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewFrame(IWebView*, RECT*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setContentRect(IWebView*, RECT*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewContentRect(IWebView*, RECT*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runJavaScriptAlertPanelWithMessage(IWebView*, BSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runJavaScriptConfirmPanelWithMessage(IWebView*, BSTR, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runJavaScriptTextInputPanelWithPrompt(IWebView*, BSTR, BSTR, BSTR*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runBeforeUnloadConfirmPanelWithMessage(IWebView*, BSTR, IWebFrame*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runOpenPanelForFileButtonWithResultListener(IWebView*, IWebOpenPanelResultListener*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE mouseDidMoveOverElement(IWebView*, IPropertyBag*, UINT) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE contextMenuItemsForElement(IWebView*, IPropertyBag*, OLE_HANDLE, OLE_HANDLE*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE validateUserInterfaceItem(IWebView*, UINT, BOOL, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE shouldPerformAction(IWebView*, UINT, UINT) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE dragDestinationActionMaskForDraggingInfo(IWebView*, IDataObject*, WebDragDestinationAction*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE willPerformDragDestinationAction(IWebView*, WebDragDestinationAction, IDataObject*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE dragSourceActionMaskForPoint(IWebView*, LPPOINT, WebDragSourceAction*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE willPerformDragSourceAction(IWebView*, WebDragSourceAction, LPPOINT, IDataObject*, IDataObject**) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE contextMenuItemSelected(IWebView*, void*, IPropertyBag*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE hasCustomMenuImplementation(BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE trackCustomPopupMenu(IWebView*, OLE_HANDLE, LPPOINT) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE measureCustomMenuItem(IWebView*, void*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE drawCustomMenuItem(IWebView*, void*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE addCustomMenuDrawingData(IWebView*, OLE_HANDLE) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE cleanUpCustomMenuDrawingData(IWebView*, OLE_HANDLE) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE canTakeFocus(IWebView*, BOOL, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE takeFocus(IWebView*, BOOL) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE registerUndoWithTarget(IWebUndoTarget*, BSTR, IUnknown*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE removeAllActionsWithTarget(IWebUndoTarget*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setActionTitle(BSTR) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE undo() { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE redo() { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE canUndo(BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE canRedo(BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE printFrame(IWebView*, IWebFrame *) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE ftpDirectoryTemplatePath(IWebView*, BSTR*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewHeaderHeight(IWebView*, float*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewFooterHeight(IWebView*, float*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE drawHeaderInRect(IWebView*, RECT*, OLE_HANDLE) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE drawFooterInRect(IWebView*, RECT*, OLE_HANDLE, UINT, UINT) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE webViewPrintingMarginRect(IWebView*, RECT*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE canRunModal(IWebView*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE createModalDialog(IWebView*, IWebURLRequest*, IWebView**) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runModal(IWebView*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE isMenuBarVisible(IWebView*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE setMenuBarVisible(IWebView*, BOOL) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE runDatabaseSizeLimitPrompt(IWebView*, BSTR, IWebFrame*, BOOL*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE paintCustomScrollbar(IWebView*, HDC, RECT, WebScrollBarControlSize, WebScrollbarControlState, WebScrollbarControlPart, BOOL, float, float, WebScrollbarControlPartMask) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE paintCustomScrollCorner(IWebView*, HDC, RECT) { return E_NOTIMPL; }

    // IWebUIDelegatePrivate
    virtual HRESULT STDMETHODCALLTYPE unused1( void) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE unused2( void) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewScrolled(
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewAddMessageToConsole(
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message,
        /* [in] */ int lineNumber,
        /* [in] */ BSTR url,
        /* [in] */ WebMessageSource source,
        /* [in] */ WebMessageLevel level);

    virtual HRESULT STDMETHODCALLTYPE webViewShouldInterruptJavaScript(
            /* [in] */ IWebView *sender,
            /* [retval][out] */ BOOL *result) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewReceivedFocus(
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewLostFocus(
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE loseFocusToHWnd) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE doDragDrop(
        /* [in] */ IWebView *sender,
        /* [in] */ IDataObject *dataObject,
        /* [in] */ IDropSource *dropSource,
        /* [in] */ DWORD okEffect,
        /* [retval][out] */ DWORD *performedEffect) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewGetDlgCode(
        /* [in] */ IWebView *sender,
        /* [in] */ UINT keyCode,
        /* [retval][out] */ LONG_PTR *code) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewPainted(
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE exceededDatabaseQuota(
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame,
        /* [in] */ IWebSecurityOrigin *origin,
        /* [in] */ BSTR databaseIdentifier) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE embeddedViewWithArguments(
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame,
        /* [in] */ IPropertyBag *arguments,
        /* [retval][out] */ IWebEmbeddedView **view) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE unused3( void) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewClosing(
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewSetCursor(
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE cursor) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewDidInvalidate(
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE focusedNodeChanged(
            /* [in] */ IWebView *sender,
            /* [in] */ BOOL formElement,
            /* [in] */ IDOMElement *focusedNode) { return E_NOTIMPL; }

    // WinLauncherWebHost

protected:
    HRESULT updateAddressBar(IWebView* webView);

protected:
    ULONG                   m_refCount;
};
