#ifndef WTF_PERFORMANCE_TRACE_H
#define WTF_PERFORMANCE_TRACE_H

#include <wtf/Platform.h>
#include <wtf/ExportMacros.h>

#if ENABLE(PERFORMANCE_TRACING)

namespace WTF {

#if PLATFORM(HP_JEDI) && CPU(ARM) && OS(WINCE)
    typedef unsigned long PerformanceTimestamp;
#else
    typedef double PerformanceTimestamp;
#endif

typedef PerformanceTimestamp (*getTimestampFunc)();

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
        Unknown, // clients should not call PerformanceTrace::start(Unknown) - this is reserved to indicate significant blocks of time not measured between start <-> end calls.
        Timer,
        WebSocketData
    };
    typedef void (*PerformanceTraceOutputFunc)(int depth, PerformanceTimestamp time, PerformanceTimestamp elapsed, Event id, const char * text);
    static void start(Event id, const char * description = NULL);
    static void end(Event id, const char * description = NULL);
    static void setOutputHandler(PerformanceTraceOutputFunc function);
    static void dumpEvents();
    static void enable(bool);
    static unsigned TimestampFrequency(); // ticks / second
    static void initializeTimestampFunc();
    static getTimestampFunc getTimestamp;
};

}

#define PERFORMANCE_START(...)     WTF::PerformanceTrace::start(__VA_ARGS__)
#define PERFORMANCE_END(...)       WTF::PerformanceTrace::end(__VA_ARGS__)

#else

#define PERFORMANCE_START(...)
#define PERFORMANCE_END(x)

#endif

#endif WTF_PERFORMANCE_TRACE_H
