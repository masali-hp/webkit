#if ENABLE(GESTURE_EVENTS)

#if !USE(SIMULATED_GESTURES)
#error PlatformGestureSupport.h is only for use with SIMULATED_GESTURES.
#endif

#include "PlatformGestureModel.h"

#undef DefWindowProc
//#define DefWindowProc DefWindowProc_Proxy

// It would be nice to get the wndProc from hWnd, rather than
// have it passed as a parameter, but WinCE modifies(!)
// lParam for WM_GESTURE messages.  (Interestingly, send the
// same data as a WM_GESTURENOTIFY and it doesn't change lParam).

LRESULT CALLBACK DefWindowProc_Proxy(
    WNDPROC wndProc,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

// Simulate the gesture functions that WebView.cpp soft links in:
typedef BOOL (*GetGestureInfoFunc)(HGESTUREINFO, PGESTUREINFO);
GetGestureInfoFunc GetGestureInfoPtr();
typedef BOOL (*CloseGestureInfoHandleFunc)(HGESTUREINFO);
CloseGestureInfoHandleFunc CloseGestureInfoHandlePtr();

#if USE(XP_GESTURE_MODEL)

typedef BOOL (*SetGestureConfigFunc)(HWND, DWORD, UINT, PGESTURECONFIG, UINT);
SetGestureConfigFunc SetGestureConfigPtr();

#elif USE(CE_GESTURE_MODEL)

#define TGF_SCOPE_PROCESS 1
#define TGF_SCOPE_WINDOW 2
typedef BOOL (*EnableGesturesFunc)(HWND, ULONGLONG, UINT);
EnableGesturesFunc EnableGesturesPtr();
typedef BOOL (*DisableGesturesFunc)(HWND, ULONGLONG, UINT);
DisableGesturesFunc DisableGesturesPtr();

#endif

// XP and CE share some gesture functions and structures, but there
// are many things that are different.
//
// For example:
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd353232(v=vs.85).aspx
// Note The GID_PAN gesture has built-in inertia. At the end of a pan gesture, additional
// pan gesture messages are created by the operating system.
//
// CE doesn't do this.  It sends a GID_SCROLL message at the end; the magnitude
// of the scroll depends on the velocity at the time the finger is released from
// the screen.  It's then up to the app to create a timer and perform additional
// scrolling (unless you use the auto gesture API).

// For simplicities sake the CE gesture model simulation will be as close as possible
// to correct; the XP model will be lazy and borrow some things from CE:

// Gesture Macros (Compact 2013)
// http://msdn.microsoft.com/en-us/library/ee504388.aspx

#define GID_SCROLL_VELOCITY(x)     ((LONG)(((ULONGLONG)(x)) >> 32))
#define GID_SCROLL_DIRECTION(x)    (LOWORD((x)))

#define ARG_SCROLL_NONE  0
#define ARG_SCROLL_DOWN  1
#define ARG_SCROLL_LEFT  2
#define ARG_SCROLL_UP    3
#define ARG_SCROLL_RIGHT 4

#define GID_SCROLL     8
#define TGF_GID_PAN    1
#define TGF_GID_SCROLL 2

#endif
