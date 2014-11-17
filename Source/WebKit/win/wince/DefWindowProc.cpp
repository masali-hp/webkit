#include "config.h"
#include "PlatformGestureModel.h"

#if USE(SIMULATED_GESTURES)
#include <windows.h>
#include <windowsx.h>
#include <map>

#include <WebCore/WindowsTouch.h>
#include <wince/PlatformGestureSupport.h>
#include <wtf/CurrentTime.h>
#include <wtf/LRUPriorityQueue.h>

// This file emulates gesture support which is not available on WinCE 6.0.

// To prevent a simple click from being treated as a gesture (there is
// mouse 'wiggle' between the mouse down and mouse up), configure how
// far the mouse has to move for a gesture to start.
#define PAN_THRESHOLD 15 // pixels

class GestureConfig {
public:
    GestureConfig()
#if USE(XP_GESTURE_MODEL)
        : config(NULL)
        , cIDs(0)
#elif USE(CE_GESTURE_MODEL)
        : enabledGestures(0)
#endif
    {
    }
#if USE(XP_GESTURE_MODEL)
    PGESTURECONFIG config;
    UINT cIDs;
#elif USE(CE_GESTURE_MODEL)
    ULONGLONG enabledGestures;
#endif
};

using namespace std;

typedef map<HWND, GestureConfig> gestureConfigMapType;
typedef map<HGESTUREINFO, GESTUREINFO*> gestureMapType;

static gestureConfigMapType * gestureConfigMap = NULL;
static gestureMapType * gestureMap = NULL;
static WTF::LRUPriorityQueue<double> * touchMoveDeltaHistory = NULL;
#define MOUSE_MOVE_HISTORY_LEN 20

#if USE(CE_GESTURE_MODEL)
GestureConfig processConfig;
#endif

#if USE(XP_GESTURE_MODEL)
static BOOL SetGestureConfigProxy(HWND hWnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize)
{
    if (sizeof(GESTURECONFIG) != cbSize)
        return FALSE;
    if (!gestureConfigMap) {
        gestureConfigMap = new gestureConfigMapType();
        gestureMap = new gestureMapType();
    }
    GestureConfig & config = (*gestureConfigMap)[hWnd];
    config.cIDs = cIDs;
    if (config.config && cIDs != config.cIDs) {
        delete [] config.config;
        config.config = NULL;
    }
    if (cIDs) {
        if (!config.config)
            config.config = new GESTURECONFIG[cIDs];
        memcpy(config.config, pGestureConfig, cbSize * cIDs);
    }
    return TRUE;
}
#elif USE(CE_GESTURE_MODEL)
static BOOL EnableGesturesProxy(HWND wnd, ULONGLONG gesturesToEnable, UINT scope)
{
    if (!gestureMap) {
        gestureMap = new gestureMapType();
        touchMoveDeltaHistory = new WTF::LRUPriorityQueue<double>(MOUSE_MOVE_HISTORY_LEN);
    }
    if (scope == TGF_SCOPE_PROCESS) {
        processConfig.enabledGestures |= gesturesToEnable;
    }
    else if (scope == TGF_SCOPE_WINDOW && wnd) {
        if (!gestureConfigMap) {
            gestureConfigMap = new gestureConfigMapType();
        }
        (*gestureConfigMap)[wnd].enabledGestures |= gesturesToEnable;
    }
    return TRUE;
}

static BOOL DisableGesturesProxy(HWND wnd, ULONGLONG gesturesToDisable, UINT scope)
{
    if (scope == TGF_SCOPE_PROCESS) {
        processConfig.enabledGestures &= ~gesturesToDisable;
    }
    if (scope == TGF_SCOPE_WINDOW) {
        if (!gestureConfigMap)
            return FALSE;
        gestureConfigMapType::iterator it = gestureConfigMap->find(wnd);
        if (it == gestureConfigMap->end())
            return FALSE;
        it->second.enabledGestures &= ~gesturesToDisable;
        if (!it->second.enabledGestures)
            gestureConfigMap->erase(it);
    }
    return TRUE;
}
#endif

static BOOL GetGestureInfoProxy(HGESTUREINFO handle, PGESTUREINFO pinfo)
{
    if (!gestureMap)
        return FALSE;
    gestureMapType::iterator it = gestureMap->find(handle);
    if (it == gestureMap->end())
        return FALSE;
    memcpy(pinfo, it->second, sizeof(GESTUREINFO));
    return TRUE;
}

