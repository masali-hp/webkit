/*
 * Copyright (C) 2006, 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009, 2011 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009, 2010, 2011 Appcelerator, Inc. All rights reserved.
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

#include "stdafx.h"

#include <wtf/Platform.h>
#include "WinLauncher.h"

#include "DOMDefaultImpl.h"
#include "PrintWebUIDelegate.h"
#include <WebKit/WebKitCOMAPI.h>

#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <string>
#include <wininet.h>

#if PLATFORM(HP)
#include "HPWebKitMemoryDebug.h"
static void DumpMemoryAllocations(bool logAllocations);
#endif

namespace WTF {
__declspec(dllimport) double currentTime();
}

#ifdef _WIN32_WCE
#define SetWindowLongPtr(x, y, z) SetWindowLong(x, y, z)
#define GetWindowLongPtr(x, y) GetWindowLong(x, y)
#define LONG_PTR LONG
#define RegisterClassEx(x) RegisterClass(x)
#endif

#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#endif

#define MAX_LOADSTRING 100
#define URLBAR_HEIGHT  24

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hMainWnd;
HWND hURLBarWnd;
WNDPROC DefEditProc = 0;
WNDPROC DefWebKitProc = 0;
IWebView* gWebView = 0;
IWebViewPrivate* gWebViewPrivate = 0;
HWND gViewWindow = 0;
WinLauncherWebHost* gWebHost = 0;
PrintWebUIDelegate* gPrintDelegate = 0;
TCHAR szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Support moving the transparent window
POINT s_windowPosition = { 100, 100 };
SIZE s_windowSize = { 800, 400 };
bool s_usesLayeredWebView = false;
bool s_fullDesktop = false;
BSTR storagePath = NULL;
BSTR inspectorURL = NULL;
BSTR inspectorServer = NULL;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    MyEditProc(HWND, UINT, WPARAM, LPARAM);

static void loadURL(BSTR urlBStr);

#if PLATFORM(HP)
void LoadIniSettings(const char * inipath);
#endif

static bool usesLayeredWebView()
{
    return s_usesLayeredWebView;
}

static bool shouldUseFullDesktop()
{
    return s_fullDesktop;
}

class SimpleEventListener : public DOMEventListener {
public:
    SimpleEventListener(LPWSTR type)
    {
        wcsncpy_s(m_eventType, 100, type, 100);
        m_eventType[99] = 0;
    }

    virtual HRESULT STDMETHODCALLTYPE handleEvent(IDOMEvent* evt)
    {
        wchar_t message[255];
        wcscpy_s(message, 255, m_eventType);
        wcscat_s(message, 255, L" event fired!");
        ::MessageBox(0, message, L"Event Handler", MB_OK);
        return S_OK;
    }

private:
    wchar_t m_eventType[100];
};

HRESULT WinLauncherWebHost::updateAddressBar(IWebView* webView)
{
    IWebFrame* mainFrame = 0;
    IWebDataSource* dataSource = 0;
    IWebMutableURLRequest* request = 0;
    BSTR frameURL = 0;

    HRESULT hr = S_OK;

    hr = webView->mainFrame(&mainFrame);
    if (FAILED(hr))
        goto exit;

    hr = mainFrame->dataSource(&dataSource);
    if (FAILED(hr) || !dataSource)
        hr = mainFrame->provisionalDataSource(&dataSource);
    if (FAILED(hr) || !dataSource)
        goto exit;

    hr = dataSource->request(&request);
    if (FAILED(hr) || !request)
        goto exit;

    hr = request->mainDocumentURL(&frameURL);
    if (FAILED(hr))
        goto exit;

    SendMessage(hURLBarWnd, (UINT)WM_SETTEXT, 0, (LPARAM)frameURL);

exit:
    if (mainFrame)
        mainFrame->Release();
    if (dataSource)
        dataSource->Release();
    if (request)
        request->Release();
    SysFreeString(frameURL);
    return 0;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::didFailProvisionalLoadWithError(IWebView*, IWebError *error, IWebFrame*)
{
    BSTR errorDescription = 0;
    HRESULT hr = error->localizedDescription(&errorDescription);
    if (FAILED(hr))
        errorDescription = L"Failed to load page and to localize error description.";

    ::MessageBoxW(0, static_cast<LPCWSTR>(errorDescription), L"Error", MB_APPLMODAL | MB_OK);
    if (SUCCEEDED(hr))
        SysFreeString(errorDescription);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebFrameLoadDelegate*>(this);
    else if (IsEqualGUID(riid, IID_IWebFrameLoadDelegate))
        *ppvObject = static_cast<IWebFrameLoadDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WinLauncherWebHost::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WinLauncherWebHost::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

HRESULT WinLauncherWebHost::didFinishLoadForFrame(IWebView* webView, IWebFrame* frame)
{
    IDOMDocument* doc = 0;
    frame->DOMDocument(&doc);

    IDOMElement* element = 0;
    IDOMEventTarget* target = 0;
    HRESULT hr = doc->getElementById(L"webkit logo", &element);
    if (!SUCCEEDED(hr))
        goto exit;

    hr = element->QueryInterface(IID_IDOMEventTarget, reinterpret_cast<void**>(&target));
    if (!SUCCEEDED(hr))
        goto exit;

    hr = target->addEventListener(L"click", new SimpleEventListener (L"webkit logo click"), FALSE);
    if (!SUCCEEDED(hr))
        goto exit;

exit:
    if (target)
        target->Release();
    if (element)
        element->Release();
    if (doc)
        doc->Release();

    return hr;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::jobStarted(
    /* [in] */ int jobId,
    BSTR url,
    BOOL synchronous)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::jobDataComplete(
    /* [in] */ int jobId,
    /* [in] */ int httpCode)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::jobFinished(
    /* [in] */ int jobId,
    /* [in] */ BOOL success,
    /* [in] */ int errorCode)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WinLauncherWebHost::jobDebug(
    /* [in] */ int jobId,
    /* [in] */ DebugInformation debugType,
    /* [in] */ unsigned char *data,
    /* [in] */ int totalSize)
{
    static double timeStart = 0;
    if (!timeStart)
        timeStart = WTF::currentTime();

    wchar_t dataBuf[1024];
    bool needCRLF = false;
    if (debugType == Text || debugType == HeaderIn || debugType == HeaderOut) {
        size_t numConverted;
        mbstowcs_s(&numConverted, dataBuf, 1024, (const char*) data, totalSize > 1023 ? 1023 : totalSize);
        needCRLF = (dataBuf[numConverted - 2] != L'\n');
    }

    wchar_t dstBuf[1024];
    if (debugType == DataIn || debugType == DataOut)
        _snwprintf(dstBuf, 1024, L"%08.3f [%s] %d bytes\r\n",
        WTF::currentTime() - timeStart, debugType == DataIn ? L"Data In" : L"Data Out", totalSize);
    else if (debugType == Text)
        _snwprintf(dstBuf, 1024, L"%08.3f %s%s",
        WTF::currentTime() - timeStart, dataBuf, needCRLF ? L"\r\n" : L"");
    else
        _snwprintf(dstBuf, 1024, L"%08.3f [%s] %s%s",
        WTF::currentTime() - timeStart,
            debugType == HeaderIn ? L"Header In" :
            debugType == HeaderOut ? L"Header Out" : L"???",
            dataBuf, needCRLF ? L"\r\n" : L"");

    OutputDebugStringW(dstBuf);
    return S_OK;
}

