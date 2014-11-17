/*
 * Copyright (C) 2014 Hewlett-Packard Company
 * Copyright (C) 2014 Hewlett-Packard Development Company, L.P.
 */

#include "config.h"
#include "ScrollbarThemeWindjammer.h"

#include "GraphicsContext.h"
#include "PlatformMouseEvent.h"
#include "Scrollbar.h"
#include "Frame.h"
#include "FrameView.h"
#include "Page.h"

using namespace std;

namespace WebCore {

// Constants used to figure the drag rect outside which we should snap the
// scrollbar thumb back to its origin.  These calculations are based on
// observing the behavior of the MSVC8 main window scrollbar + some
// guessing/extrapolation.
static const int kOffEndMultiplier = 3;
static const int kOffSideMultiplier = 8;

static const int SCROLL_VERTICAL = 0;
static const int SCROLL_HORIZONTAL = 1;
static const int SCROLL_FORWARD = 0;
static const int SCROLL_BACKWARD = 1;
static const int BTN_NORMAL = 0;
static const int BTN_DOWN = 1;
static const int BTN_DISABLED = 2;

// This is the number of pixels around the scrollbar.
// this area can be filled with a color on WJ2.6/1.5
static int scrollbarPadding = 4;
// This is the number of pixels between the thumb and the 
// button.
static int paddingBetweenThumbAndButton = 3;
// A 'tight' scrollbar, like you have on winxp, would 
// have 0 for both these values.

//[vertical/horizontal][forward/backward][up/down/disabled]
static RefPtr<Image> buttons[2][2][3];
static RefPtr<Image> leftThumb;
static RefPtr<Image> rightThumb;
static RefPtr<Image> topThumb;
static RefPtr<Image> bottomThumb;
static RefPtr<Image> trackCapH;
static RefPtr<Image> trackCapV;

static PassRefPtr<Image> ButtonImage(ScrollbarThemeClient* scrollbar, ScrollbarPart part)
{
    if (part != BackButtonStartPart && part != ForwardButtonEndPart)
        return NULL;
    bool horizontal = scrollbar->orientation() == HorizontalScrollbar;
    bool forward = (part == BackButtonStartPart);
    int state = scrollbar->enabled() ? scrollbar->pressedPart() == part : 2;
    return buttons[horizontal][forward][state];
}

ScrollbarThemeWindjammer::ScrollbarThemeWindjammer()
    : wjStyle(WJStyleUndefined)
{
    setWindjammerStyle(WJ26);
}

void ScrollbarThemeWindjammer::setWindjammerStyle(WindjammerStyle style)
{
    if (style == wjStyle)
        return;
    if (wjStyle == WJ15 || wjStyle == WJ26) {
        for (int fb = 0; fb < 2; fb++)
            for (int updn = 0; updn < 2; updn++)
                for (int state = 0; state < 3; state++)
                    buttons[fb][updn][state] = 0;
        leftThumb = 0;
        rightThumb = 0;
        topThumb = 0;
        bottomThumb = 0;
    }
    else if (wjStyle == WJ4Line) {
        trackCapH = 0;
        trackCapV = 0;
    }

    wjStyle = style; 

    switch (wjStyle) {
    case WJ15:
    case WJ26:
        buttons[SCROLL_VERTICAL][SCROLL_FORWARD][BTN_NORMAL] = Image::loadPlatformResource("wj_scroll_dn_up");
        buttons[SCROLL_VERTICAL][SCROLL_FORWARD][BTN_DOWN] = Image::loadPlatformResource("wj_scroll_dn_dn");
        buttons[SCROLL_VERTICAL][SCROLL_FORWARD][BTN_DISABLED] = Image::loadPlatformResource("wj_scroll_dn_dis");
        buttons[SCROLL_VERTICAL][SCROLL_BACKWARD][BTN_NORMAL] = Image::loadPlatformResource("wj_scroll_up_up");
        buttons[SCROLL_VERTICAL][SCROLL_BACKWARD][BTN_DOWN] = Image::loadPlatformResource("wj_scroll_up_dn");
        buttons[SCROLL_VERTICAL][SCROLL_BACKWARD][BTN_DISABLED] = Image::loadPlatformResource("wj_scroll_up_dis");
        buttons[SCROLL_HORIZONTAL][SCROLL_FORWARD][BTN_NORMAL] = Image::loadPlatformResource("wj_scroll_right_up");
        buttons[SCROLL_HORIZONTAL][SCROLL_FORWARD][BTN_DOWN] = Image::loadPlatformResource("wj_scroll_right_dn");
        buttons[SCROLL_HORIZONTAL][SCROLL_FORWARD][BTN_DISABLED] = Image::loadPlatformResource("wj_scroll_right_dis");
        buttons[SCROLL_HORIZONTAL][SCROLL_BACKWARD][BTN_NORMAL] = Image::loadPlatformResource("wj_scroll_left_up");
        buttons[SCROLL_HORIZONTAL][SCROLL_BACKWARD][BTN_DOWN] = Image::loadPlatformResource("wj_scroll_left_dn");
        buttons[SCROLL_HORIZONTAL][SCROLL_BACKWARD][BTN_DISABLED] = Image::loadPlatformResource("wj_scroll_left_dis");
        leftThumb = Image::loadPlatformResource("wj_scroll_thumb_left");
        rightThumb = Image::loadPlatformResource("wj_scroll_thumb_right");
        topThumb = Image::loadPlatformResource("wj_scroll_thumb_top");
        bottomThumb = Image::loadPlatformResource("wj_scroll_thumb_bottom");
        scrollbarPadding = (wjStyle == WJ15 ? 6 : 4);
        paddingBetweenThumbAndButton = (wjStyle == WJ15 ? 4 : 3);
        break;
    case WJ4Line:        
        trackCapH = Image::loadPlatformResource("wj_scroll_track_4line_h");
        trackCapV = Image::loadPlatformResource("wj_scroll_track_4line_v");
    default:
        break;
    }
}

ScrollbarThemeWindjammer::~ScrollbarThemeWindjammer()
{
}

int ScrollbarThemeWindjammer::scrollbarThickness(ScrollbarControlSize)
{
    switch (wjStyle) {
        case WJ15:
        case WJ26:
            return 39 + scrollbarPadding * 2;
        case WJ4Line:
            return 6;
        default:
            return 0;
    }
}

int ScrollbarThemeWindjammer::buttonSize(int scrollbarLength)
{
    // scrollbar thickness is by definition the size of the button plus padding (on both sides of the button)
    // to get the button size by itself we subtract the padding from the thickness
    int standardButtonLen = (scrollbarThickness() - scrollbarPadding * 2);
    
    // in order for buttons to be drawn full size, there has to be enough room to draw the buttons and the padding
    return scrollbarLength < 2 * (standardButtonLen + scrollbarPadding) ? (scrollbarLength - scrollbarPadding * 2 + 1) / 2 : standardButtonLen;
}

bool ScrollbarThemeWindjammer::hasThumb(ScrollbarThemeClient * scrollbar)
{
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeWindjammer::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == BackButtonEndPart || wjStyle == WJ4Line)
        return IntRect();

