#ifndef WTF_PERFORMANCE_TRACE_H
#define WTF_PERFORMANCE_TRACE_H

#include <wtf/Platform.h>
#include <wtf/ExportMacros.h>

#if ENABLE(PERFORMANCE_TRACING)

namespace WTF {

WTF_EXPORT_PRIVATE class PerformanceTrace {
public:
    enum Event {
        FireEventListeners,
        EventDispatch,
        InputEvent,
        WindowsMessage,
        ExecuteJavaScript,
        RecalcStyle,
        Layout,
        Paint,
        Other,
        Unknown // clients should not call PerformanceTrace::start(Unknown) - this is reserved to indicate significant blocks of time not measured between start <-> end calls.
    };
    typedef void (*PerformanceTraceOutputFunc)(int depth, double time, double elapsed, Event id, const char * text);
    static void start(Event id, const char * description = NULL);
    static void end(Event id);
    static void setOutputHandler(PerformanceTraceOutputFunc function);
    static void dumpEvents();
    static void enable(bool);
};

}

#define PERFORMANCE_START(...)     WTF::PerformanceTrace::start(__VA_ARGS__)
#define PERFORMANCE_END(x)       WTF::PerformanceTrace::end(x)

#else

#define PERFORMANCE_START(...)
#define PERFORMANCE_END(x)

#endif

#endif WTF_PERFORMANCE_TRACE_H
