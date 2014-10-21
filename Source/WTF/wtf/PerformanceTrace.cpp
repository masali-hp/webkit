#include "config.h"

#if ENABLE(PERFORMANCE_TRACING)
#include "PerformanceTrace.h"
#include "CurrentTime.h"

#include <string.h>
#include <stdio.h>

#if PLATFORM(HP)
#if CPU(ARM) && OS(WINCE)
#include <Diagnostics/SysTrace.h>
#endif
#endif

namespace WTF {

#define DEBUG_PERFORMANCE_TRACE 0
#define WRITE_DEBUG_TO_FILE 0

#if DEBUG_PERFORMANCE_TRACE
#include <windows.h>
static int currentDepth = 0;

#if WRITE_DEBUG_TO_FILE
static HANDLE fhandle = 0;
#endif

static void debug_write(const char * data, int len) {
#if WRITE_DEBUG_TO_FILE
    DWORD written = 0;
    ::WriteFile(fhandle, data, len, &written, NULL);
#else
#if OS(WINCE)
    wchar_t buff[1024];
    if (len > 1023)
        len = 1023;
    for (int i = 0; i < len; i++)
        buff[i] = data[i];
    buff[len] = L'\0';
    OutputDebugStringW(buff);
#else
    OutputDebugStringA(data);
#endif
#endif
}

static const char * getDescription(WTF::PerformanceTrace::Event e) {
    switch (e) {
        case WTF::PerformanceTrace::FireEventListeners:
            return "Fire Event Listeners";
        case WTF::PerformanceTrace::EventDispatch:
            return "Event Dispatch";
        case WTF::PerformanceTrace::InputEvent:
            return "InputEvent";
        case WTF::PerformanceTrace::WindowsMessage:
            return "WindowsMessage";
        case WTF::PerformanceTrace::ExecuteJavaScript:
            return "JavaScript";
        case WTF::PerformanceTrace::RecalcStyle:
            return "RecalcStyle";
        case WTF::PerformanceTrace::Layout:
            return "Layout";
        case WTF::PerformanceTrace::Paint:
            return "Paint";
        case WTF::PerformanceTrace::Other:
            return "Other";
        case WTF::PerformanceTrace::Unknown:
            return "Unknown";
        case WTF::PerformanceTrace::Timer:
            return "Timer";
        case WTF::PerformanceTrace::WebSocketData:
            return "WebSocketData";
        default:
            return "???";
    }
}

static void debug_output(bool start, int slot, int depth, PerformanceTimestamp time, PerformanceTrace::Event id, const char * text)
{
    int len;
    char depthBuffer[33];
    char msg[256];
    if (depth < 0)
        depth = 0;
    if (depth > 16)
        depth = 16;
    for (int i = 0; i < depth * 2; i++)
        depthBuffer[i] = ' ';
    depthBuffer[depth * 2] = '\0';

#if PLATFORM(HP) && CPU(ARM) && OS(WINCE)
    len = sprintf(msg, "[%s][%03d][%04d][%lu] %s%s%s%s%s\n", start ? "START" : "  END", slot, depth, time, depthBuffer, getDescription(id), text ? " (" : "", text ? text : "", text ? ")" : "");
#else
    len = sprintf(msg, "[%s][%03d][%04d][%.2f] %s%s%s%s%s\n", start ? "START" : "  END", slot, depth, time, depthBuffer, getDescription(id), text ? " (" : "", text ? text : "", text ? ")" : "");
#endif
    debug_write(msg, len);
}
#endif

static void resizeEventLog(int newEventLogSize);

struct PerformanceEntry {
    PerformanceTrace::Event id;
    PerformanceTimestamp time;
    PerformanceTimestamp elapsed;
    PerformanceTimestamp unknownElapsed;
    int childCount;
    PerformanceTimestamp childTime;
    bool startEvent;
    char text[64];
};

#define STANDARD_EVENT_LOG_SIZE 1000
static int eventLogSize = STANDARD_EVENT_LOG_SIZE;
static PerformanceEntry * performanceEvents = NULL;

static int next = 0;
static int count = 0;
static PerformanceTrace::PerformanceTraceOutputFunc output = NULL;
static bool enabled = false;

#if PLATFORM(HP) && CPU(ARM) && OS(WINCE)
unsigned long * hwtimer = 0;
static PerformanceTimestamp getTimestampCountdown()
{
    return (0x7FFFFFFF - *hwtimer);
}
static PerformanceTimestamp getTimestampNormal()
{
    return (*hwtimer);
}
getTimestampFunc PerformanceTrace::getTimestamp = getTimestampNormal;
#else
getTimestampFunc PerformanceTrace::getTimestamp = currentTimeMS;
#endif

void PerformanceTrace::initializeTimestampFunc() {
#if PLATFORM(HP) && CPU(ARM) && OS(WINCE)
    if (hwtimer == 0) {
        HP::Common::System::OS::Diagnostics::HPTimerHwStruct timerInfo;
        if (!HP::Common::System::OS::Diagnostics::GetRTTraceTimerInfo(&timerInfo))
            CRASH();
        if (timerInfo.TimerEndCount == 0)
            getTimestamp = getTimestampCountdown;
        else
            getTimestamp = getTimestampNormal;
        HP::Common::System::OS::Diagnostics::GetRTTraceTimer(&hwtimer);
        if (!hwtimer)
            CRASH();
    }
#endif
}

#if RECORD_ALLOCATION_TIME
class TimestampInitializer {
public:
    TimestampInitializer::TimestampInitializer() {
        PerformanceTrace::initializeTimestampFunc();
    }
};
static TimestampInitializer initTimestamp;
#endif

void PerformanceTrace::enable(bool e) {
    if (enabled != e) {
        enabled = e;
        if (enabled) { // when re-enabling, reset back to 0.
#if PLATFORM(HP) && DEBUG_PERFORMANCE_TRACE && WRITE_DEBUG_TO_FILE
#if OS(WINCE)
            fhandle = ::CreateFile(L"\\Customer\\Browser\\performance-debug.txt", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
#else
            fhandle = ::CreateFile(L"performance-debug.txt", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
#endif
            if (!fhandle)
                HP_FATAL_ERROR(HP_FATAL_ERROR_ASSERT_FAILURE);
#endif
            next = 0;
            count = 0;
            initializeTimestampFunc();
        }
        resizeEventLog(enabled ? STANDARD_EVENT_LOG_SIZE : 0);
    }
}

static PerformanceEntry & getEvent(int n) {
    return performanceEvents[(next - count + eventLogSize + n) % eventLogSize];
}

void PerformanceTrace::start(Event id, const char * description) {
    if (!enabled)
        return;
    PerformanceEntry & current = performanceEvents[next];
    current.id = id;
    current.time = getTimestamp();
    current.startEvent = true;
    current.childCount = 0;
    current.childTime = 0;
    if (description) {
        strncpy(current.text, description, sizeof(current.text));
        current.text[sizeof(current.text) - 1] = 0;
    }
    else
        current.text[0] = 0;
    count++;
#if DEBUG_PERFORMANCE_TRACE
    debug_output(true, next, currentDepth, current.time, current.id, current.text);
    currentDepth++;
#endif
    next = (next + 1) % eventLogSize;
    if (count == eventLogSize)
        PerformanceTrace::dumpEvents();
}

void PerformanceTrace::end(Event id, const char * description) {
    if (!enabled)
        return;
    PerformanceEntry & current = performanceEvents[next];
    current.id = id;
    current.time = getTimestamp();
    current.startEvent = false;
    if (description) {
        strncpy(current.text, description, sizeof(current.text));
        current.text[sizeof(current.text) - 1] = 0;
    }
    else
        current.text[0] = 0;
    count++;
#if DEBUG_PERFORMANCE_TRACE
    currentDepth--;
    debug_output(false, next, currentDepth, current.time, current.id, current.text);
#endif
    next = (next + 1) % eventLogSize;
    if (count == eventLogSize)
        dumpEvents();
}

void PerformanceTrace::setOutputHandler(PerformanceTraceOutputFunc function) {
    output = function;
}

void PerformanceTrace::dumpEvents() {
#if DEBUG_PERFORMANCE_TRACE
    char msgbuff[128];
    int len = sprintf(msgbuff, "Attempting to flush performance events.  Count=%d, next=%d, eventLogSize=%d, enabled=%s\n", count, next, eventLogSize, enabled ? "true" : "false");
    debug_write(msgbuff, len);
#endif
    if (!enabled)
        return;
    int depth = 0;
    int lastMatchingEnd = -1;
    // before we dump, we need to find the last end event with a matching start event.
    for (int i = 0; i < count; i++) {
        PerformanceEntry & current = getEvent(i);
        if (current.startEvent)
            depth++;
        else {
            depth--;
            if (depth == 0)
                lastMatchingEnd = i;
        }
    }
    // change all the times to be elapsed times.
    const int maxDepth = 20;
    int startEvent[maxDepth];
    depth = 0;
    for (int i = 0; i <= lastMatchingEnd; i++) {
        PerformanceEntry & current = getEvent(i);
        if (current.startEvent) {
            startEvent[depth] = i;
            depth++;
            ASSERT(depth <= maxDepth);
        }
        else {
            depth--;
            PerformanceEntry & start = getEvent(startEvent[depth]);
            PerformanceTimestamp eventElapsed = current.time - start.time;
            start.elapsed = eventElapsed;
            // If the end event has a description (rare), append it to the start event text.
            if (*(current.text)) {
                strncat(start.text, current.text, 64);
                start.text[sizeof(start.text) - 1] = 0;
            }
            if (depth > 0) {
                getEvent(startEvent[depth-1]).childCount++;
                getEvent(startEvent[depth-1]).childTime += eventElapsed;
            }
        }
        // capture unknown time before this event started.
        current.unknownElapsed = 0;
        if (i > 0 && !(getEvent(i - 1).startEvent && !current.startEvent)) {
            // we are not transitioning between a start to end event,
            // opportunity for unknown time exists.
            current.unknownElapsed = current.time - getEvent(i - 1).time;
        }
    }

    depth = 0;
    // print out data at all start events now
    for (int i = 0; i <= lastMatchingEnd; i++) {
        PerformanceEntry & current = getEvent(i);
        if (current.unknownElapsed > 0.01) {
            output(depth, getEvent(i - 1).time, current.unknownElapsed, Unknown, NULL);
        }
        if (current.startEvent) {
            output(depth, current.time, current.elapsed, current.id, current.text);
            depth++;
        }
        else {
            depth--;
        }
    }

    if (lastMatchingEnd > 0) {
        int consumed = lastMatchingEnd + 1;
        count -= consumed;
#if DEBUG_PERFORMANCE_TRACE
        len = sprintf(msgbuff, "Count flushed: %d, last matching end event=%d, count=%d\n", consumed, lastMatchingEnd, count);
        debug_write(msgbuff, len);
#endif
    }

    if (count == eventLogSize) {
        // The log is full and we cannot flush it.
        // This happens when one event has more than eventLogSize children events (drag and drop is a good example).
        // Double the event log size.
        resizeEventLog(eventLogSize * 2);
    }
    else if (count < STANDARD_EVENT_LOG_SIZE) {
        // count should always be less than STANDARD_EVENT_LOG_SIZE in this case...
        resizeEventLog(STANDARD_EVENT_LOG_SIZE);
    }

#if DEBUG_PERFORMANCE_TRACE
#if WRITE_DEBUG_TO_FILE
    if (fhandle)
        FlushFileBuffers(fhandle);
#endif
#endif
}

unsigned PerformanceTrace::TimestampFrequency()
{
#if PLATFORM(HP) && CPU(ARM) && OS(WINCE)
    HP::Common::System::OS::Diagnostics::HPTimerHwStruct timerInfo;
    if (!HP::Common::System::OS::Diagnostics::GetRTTraceTimerInfo(&timerInfo))
        CRASH();
    return timerInfo.TimerFrequencyHz;
#else
    return 1000; // 1000 miliseconds / second
#endif
}

static void resizeEventLog(int newEventLogSize) {
#if DEBUG_PERFORMANCE_TRACE
    char msgbuff[128];
    int len = sprintf(msgbuff, "Resizing performance event log.  count=%d, eventLogSize=%d, newEventLogSize=%d\n", count, eventLogSize, newEventLogSize);
    debug_write(msgbuff, len);
#endif
    // resize the log, moving existing events to the beginning of the new log.
    ASSERT((!enabled && newEventLogSize == 0) || count <= newEventLogSize);
    PerformanceEntry * newPerformanceEvents = NULL;
    if (newEventLogSize > 0) {
        newPerformanceEvents = (PerformanceEntry*) fastMalloc(sizeof(PerformanceEntry) * newEventLogSize);
    }

    if (count > 0 && newPerformanceEvents) {
        int firstEvent = (next - count + eventLogSize) % eventLogSize;
        if (firstEvent + count > eventLogSize) {
            // the log wraps.
            int firstBlockCount = eventLogSize - firstEvent;
            int secondBlockCount = count - firstBlockCount;
            memcpy(newPerformanceEvents, performanceEvents + firstEvent, sizeof(PerformanceEntry) * firstBlockCount);
            memcpy(newPerformanceEvents + firstBlockCount, performanceEvents, sizeof(PerformanceEntry) * secondBlockCount);
        }
        else {
            // the current log entries are in one contiguous block of memory
            memcpy(newPerformanceEvents, performanceEvents + firstEvent, sizeof(PerformanceEntry) * count);
        }
        next = count;
    }

    eventLogSize = newEventLogSize;
    PerformanceEntry * old = performanceEvents;
    performanceEvents = newPerformanceEvents;
    fastFree(old);
}

}
#endif
