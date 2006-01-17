/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2006       Alexander Kellett <lypanov@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_KCanvasRenderingStyle_H
#define KSVG_KCanvasRenderingStyle_H

#include <qvaluelist.h>

#include "css_valueimpl.h"

#include <kcanvas/KCanvasMatrix.h>

// FIXME: these should be removed, use KSVG ones instead
typedef enum
{
    CAP_BUTT = 1,
    CAP_ROUND = 2,
    CAP_SQUARE = 3
} KCCapStyle;

typedef enum
{
    JOIN_MITER = 1,
    JOIN_ROUND = 2,
    JOIN_BEVEL = 3
} KCJoinStyle;


// Special types
typedef Q3ValueList<float> KCDashArray;


namespace khtml {
    class RenderStyle;
}

class KRenderingFillPainter;
class KRenderingStrokePainter;
class KRenderingPaintServer;
class RenderPath;

namespace KSVG
{
    class KSVGPainterFactory
    {
    public:
        static KRenderingFillPainter fillPainter(const khtml::RenderStyle *style, const RenderPath *item);
        static KRenderingStrokePainter strokePainter(const khtml::RenderStyle *style, const RenderPath *item);

        static bool isStroked(const khtml::RenderStyle *style);
        static KRenderingPaintServer *strokePaintServer(const khtml::RenderStyle *style, const RenderPath*);

        static bool isFilled(const khtml::RenderStyle *style);
        static KRenderingPaintServer *fillPaintServer(const khtml::RenderStyle *style, const RenderPath*);

        static double cssPrimitiveToLength(const RenderPath *item, KDOM::CSSValueImpl *value, double defaultValue = 0.0);
    };
};

#endif

// vim:ts=4:noet
