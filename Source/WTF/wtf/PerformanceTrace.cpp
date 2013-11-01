#include "config.h"

#if ENABLE(PERFORMANCE_TRACING)
#include "PerformanceTrace.h"
#include "CurrentTime.h"

#include <string.h>

namespace WTF {

struct PerformanceEntry {
    PerformanceTrace::Event id;
    double time;
    double elapsed;
    double unknownElapsed;
    int childCount;
    double childTime;
    bool startEvent;
    char text[64];
};

static const int eventLogSize = 1000;
static PerformanceEntry performanceEvents[eventLogSize];

static int next = 0;
static int count = 0;
static PerformanceTrace::PerformanceTraceOutputFunc output = NULL;
static bool enabled = false;

void PerformanceTrace::enable(bool e) {
    if (enabled != e) {
        enabled = e;
        if (enabled) { // when re-enabling, reset back to 0.
            next = 0;
            count = 0;
        }
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
    current.time = currentTimeMS();
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
    next = (next + 1) % eventLogSize;
    if (count == eventLogSize)
        PerformanceTrace::dumpEvents();
}

void PerformanceTrace::end(Event id) {
    if (!enabled)
        return;
    PerformanceEntry & current = performanceEvents[next];
    current.id = id;
    current.time = currentTimeMS();
    current.startEvent = false;
    count++;
    next = (next + 1) % eventLogSize;
    if (count == eventLogSize)
        dumpEvents();
}

void PerformanceTrace::setOutputHandler(PerformanceTraceOutputFunc function) {
    output = function;
}

void PerformanceTrace::dumpEvents() {
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
            double eventElapsed = current.time - getEvent(startEvent[depth]).time;
            getEvent(startEvent[depth]).elapsed = eventElapsed;
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
    }
}

}
#endif
