/*
 * (C) Copyright 2012 Hewlett-Packard Development Company, L.P.
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of version 2.1 of the GNU Lesser General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to:
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#ifndef ScrollbarThemeWindjammer_h
#define ScrollbarThemeWindjammer_h

#include "ScrollbarThemeComposite.h"
#include "WindjammerTheme.h"

namespace WebCore {

class ScrollbarThemeWindjammer : public ScrollbarThemeComposite {
public:
    ScrollbarThemeWindjammer();
    virtual ~ScrollbarThemeWindjammer();

    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar);
    
    virtual void setWindjammerStyle(WindjammerStyle style);

protected:
    virtual bool hasButtons(ScrollbarThemeClient*) { return true; }
    virtual bool hasThumb(ScrollbarThemeClient*);

    virtual IntRect backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false);
    virtual IntRect forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false);
    virtual IntRect trackRect(ScrollbarThemeClient*, bool painting = false);

    virtual bool shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent&);
    virtual bool shouldSnapBackToDragOrigin(ScrollbarThemeClient*, const PlatformMouseEvent&);

    virtual void paintScrollbarBackground(GraphicsContext*, ScrollbarThemeClient*);
    virtual void paintButton(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart);
    virtual void paintThumb(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);
    virtual void paintScrollCorner(ScrollView*, GraphicsContext*, const IntRect& cornerRect);

    virtual int minimumThumbLength(ScrollbarThemeClient* scrollbar) { return 4; }

    int buttonSize(int scrollbarLength);

	WindjammerStyle wjStyle;
    int scrollbarPadding;
};

}
#endif