static BOOL CloseGestureInfoHandleProxy(HGESTUREINFO handle)
{
    if (!gestureMap)
        return FALSE;
    gestureMapType::iterator it = gestureMap->find(handle);
    if (it == gestureMap->end())
        return FALSE;
    delete it->second;
    gestureMap->erase(it);
    return TRUE;
}

#if USE(XP_GESTURE_MODEL)
inline static bool gestureWanted(const GESTURECONFIG & gconfig, int flag) {
    return (gconfig.dwWant & flag) && !(gconfig.dwBlock & flag);
}
#endif

static bool gestureConfigForWindow(HWND hWnd, GestureConfig& config)
{
    if (!gestureConfigMap) {
#if USE(XP_GESTURE_MODEL)
        return false;
#elif USE(CE_GESTURE_MODEL)
        config = processConfig;
#endif
    }
    else {
        gestureConfigMapType::iterator it = gestureConfigMap->find(hWnd);
        if (it == gestureConfigMap->end()) {
#if USE(XP_GESTURE_MODEL)
            return false;
#elif USE(CE_GESTURE_MODEL)
            config = processConfig;
#endif
        }
        else {
            config = it->second;
        }
    }
    return true;
}

// is the window currently configured to recognize any gestures?
static bool windowRecognizesGestures(HWND hWnd)
{
    GestureConfig config;
    if (!gestureConfigForWindow(hWnd, config))
        return false;
#if USE(XP_GESTURE_MODEL)
    for (int i = 0; i < config.cIDs; i++) {
        GESTURECONFIG & gconfig = config.config[i];
        if (gconfig.dwID == GID_PAN) {
            if (gestureWanted(gconfig, GC_PAN_WITH_SINGLE_FINGER_VERTICALLY) ||
                gestureWanted(gconfig, GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY)) {
                return true;
            }
        }
    }
    return false;
#elif USE(CE_GESTURE_MODEL)
    return config.enabledGestures;
#endif
}

static GESTUREINFO * allocateGesture(DWORD gid, HWND target, POINTS startLocation)
{
    if (!gestureMap)
        return NULL;
    GESTUREINFO * gestureInfo = new GESTUREINFO();
    gestureInfo->cbSize = sizeof(GESTUREINFO);
    gestureInfo->dwID = gid;
    gestureInfo->dwFlags = 0;
    gestureInfo->hwndTarget = target;
    gestureInfo->ptsLocation = startLocation;
    (*gestureMap)[reinterpret_cast<HGESTUREINFO>(gestureInfo)] = gestureInfo;
    return gestureInfo;
}