static void resizeSubViews()
{
    if (usesLayeredWebView() || !gViewWindow)
        return;

    RECT rcClient;
    GetClientRect(hMainWnd, &rcClient);
    MoveWindow(hURLBarWnd, 0, 0, rcClient.right, URLBAR_HEIGHT, TRUE);
    MoveWindow(gViewWindow, 0, URLBAR_HEIGHT, rcClient.right, rcClient.bottom - URLBAR_HEIGHT, TRUE);
}

static void subclassForLayeredWindow()
{
    hMainWnd = gViewWindow;
#if defined _M_AMD64 || defined _WIN64
    DefWebKitProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hMainWnd, GWLP_WNDPROC));
    ::SetWindowLongPtr(hMainWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
#else
    DefWebKitProc = reinterpret_cast<WNDPROC>(::GetWindowLong(hMainWnd, GWL_WNDPROC));
    ::SetWindowLong(hMainWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
#endif
}

static void computeFullDesktopFrame()
{
    RECT desktop;
    if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, static_cast<void*>(&desktop), 0))
        return;

    s_windowPosition.x = 0;
    s_windowPosition.y = 0;
    s_windowSize.cx = desktop.right - desktop.left;
    s_windowSize.cy = desktop.bottom - desktop.top;
}

#ifdef _WIN32_WCE
namespace WTF {
__declspec(dllimport) void setMainThreadStackBounds(void *, void*);
}
#endif

