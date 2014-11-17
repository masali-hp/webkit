/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScrollbarThemeOpus.h"

#include "GraphicsContext.h"
#include "PlatformMouseEvent.h"
#include "Scrollbar.h"
#include <wtf/HashMap.h>

using namespace std;

WTF::HashMap<WebCore::ScrollbarThemeClient*,int> * clientOpacityMap = NULL;

namespace WebCore {

#define SCROLLBAR_THICKNESS 7

// Constants used to figure the drag rect outside which we should snap the
// scrollbar thumb back to its origin.  These calculations are based on
// observing the behavior of the MSVC8 main window scrollbar + some
// guessing/extrapolation.
static const int kOffEndMultiplier = 3;
static const int kOffSideMultiplier = 8;

ScrollbarThemeOpus::ScrollbarThemeOpus()
{
}

ScrollbarThemeOpus::~ScrollbarThemeOpus()
{
}

int ScrollbarThemeOpus::scrollbarThickness(ScrollbarControlSize)
{
    return SCROLLBAR_THICKNESS;
}

bool ScrollbarThemeOpus::hasThumb(ScrollbarThemeClient* scrollbar)
{
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeOpus::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    return IntRect();
}

IntRect ScrollbarThemeOpus::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    return IntRect();
}

IntRect ScrollbarThemeOpus::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar) {
        if (scrollbar->width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar->x(), scrollbar->y(), scrollbar->width(), thickness);
    }
    if (scrollbar->height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y(), thickness, scrollbar->height());
}

bool ScrollbarThemeOpus::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& evt)
{
    return evt.shiftKey() && evt.button() == LeftButton;
}

bool ScrollbarThemeOpus::shouldSnapBackToDragOrigin(ScrollbarThemeClient* scrollbar, const PlatformMouseEvent& evt)
{
    // Find the rect within which we shouldn't snap, by expanding the track rect
    // in both dimensions.
    IntRect rect = trackRect(scrollbar);
    const bool horz = scrollbar->orientation() == HorizontalScrollbar;
    const int thickness = scrollbarThickness(scrollbar->controlSize());
    rect.inflateX((horz ? kOffEndMultiplier : kOffSideMultiplier) * thickness);
    rect.inflateY((horz ? kOffSideMultiplier : kOffEndMultiplier) * thickness);

    // Convert the event to local coordinates.
    IntPoint mousePosition = scrollbar->convertFromContainingWindow(evt.position());
    mousePosition.move(scrollbar->x(), scrollbar->y());

    // We should snap iff the event is outside our calculated rect.
    return !rect.contains(mousePosition);
}

void ScrollbarThemeOpus::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    if (!clientOpacityMap->contains(scrollbar))
        return;
    int opacity = clientOpacityMap->get(scrollbar);
    if (!opacity)
        return;
    IntRect thumbRect = rect;
    thumbRect.inflate(-1);
    int scrollThickness = thumbRect.width() < thumbRect.height() ? thumbRect.width() : thumbRect.height();
    IntSize curveSize(scrollThickness / 2, scrollThickness / 2);
    Color fillColor(makeRGBA(128, 128, 128, opacity));
    context->fillRoundedRect(thumbRect, curveSize, curveSize, curveSize, curveSize, fillColor, ColorSpaceDeviceRGB);
}

void ScrollbarThemeOpus::registerScrollbar(ScrollbarThemeClient* scrollbar)
{
    if (!clientOpacityMap)
        clientOpacityMap = new WTF::HashMap<ScrollbarThemeClient*,int>();
    clientOpacityMap->add(scrollbar, 0);
}

void ScrollbarThemeOpus::unregisterScrollbar(ScrollbarThemeClient* scrollbar)
{
    clientOpacityMap->remove(scrollbar);
}

bool ScrollbarThemeOpus::setThumbOpacity(ScrollbarThemeClient* scrollbar, int opacity)
{
    if (!clientOpacityMap)
        return false;
    if (!clientOpacityMap->contains(scrollbar))
        return false;
    clientOpacityMap->set(scrollbar, opacity);
    return true;
}

}