    // This method returns the exact position and size where the button 
    // should be drawn, taking into account padding.
    int stdButtonSize = scrollbarThickness() - 2 * scrollbarPadding;
    IntPoint buttonOrigin(scrollbar->x() + scrollbarPadding, scrollbar->y() + scrollbarPadding);
    if (scrollbar->orientation() == HorizontalScrollbar) {
        return IntRect(buttonOrigin, IntSize(buttonSize(scrollbar->width()), stdButtonSize));
    }
    else {
        return IntRect(buttonOrigin, IntSize(stdButtonSize, buttonSize(scrollbar->height())));
    }
}

IntRect ScrollbarThemeWindjammer::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == ForwardButtonStartPart || wjStyle == WJ4Line)
        return IntRect();

    // This method returns the exact position and size where the button 
    // should be drawn, taking into account padding.
    int stdButtonSize = scrollbarThickness() - 2 * scrollbarPadding;
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int width = buttonSize(scrollbar->width());
        return IntRect(scrollbar->x() + scrollbar->width() - scrollbarPadding - width, 
                       scrollbar->y() + scrollbarPadding, 
                       width, 
                       stdButtonSize);
    }
    int height = buttonSize(scrollbar->height());
    return IntRect(scrollbar->x() + scrollbarPadding, 
                   scrollbar->y() + scrollbar->height() - scrollbarPadding - height, 
                   stdButtonSize, 
                   height);
}

