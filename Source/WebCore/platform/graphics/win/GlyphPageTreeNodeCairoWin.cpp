/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GlyphPageTreeNode.h"

#include "SimpleFontData.h"

namespace WebCore {

bool GlyphPage::fill(unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    // bufferLength will be greater than the requested number of glyphs if the buffer contains surrogate pairs.
    // We won't support this for now.
    if (bufferLength > length)
        return false;

#if OS(WINCE)
    // WinCE does not have GetGlyphIndices.  The closest it has is provided through
    // the IMLangFontLink interface, which is only present if IE is installed.
    // http://msdn.microsoft.com/en-us/library/ee492017(v=winembedded.60).aspx
    // Our 'glyph' index will simply be the character index.  When cairo draws text
    // in WinCE, it assumes the glyph index is a character index.
    // It would be better to add a feature to webkit - something like GLYPH_LOOKUP,
    // which would enable the whole glyph page stuff to be stubbed out.
    for (unsigned i = 0; i < length; ++i)
        setGlyphDataForIndex(offset + i, buffer[i], fontData);
    return true;
#else
    bool haveGlyphs = false;

    HDC dc = GetDC((HWND)0);
    SaveDC(dc);
    SelectObject(dc, fontData->platformData().hfont());

    WORD localGlyphBuffer[GlyphPage::size * 2];
    DWORD result = GetGlyphIndices(dc, buffer, bufferLength, localGlyphBuffer, 0);
    bool success = result != GDI_ERROR && static_cast<unsigned>(result) == bufferLength;
    if (success) {
        for (unsigned i = 0; i < length; i++) {
            Glyph glyph = localGlyphBuffer[i];
            if (!glyph)
                setGlyphDataForIndex(offset + i, 0, 0);
            else {
                setGlyphDataForIndex(offset + i, glyph, fontData);
                haveGlyphs = true;
            }
        }
    }
    RestoreDC(dc, -1);
    ReleaseDC(0, dc);

    return haveGlyphs;
#endif
}

}