static GESTUREINFO * allocateGesture(DWORD gid, HWND target, POINTS startLocation, DWORD gf_flags, POINTS location, POINTS delta)
{
    GESTUREINFO * gestureInfo = allocateGesture(gid, target, startLocation);
    if (!gestureInfo)
        return NULL;

    gestureInfo->dwFlags = gf_flags;
    gestureInfo->ptsLocation.y = location.y;
    gestureInfo->ptsLocation.x = location.x;

    // http://msdn.microsoft.com/en-us/library/ee503892.aspx
    // dwFlags: These flags are different from the GID_BEGIN and GID_END window
    // messages. GID_BEGIN and GID_END indicate the beginning and end of a
    // gesture group, which can consist of one or more gestures. Each individual
    // gesture has GF_BEGIN and GF_END flags that indicate the start and end of
    // that individual gesture.
#if USE(XP_GESTURE_MODEL)
    if (gid == GID_PAN && (gf_flags & GF_END)) {
#elif USE(CE_GESTURE_MODEL)
    if (gid == GID_SCROLL) {
#endif
        int scrollDirection = ARG_SCROLL_NONE;

        if (abs(delta.y) > abs(delta.x)) {
            // primary scroll direction: vertical
            if (delta.y < 0)
                scrollDirection = ARG_SCROLL_DOWN;
            else if (delta.y > 0)
                scrollDirection = ARG_SCROLL_UP;
        }
        else {
            // primary scroll direction: horizontal
            if (delta.x < 0)
                scrollDirection = ARG_SCROLL_RIGHT;
            else if (delta.x > 0)
                scrollDirection = ARG_SCROLL_LEFT;
        }

        // Unfortunately, GetMessageTime does not exist on WinCE so we have no way to know
        // when the mouse move events were recorded.  Instead we track a history of deltas
        // between WM_MOUSEMOVE events.  Both the slowest and fastest times are wrong, but
        // we want close to the fastest time.  sorted(MOUSE_MOVE_HISTORY_LEN / 5) yields
        // the 80th percentile time.
        double estimatedTime;
        if (touchMoveDeltaHistory->size() > MOUSE_MOVE_HISTORY_LEN / 2)
            estimatedTime = touchMoveDeltaHistory->sorted(MOUSE_MOVE_HISTORY_LEN / 5);
        else
            estimatedTime = 0.01;
        int velocity = max(abs(delta.x), abs(delta.y)) / estimatedTime; // pixels / second
        gestureInfo->ullArguments = (((ULONGLONG) velocity) << 32) + (ULONGLONG) scrollDirection;
        // TODO: support multiple dimensional scrolling?
    }
    else {
        gestureInfo->ullArguments = 0;
    }
    gestureInfo->dwInstanceID = 0;
    gestureInfo->dwSequenceID = 0;
    gestureInfo->cbExtraArgs = 0;
    return gestureInfo;
}

#if USE(XP_GESTURE_MODEL)
static bool TestForPanGesture(const GESTURECONFIG & gconfig, const int gc_axis_flag, const int delta, const int contra_delta)
{
    if (gestureWanted(gconfig, gc_axis_flag)) {
        if (delta > PAN_THRESHOLD && delta > contra_delta) {
            return true;
        }
    }
    return false;
}

static LONG recognizeGesture(const GestureConfig & config, POINTS mouseDown, POINTS location) {
    POINTS delta = {location.x - mouseDown.x, location.y - mouseDown.y};
    POINTS abs_delta = {abs(delta.x), abs(delta.y)};
    LONG gestureId = 0;
    for (int i = 0; i < config.cIDs && !gestureId; i++) {
        GESTURECONFIG & gconfig = config.config[i];
        if (gconfig.dwID == GID_PAN) {
            if (TestForPanGesture(gconfig, GC_PAN_WITH_SINGLE_FINGER_VERTICALLY, abs_delta.y, abs_delta.x) ||
                TestForPanGesture(gconfig, GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY, abs_delta.x, abs_delta.y)) {
                return gconfig.dwID;
            }
        }
    }
    return 0;
}
#elif USE(CE_GESTURE_MODEL)
static LONG recognizeGesture(const GestureConfig & config, POINTS mouseDown, POINTS location) {
    POINTS delta = {location.x - mouseDown.x, location.y - mouseDown.y};
    POINTS abs_delta = {abs(delta.x), abs(delta.y)};
    if (config.enabledGestures & TGF_GID_PAN) {
        if (abs_delta.x > PAN_THRESHOLD || abs_delta.y > PAN_THRESHOLD)
            return GID_PAN;
    }
    return 0;
}
#endif

static void runGestureLoop(HWND hWnd, WNDPROC wndProc, POINTS startLocation)
{
    LONG gestureStarted = 0;
    bool gestureEnded = false;
    GestureConfig config;
    gestureConfigForWindow(hWnd, config);
    MSG msg;
    double startTime = WTF::currentTime();
    bool gf_begin_sent = false;
    POINTS lastLocation = startLocation;
    POINTS moveDelta = {0, 0};
    double lastTime = 0;

    GESTUREINFO * gestureInfo = allocateGesture(GID_BEGIN, hWnd, startLocation);
    wndProc(hWnd, WM_GESTURE, 0, reinterpret_cast<LPARAM>(gestureInfo));

    while (!gestureEnded && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        // Filter all mouse events during the gesture:
        if (msg.message == WM_LBUTTONUP || msg.message == WM_LBUTTONDOWN ||
            msg.message == WM_RBUTTONUP || msg.message == WM_RBUTTONDOWN ||
            msg.message == WM_MOUSEMOVE)
        {
            POINT pt = {GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)};
            ::ClientToScreen(msg.hwnd, &pt);
            POINTS location = {pt.x, pt.y};
            double currentTime =  WTF::currentTime();

            if (msg.message == WM_LBUTTONUP || (msg.message == WM_MOUSEMOVE && !(MK_LBUTTON & msg.wParam)) || msg.hwnd != hWnd
                || msg.message == WM_LBUTTONDOWN) { // shouldn't ever see a WM_LBUTTONDOWN down here... but if we do, let's end the gesture.
                gestureEnded = true;
            }
            if (!gestureStarted) {
                if (!gestureEnded && msg.message == WM_MOUSEMOVE) {
                    gestureStarted = recognizeGesture(config, startLocation, location);
                }
            }
            if (gestureStarted) {
                if (gestureEnded) {
                    // If the finger/mouse pauses at the end of the gesture, don't send a final GID_PAN gesture or a GID_SCROLL message.
                    if (currentTime - lastTime < 0.100) {
                        DWORD flags = GF_END;
#if USE(CE_GESTURE_MODEL)
                        if (config.enabledGestures & TGF_GID_SCROLL)
                            flags |= GF_INERTIA;
#endif
                        gestureInfo = allocateGesture(gestureStarted, msg.hwnd, startLocation, flags, location, moveDelta);
                        wndProc(hWnd, WM_GESTURE, 0, reinterpret_cast<LPARAM>(gestureInfo));
#if USE(CE_GESTURE_MODEL)
                        if (config.enabledGestures & TGF_GID_SCROLL) {
                            // send a 'flick' gesture message now.
                            gestureInfo = allocateGesture(GID_SCROLL, msg.hwnd, startLocation, 0, location, moveDelta);
                            wndProc(hWnd, WM_GESTURE, 0, reinterpret_cast<LPARAM>(gestureInfo));
                        }
#endif
                    }
                }
                else if (msg.message == WM_MOUSEMOVE && (location.x != lastLocation.x || location.y != lastLocation.y)) {
                    if (lastTime != 0)
                        touchMoveDeltaHistory->add(currentTime - lastTime);
                    moveDelta.x = location.x - lastLocation.x;
                    moveDelta.y = location.y - lastLocation.y;
                    gestureInfo = allocateGesture(gestureStarted, msg.hwnd, startLocation, gf_begin_sent ? 0 : GF_BEGIN, lastLocation, moveDelta);
                    gf_begin_sent = true;
                    wndProc(hWnd, WM_GESTURE, 0, reinterpret_cast<LPARAM>(gestureInfo));
                    lastLocation = location;
                    lastTime = currentTime;
                }
            }
        }
        DispatchMessage(&msg);
    }
    gestureInfo = allocateGesture(GID_END, hWnd, startLocation);
    wndProc(hWnd, WM_GESTURE, 0, reinterpret_cast<LPARAM>(gestureInfo));
}