// This is the portion of the scrollbar between the buttons; basically the 
// scroll bar background or "track"
IntRect ScrollbarThemeWindjammer::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    if (wjStyle == WJ4Line) {
        IntRect trackRect(scrollbar->frameRect());
        if (scrollbar->orientation() == HorizontalScrollbar)
            trackRect.inflateX(-1);
        else
            trackRect.inflateY(-1);
        return trackRect;
    }

    int thickness = scrollbarThickness();
    int minLengthForTrack = (thickness - scrollbarPadding + paddingBetweenThumbAndButton) * 2;

    if (scrollbar->orientation() == HorizontalScrollbar) {
        if (scrollbar->width() < minLengthForTrack)
            return IntRect();
        return IntRect(scrollbar->x() + thickness - scrollbarPadding + paddingBetweenThumbAndButton, 
                       scrollbar->y(), 
                       scrollbar->width() - 2 * (thickness - scrollbarPadding + paddingBetweenThumbAndButton), 
                       thickness);
    }
    if (scrollbar->height() < minLengthForTrack)
        return IntRect();
    return IntRect(scrollbar->x(), 
                   scrollbar->y() + thickness - scrollbarPadding + paddingBetweenThumbAndButton, 
                   thickness, 
                   scrollbar->height() - 2 * (thickness - scrollbarPadding + paddingBetweenThumbAndButton));
}

bool ScrollbarThemeWindjammer::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& evt)
{
    //return evt.shiftKey() && evt.button() == LeftButton;
    return evt.button() == LeftButton; // instead of requiring shift button to be pressed, always go to wherever the user clicked on the scrollbar.
}

bool ScrollbarThemeWindjammer::shouldSnapBackToDragOrigin(ScrollbarThemeClient* scrollbar, const PlatformMouseEvent& evt)
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

void ScrollbarThemeWindjammer::paintScrollbarBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar)
{
    StrokeStyle sstyle = context->strokeStyle();

    if (wjStyle == WJ4Line) {
        IntRect r(scrollbar->frameRect());
        Color background(0xe6, 0xe6, 0xe6);
        Color track(0xcc, 0xcc, 0xcc);
        context->setStrokeStyle(SolidStroke);
        context->setStrokeThickness(1);  
        context->setFillColor(track, ColorSpaceDeviceRGB);
        context->setStrokeColor(background, ColorSpaceDeviceRGB);
        context->drawRect(r);
        if (scrollbar->orientation() == VerticalScrollbar) {
            IntPoint p(r.minXMinYCorner().x(), r.minXMinYCorner().y() + 1);
            context->drawImage(trackCapV.get(), ColorSpaceDeviceRGB, p);
            p.setY(r.minXMaxYCorner().y() - 1);
            context->drawImage(trackCapV.get(), ColorSpaceDeviceRGB, p);
        }
        else {
            IntPoint p(r.minXMinYCorner().x() + 1, r.minXMinYCorner().y());
            context->drawImage(trackCapH.get(), ColorSpaceDeviceRGB, p);
            p.setX(r.maxXMinYCorner().x() - 1);
            context->drawImage(trackCapH.get(), ColorSpaceDeviceRGB, p);
        }            
    }
    else
    {
        /*
        FrameView* frameView = static_cast<FrameView*>(scrollbar->parent());
        if (frameView) {
            Page* page = frameView->frame() ? frameView->frame()->page() : 0;
            if (page) {
                ChromeClient* chromeClient = page->chrome()->client();
                if (chromeClient->scrollBarBackgroundColorSet()) {
                    context->fillRect(IntRect(scrollbar->frameRect()), Color(chromeClient->scrollBarBackgroundColor()));
                }
            }
        }
        */
        IntRect r = trackRect(scrollbar);
        if (!r.isEmpty()) {
            if (scrollbar->orientation() == VerticalScrollbar) {
                r.inflateX(-scrollbarPadding);
                r.inflateY(paddingBetweenThumbAndButton);
            }
            else {
                r.inflateY(-scrollbarPadding);
                r.inflateX(paddingBetweenThumbAndButton);
            }
            context->fillRect(r, Color(0xcc, 0xcc, 0xcc), ColorSpaceDeviceRGB);
        }
    }
    context->setStrokeStyle(sstyle);
}

void ScrollbarThemeWindjammer::paintScrollCorner(ScrollView* view, GraphicsContext* context, const IntRect& cornerRect)
{
    /*
    FrameView* frameView = static_cast<FrameView*>(view);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (page) {
        ChromeClient* chromeClient = page->chrome()->client();
        if (chromeClient->scrollBarBackgroundColorSet()) {
            context->fillRect(cornerRect, Color(chromeClient->scrollBarBackgroundColor()), ColorSpaceDeviceRGB);
        }
        else {
            context->fillRect(cornerRect, Color::white, ColorSpaceDeviceRGB);
        }
    }*/
    context->fillRect(cornerRect, Color::white, ColorSpaceDeviceRGB);
}