#ifdef BUILD_WINLAUNCHER_EXE
#if defined(_WIN32_WCE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpstrCmdLine, int nCmdShow)
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpstrCmdLine, int nCmdShow)
#endif
{
    hInst = hInstance;
#else
BOOL WINAPI DllMain(HINSTANCE dllInstance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        hInst = dllInstance;

    return TRUE;
}
extern "C" __declspec(dllexport) int WINAPI dllLauncherEntryPoint(HINSTANCE, HINSTANCE, LPTSTR, int nCmdShow)
{
#endif

#ifdef _CRTDBG_MAP_ALLOC
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
#endif

     // TODO: Place code here.
    MSG msg = {0};
    HACCEL hAccelTable;
    BSTR frameUrl = NULL;
    INITCOMMONCONTROLSEX InitCtrlEx;

    InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrlEx.dwICC  = 0x00004000; //ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&InitCtrlEx);

    // Init COM
#ifdef _WIN32_WCE
    // Take the address of the first variable in this function, aligned
    // to a 4k boundary as the start of the stack, then substract the stack
    // size (because the stack is growing downward).
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD pageSize = systemInfo.dwPageSize;
    void * stackStart = (void*)((reinterpret_cast<unsigned>(&msg) & ~(pageSize - 1)) + pageSize);
    void * stackBound = static_cast<char*>(stackStart) - (508 * 1024); // assume up to 4K stack above us
    WTF::setMainThreadStackBounds(stackStart, stackBound);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#else
    OleInitialize(NULL);
#endif

    storagePath = SysAllocString(L".\\");

    gWebHost = new WinLauncherWebHost();
    gWebHost->AddRef();

#if PLATFORM(HP)
    // The debugger hangs loading symbols for this lib, but Jim's blog post
    // mentions a workaround that seems to work.
    // http://qualapps.blogspot.de/2006/12/visual-studio-hangs-during-debugging.html
    HMODULE lib = LoadLibrary(L"RASAPI32.DLL");
    FreeLibrary(lib);
    lib = LoadLibrary(L"RTUTILS.DLL");
    FreeLibrary(lib);
    lib = LoadLibrary(L"SENSAPI.DLL");
    FreeLibrary(lib);
#if defined(_WIN32_WCE)
    LoadIniSettings("\\Program Files\\WinLauncher\\browser.ini");
#else
    LoadIniSettings("browser.ini");
#endif
#endif

#ifndef _WIN32_WCE
    int argc = 0;
    WCHAR** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        if (!wcsicmp(argv[i], L"--transparent"))
            s_usesLayeredWebView = true;
        else if (!wcsicmp(argv[i], L"--desktop"))
            s_fullDesktop = true;
        else if (!_wcsnicmp(argv[i], L"--inspector-url=", 16))
            inspectorURL = SysAllocString(argv[i] + 16);
        else if (!_wcsnicmp(argv[i], L"--inspector-server=", 19))
            inspectorServer = SysAllocString(argv[i] + 19);
        else if (!_wcsnicmp(argv[i], L"--frame-url=", 12))
            frameUrl = SysAllocString(argv[i] + 12);
    }
#endif

    // Initialize global strings
    LoadString(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInst, IDC_WINLAUNCHER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInst);

    if (shouldUseFullDesktop())
        computeFullDesktopFrame();

    if (usesLayeredWebView()) {
        hURLBarWnd = CreateWindow(L"EDIT", L"Type URL Here",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOVSCROLL, 
                    s_windowPosition.x, s_windowPosition.y + s_windowSize.cy, s_windowSize.cx, URLBAR_HEIGHT,
                    0,
                    0,
                    hInst, 0);
    } else {
        hMainWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInst, 0);

        if (!hMainWnd)
            return FALSE;

        hURLBarWnd = CreateWindow(L"EDIT", 0,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOVSCROLL, 
                    0, 0, 0, 0,
                    hMainWnd,
                    0,
                    hInst, 0);

        ShowWindow(hMainWnd, nCmdShow);
        UpdateWindow(hMainWnd);
    }