LRESULT CALLBACK DefWindowProc_Proxy(
    WNDPROC wndProc,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    bool handled = false;
    switch (Msg) {
        case WM_LBUTTONDOWN: {
            POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (hWnd)
                ::ClientToScreen(hWnd, &p);
            POINTS location = {p.x, p.y};
            GESTURENOTIFYSTRUCT notifyStruct;
            notifyStruct.cbSize = sizeof(GESTURENOTIFYSTRUCT);
            notifyStruct.ptsLocation.x = p.x;
            notifyStruct.ptsLocation.y = p.y;
            notifyStruct.dwFlags = 0;
            notifyStruct.hwndTarget = hWnd;
            notifyStruct.dwInstanceID = 0;
            // It would be nice to get the wndProc from hWnd, rather than
            // have it passed as a parameter, but WinCE modifies(!)
            // lParam for WM_GESTURE messages.  (Interestingly, send the
            // same data as a WM_GESTURENOTIFY and it doesn't change lParam).
            //WNDPROC wndProc = (WNDPROC) GetWindowLong(hWnd, GWL_WNDPROC);
            wndProc(hWnd, WM_GESTURENOTIFY, 0, reinterpret_cast<LPARAM>(&notifyStruct));
            if (windowRecognizesGestures(hWnd))
                runGestureLoop(hWnd, wndProc, location);
            handled = true;
            break;
        }
        case WM_GESTURE: {
            HGESTUREINFO gestureHandle = reinterpret_cast<HGESTUREINFO>(lParam);
            CloseGestureInfoHandleProxy(gestureHandle);
            handled = true;
            break;
        }
    }
    if (!handled)
        return DefWindowProcW(hWnd, Msg, wParam, lParam);
    return 0;
}

GetGestureInfoFunc GetGestureInfoPtr()
{
    return GetGestureInfoProxy;
}

CloseGestureInfoHandleFunc CloseGestureInfoHandlePtr()
{
    return CloseGestureInfoHandleProxy;
}

#if USE(XP_GESTURE_MODEL)
SetGestureConfigFunc SetGestureConfigPtr()
{
    return SetGestureConfigProxy;
}
#elif USE(CE_GESTURE_MODEL)
EnableGesturesFunc EnableGesturesPtr()
{
    return EnableGesturesProxy;
}

DisableGesturesFunc DisableGesturesPtr()
{
    return DisableGesturesProxy;
}
#endif

#endif