void ScrollbarThemeWindjammer::paintButton(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect, ScrollbarPart part)
{
    if (wjStyle == WJ4Line)
        return;
    RefPtr<Image> img = ButtonImage(scrollbar, part);
    if (img) {
        IntPoint pt(rect.x(), rect.y());
        bool clipped = (scrollbar->orientation() == HorizontalScrollbar ? rect.width() < img->width() : rect.height() < img->height());
        if (clipped) {
            IntSize destSize;
            if (scrollbar->orientation() == VerticalScrollbar)
                destSize = IntSize(img->width(), buttonSize(scrollbar->height()));
            else
                destSize = IntSize(buttonSize(scrollbar->width()), img->height());
            FloatRect destRect = FloatRect(IntRect(pt,destSize));
            FloatRect srcRect = FloatRect(0, 0, img->width(), img->height());
            context->drawImage(img.get(), ColorSpaceDeviceRGB, destRect, srcRect);
        }
        else {
            context->drawImage(img.get(), ColorSpaceDeviceRGB, pt);
        }
    }
}

void ScrollbarThemeWindjammer::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    if (wjStyle == WJ4Line) {
        Color thumbColor(0x64, 0x64, 0x64);
        context->setStrokeStyle(SolidStroke);
        context->setFillColor(thumbColor, ColorSpaceDeviceRGB);
        context->setStrokeColor(thumbColor, ColorSpaceDeviceRGB);
        IntRect thumbRect(rect);

        if (scrollbar->orientation() == VerticalScrollbar) {
            IntPoint p1(thumbRect.x()+2, thumbRect.y());
            IntPoint p2(thumbRect.maxX()-2, thumbRect.y());
            context->drawLine(p1,p2);
            p1.setY(thumbRect.maxY()-1);
            p2.setY(thumbRect.maxY()-1);
            context->drawLine(p1,p2);
            thumbRect.inflateX(-1);
            thumbRect.inflateY(-1);
        }
        else {
            IntPoint p1(thumbRect.x(), thumbRect.y()+2);
            IntPoint p2(thumbRect.x(), thumbRect.maxY()-2);
            context->drawLine(p1,p2);
            p1.setX(thumbRect.maxX()-1);
            p2.setX(thumbRect.maxX()-1);
            context->drawLine(p1,p2);
            thumbRect.inflateY(-1);
            thumbRect.inflateX(-1);
        }
        context->drawRect(thumbRect);
    }
    else {
        Color fill(0x82, 0x82, 0x82);
        Color innerBorder(0xa1, 0xa1, 0xa1);
        Color white(0xFF, 0xFF, 0xFF);

        IntRect fillRect(rect);
        context->setStrokeStyle(SolidStroke);
        context->setStrokeThickness(1);
        context->setFillColor(fill, ColorSpaceDeviceRGB);
        context->setStrokeColor(innerBorder, ColorSpaceDeviceRGB);

        if (scrollbar->orientation() == VerticalScrollbar) {
            int trackPadding = (rect.width() - scrollbarPadding * 2 - topThumb->width()) / 2;
            fillRect.inflateY(-2);
            fillRect.inflateX(-trackPadding - scrollbarPadding - 1);
            context->drawRect(fillRect);
            IntPoint p1(fillRect.x() - 1, fillRect.y());
            IntPoint p2(fillRect.x() - 1, fillRect.maxY());
            context->setStrokeColor(white, ColorSpaceDeviceRGB);
            context->drawLine(p1, p2);
            p1.setX(fillRect.maxX());
            p2.setX(fillRect.maxX());
            context->drawLine(p1, p2);
            IntPoint p(rect.x() + scrollbarPadding + trackPadding, rect.y());
            context->drawImage(topThumb.get(), ColorSpaceDeviceRGB, p);
            p.setY(rect.y() + rect.height() - bottomThumb->height());
            context->drawImage(bottomThumb.get(), ColorSpaceDeviceRGB, p);
        }
        else {
            int trackPadding = (rect.height() - scrollbarPadding * 2 - leftThumb->height()) / 2;
            fillRect.inflateX(-2);
            fillRect.inflateY(-trackPadding - scrollbarPadding - 1);
            context->drawRect(fillRect);
            IntPoint p1(fillRect.x(), fillRect.y() - 1);
            IntPoint p2(fillRect.maxX(), fillRect.y() - 1);
            context->setStrokeColor(white, ColorSpaceDeviceRGB);
            context->drawLine(p1, p2);
            p1.setY(fillRect.maxY());
            p2.setY(fillRect.maxY());
            context->drawLine(p1, p2);
            IntPoint p(rect.x(), rect.y() + scrollbarPadding + trackPadding);
            context->drawImage(leftThumb.get(), ColorSpaceDeviceRGB, p);
            p.setX(rect.x() + rect.width() - rightThumb->width());
            context->drawImage(rightThumb.get(), ColorSpaceDeviceRGB, p);
        }
    }
}

}