#if defined _M_AMD64 || defined _WIN64
    DefEditProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hURLBarWnd, GWLP_WNDPROC));
    SetWindowLongPtr(hURLBarWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MyEditProc));
#else
    DefEditProc = reinterpret_cast<WNDPROC>(GetWindowLong(hURLBarWnd, GWL_WNDPROC));
    SetWindowLong(hURLBarWnd, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(MyEditProc));
#endif

    SetFocus(hURLBarWnd);

    RECT clientRect = { s_windowPosition.x, s_windowPosition.y, s_windowPosition.x + s_windowSize.cx, s_windowPosition.y + s_windowSize.cy };

    IWebPreferences* tmpPreferences = 0;
    IWebPreferences* standardPreferences = 0;
    if (FAILED(WebKitCreateInstance(CLSID_WebPreferences, 0, IID_IWebPreferences, reinterpret_cast<void**>(&tmpPreferences))))
        goto exit;

    if (FAILED(tmpPreferences->standardPreferences(&standardPreferences)))
        goto exit;

    standardPreferences->setAcceleratedCompositingEnabled(TRUE);

    standardPreferences->setEmulateTouchEvents(TRUE);

    IWebPreferencesPrivate * prefsPrivate = NULL;
    if (SUCCEEDED(standardPreferences->QueryInterface(IID_IWebPreferencesPrivate, (void**)&prefsPrivate))) {
        prefsPrivate->setLocalStorageDatabasePath(storagePath);

        if(inspectorURL != NULL){
            prefsPrivate->setInspectorURL(inspectorURL);
        }
        if(inspectorServer != NULL){
            prefsPrivate->setInspectorServerAddress(inspectorServer);
            SysFreeString(inspectorServer);
        }
        prefsPrivate->Release();
    }

    HRESULT hr = WebKitCreateInstance(CLSID_WebView, 0, IID_IWebView, reinterpret_cast<void**>(&gWebView));
    if (FAILED(hr))
        goto exit;

    hr = gWebView->QueryInterface(IID_IWebViewPrivate, reinterpret_cast<void**>(&gWebViewPrivate));
    if (FAILED(hr))
        goto exit;

    hr = gWebView->setFrameLoadDelegate(gWebHost);
    if (FAILED(hr))
        goto exit;

    gPrintDelegate = new PrintWebUIDelegate;
    gPrintDelegate->AddRef();
    hr = gWebView->setUIDelegate(gPrintDelegate);
    if (FAILED (hr))
        goto exit;

    hr = gWebView->setHostWindow(reinterpret_cast<OLE_HANDLE>(hMainWnd));
    if (FAILED(hr))
        goto exit;

    hr = gWebView->initWithFrame(clientRect, 0, 0);
    if (FAILED(hr))
        goto exit;

    IWebFrame* frame;
    hr = gWebView->mainFrame(&frame);
    if (FAILED(hr))
        goto exit;

    if(frameUrl != NULL){
        loadURL(frameUrl);
        SysFreeString(frameUrl);
    }
    else{
        static BSTR defaultHTML = SysAllocString(TEXT("<p style=\"background-color: #00FF00\">Testing</p><img id=\"webkit logo\" src=\"http://webkit.org/images/icon-gold.png\" alt=\"Face\"><div style=\"border: solid blue; background: white;\" contenteditable=\"true\">div with blue border</div><ul><li>foo<li>bar<li>baz</ul>"));
        frame->loadHTMLString(defaultHTML, 0);
        frame->Release();
    }

    hr = gWebViewPrivate->setTransparent(usesLayeredWebView());
    if (FAILED(hr))
        goto exit;

    hr = gWebViewPrivate->setUsesLayeredWindow(usesLayeredWebView());
    if (FAILED(hr))
        goto exit;

    hr = gWebViewPrivate->viewWindow(reinterpret_cast<OLE_HANDLE*>(&gViewWindow));
    if (FAILED(hr) || !gViewWindow)
        goto exit;

    if (usesLayeredWebView())
        subclassForLayeredWindow();

    resizeSubViews();

    ShowWindow(gViewWindow, nCmdShow);
    UpdateWindow(gViewWindow);

    hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_WINLAUNCHER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
#if PLATFORM(HP)
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_PAUSE) {
                // Let's do a memory dump.
                // Enable stack collection if we haven't already.
                EnableStackCollection(true);
                DumpMemoryAllocations(true);
            }
#endif
            DispatchMessage(&msg);
        }
    }

exit:
    gPrintDelegate->Release();
    if (gWebViewPrivate)
        gWebViewPrivate->Release();
    gWebView->Release();
    if (standardPreferences)
        standardPreferences->Release();
    tmpPreferences->Release();

    shutDownWebKit();
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif

    // Shut down COM.
#ifdef _WIN32_WCE
    CoUninitialize();
#else
    OleUninitialize();
#endif
    
    return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
#if defined(_WIN32_WCE)
    WNDCLASS wcex;
    wcex.lpszMenuName   = 0;
#else
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_WINLAUNCHER);
#endif

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINLAUNCHER));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = 0;
    wcex.lpszClassName  = szWindowClass;

#if defined(_WIN32_WCE)
    return RegisterClass(&wcex);
#else
    return RegisterClassEx(&wcex);
#endif
}

#if !defined(_WIN32_WCE)
static BOOL CALLBACK AbortProc(HDC hDC, int Error)
{
    MSG msg;
    while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return TRUE;
}

static HDC getPrinterDC()
{
    PRINTDLG pdlg;
    memset(&pdlg, 0, sizeof(PRINTDLG));
    pdlg.lStructSize = sizeof(PRINTDLG);
    pdlg.Flags = PD_PRINTSETUP | PD_RETURNDC;

    ::PrintDlg(&pdlg);

    return pdlg.hDC;
}

static void initDocStruct(DOCINFO* di, TCHAR* docname)
{
    memset(di, 0, sizeof(DOCINFO));
    di->cbSize = sizeof(DOCINFO);
    di->lpszDocName = docname;
}

void PrintView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC printDC = getPrinterDC();
    if (!printDC) {
        ::MessageBoxW(0, L"Error creating printing DC", L"Error", MB_APPLMODAL | MB_OK);
        return;
    }

    if (::SetAbortProc(printDC, AbortProc) == SP_ERROR) {
        ::MessageBoxW(0, L"Error setting up AbortProc", L"Error", MB_APPLMODAL | MB_OK);
        return;
    }

    IWebFrame* frame = 0;
    IWebFramePrivate* framePrivate = 0;
    if (FAILED(gWebView->mainFrame(&frame)))
        goto exit;

    if (FAILED(frame->QueryInterface(&framePrivate)))
        goto exit;

    framePrivate->setInPrintingMode(TRUE, printDC);

    UINT pageCount = 0;
    framePrivate->getPrintedPageCount(printDC, &pageCount);

    DOCINFO di;
    initDocStruct(&di, L"WebKit Doc");
    ::StartDoc(printDC, &di);

    // FIXME: Need CoreGraphics implementation
    void* graphicsContext = 0;
    for (size_t page = 1; page <= pageCount; ++page) {
        ::StartPage(printDC);
        framePrivate->spoolPages(printDC, page, page, graphicsContext);
        ::EndPage(printDC);
    }

    framePrivate->setInPrintingMode(FALSE, printDC);

    ::EndDoc(printDC);
    ::DeleteDC(printDC);

exit:
    if (frame)
        frame->Release();
    if (framePrivate)
        framePrivate->Release();
}
#endif

static const int dragBarHeight = 30;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC parentProc = usesLayeredWebView() ? DefWebKitProc : DefWindowProc;

    switch (message) {
#if !defined(_WIN32_WCE)
    case WM_NCHITTEST:
        if (usesLayeredWebView()) {
            RECT window;
            ::GetWindowRect(hWnd, &window);
            // For testing our transparent window, we need a region to use as a handle for
            // dragging. The right way to do this would be to query the web view to see what's
            // under the mouse. However, for testing purposes we just use an arbitrary
            // 30 pixel band at the top of the view as an arbitrary gripping location.
            //
            // When we are within this bad, return HT_CAPTION to tell Windows we want to
            // treat this region as if it were the title bar on a normal window.
            int y = HIWORD(lParam);

            if ((y > window.top) && (y < window.top + dragBarHeight))
                return HTCAPTION;
        }
        return CallWindowProc(parentProc, hWnd, message, wParam, lParam);
#endif
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
#if !defined(_WIN32_WCE)
        case IDM_PRINT:
            PrintView(hWnd, message, wParam, lParam);
            break;
#endif
        default:
            return CallWindowProc(parentProc, hWnd, message, wParam, lParam);
        }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (!gWebView || usesLayeredWebView())
           return CallWindowProc(parentProc, hWnd, message, wParam, lParam);

        resizeSubViews();
        break;
    default:
        return CallWindowProc(parentProc, hWnd, message, wParam, lParam);
    }

    return 0;
}

#define MAX_URL_LENGTH  1024

LRESULT CALLBACK MyEditProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_CHAR:
            if (wParam == 13) { // Enter Key
                wchar_t strPtr[MAX_URL_LENGTH];
                *((LPWORD)strPtr) = MAX_URL_LENGTH; 
                int strLen = SendMessage(hDlg, EM_GETLINE, 0, (LPARAM)strPtr);

                BSTR bstr = SysAllocStringLen(strPtr, strLen);
                loadURL(bstr);
                SysFreeString(bstr);

                return 0;
            } else
                return (LRESULT)CallWindowProc((WNDPROC)DefEditProc,hDlg,message,wParam,lParam);
            break;
        default:
             return (LRESULT)CallWindowProc((WNDPROC)DefEditProc,hDlg,message,wParam,lParam);
        break;
    }
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static void loadURL(BSTR urlBStr)
{
    IWebFrame* frame = 0;
    IWebMutableURLRequest* request = 0;

    static BSTR methodBStr = SysAllocString(TEXT("GET"));

#ifndef _WIN32_WCE
    if (urlBStr && urlBStr[0] && (PathFileExists(urlBStr) || PathIsUNC(urlBStr))) {
        TCHAR fileURL[INTERNET_MAX_URL_LENGTH];
        DWORD fileURLLength = sizeof(fileURL)/sizeof(fileURL[0]);

        if (SUCCEEDED(UrlCreateFromPath(urlBStr, fileURL, &fileURLLength, 0)))
            SysReAllocString(&urlBStr, fileURL);
    }
#endif

    HRESULT hr = gWebView->mainFrame(&frame);
    if (FAILED(hr))
        goto exit;

    hr = WebKitCreateInstance(CLSID_WebMutableURLRequest, 0, IID_IWebMutableURLRequest, (void**)&request);
    if (FAILED(hr))
        goto exit;

    hr = request->initWithURL(urlBStr, WebURLRequestUseProtocolCachePolicy, 60);
    if (FAILED(hr))
        goto exit;

    hr = request->setHTTPMethod(methodBStr);
    if (FAILED(hr))
        goto exit;

    hr = frame->loadRequest(request);
    if (FAILED(hr))
        goto exit;

    SetFocus(gViewWindow);

exit:
    if (frame)
        frame->Release();
    if (request)
        request->Release();
}

#if PLATFORM(HP)
static void RecordMemoryDump(const char * text, int len, void * data)
{
    DWORD written = 0;
    WriteFile((HANDLE) data, text, len, &written, NULL);
}

static void DumpMemoryAllocations(bool logAllocations)
{
    static int dumpCount = 0;
    static unsigned int lastAllocCount = 0;

    unsigned int currentAllocCount = MemoryDebugGetCurrentAllocationID();

#if defined(_WIN32_WCE)
    wchar_t directory_path[] = L"\\Program Files\\WinLauncher";
#else
    wchar_t directory_path[256];
    GetCurrentDirectoryW(256, directory_path);
#endif
    wchar_t filename[256];
    _snwprintf(filename, 256, L"%s\\memorydump-%d (%u-%u).txt", directory_path, dumpCount++, lastAllocCount, currentAllocCount);

    wchar_t wbuff[512];
    _snwprintf(wbuff, 256, L"Writing memory %s log to %s\n", logAllocations ? L"allocation" : L"free", filename);
    OutputDebugStringW(wbuff);

    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (logAllocations)
        MemoryDebugDumpAllObjects(RecordMemoryDump, lastAllocCount, hFile);
    else
        MemoryDebugDumpAllFreedObjects(true, RecordMemoryDump, lastAllocCount, hFile);
    CloseHandle(hFile);

    lastAllocCount = MemoryDebugGetCurrentAllocationID();
}
#endif
