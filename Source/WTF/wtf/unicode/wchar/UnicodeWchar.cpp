/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
 * Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
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
#include "UnicodeWchar.h"

#include <algorithm>

#include "UnicodeCaseTable.cpp"
#include "UnicodeICULite.cpp"

namespace WTF {
namespace Unicode {

CharCategory category(UChar32 c)
{
    uint32_t props;
    GET_PROPS(c, props);
    return static_cast<CharCategory>(TO_MASK((int8_t)(props&0x1f)));
}

unsigned char combiningClass(UChar32 c)
{
    uint32_t norm32;
    UTRIE_GET32(&NormProp::normTrie, c, norm32);
    return (uint8_t)(norm32 >> _NORM_CC_SHIFT);
}

Direction direction(UChar32 c)
{
    uint32_t props;
    GET_BIDI_PROPS(&BidiProp::ubidi_props_singleton, c, props);
    return static_cast<Direction>(UBIDI_GET_CLASS(props));
}

DecompositionType decompositionType(UChar32 c)
{
    // see u_getIntPropertyValue in uprops.c
    //return static_cast<DecompositionType>(u_getIntPropertyValue(c, UCHAR_DECOMPOSITION_TYPE));
    return static_cast<DecompositionType>(u_getUnicodeProperties(c, 2) & UPROPS_DT_MASK);
}

bool hasLineBreakingPropertyComplexContext(UChar32 c)
{
    // see u_getIntPropertyValue in uprops.c
    //return u_getIntPropertyValue(c, UCHAR_LINE_BREAK) == U_LB_COMPLEX_CONTEXT;
    return (int32_t)(u_getUnicodeProperties(c, UPROPS_LB_VWORD)&UPROPS_LB_MASK)>>UPROPS_LB_SHIFT == U_LB_COMPLEX_CONTEXT;
}

UChar32 mirroredChar(UChar32 c)
{
    const BidiProp::UBiDiProps *bdp = &BidiProp::ubidi_props_singleton;

    uint32_t props;
    int32_t delta;

    GET_BIDI_PROPS(bdp, c, props);
    delta = ((int16_t)props) >> UBIDI_MIRROR_DELTA_SHIFT;
    if (delta != UBIDI_ESC_MIRROR_DELTA) {
        return c+delta;
    } else {
        /* look for mirror code point in the mirrors[] table */
        const uint32_t *mirrors = bdp->mirrors;
        int32_t length = bdp->indexes[UBIDI_IX_MIRROR_LENGTH];

        /* linear search */
        for (int32_t i = 0; i < length; ++i) {
            uint32_t m = mirrors[i];
            UChar32 c2 = UBIDI_GET_MIRROR_CODE_POINT(m);
            if (c == c2) {
                /* found c, return its mirror code point using the index in m */
                return UBIDI_GET_MIRROR_CODE_POINT(mirrors[UBIDI_GET_MIRROR_INDEX(m)]);
            } else if (c < c2) {
                break;
            }
        }

        /* c not found, return it itself */
        return c;
    }
}

/*
template<UChar Function(UChar)>
static inline int convertWithFunction(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    UChar* resultIterator = result;
    UChar* resultEnd = result + std::min(resultLength, sourceLength);
    while (resultIterator < resultEnd)
        *resultIterator++ = Function(*source++);

    if (sourceLength < resultLength)
        *resultIterator = '\0';

    *isError = sourceLength > resultLength;
    return sourceLength;
}

int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<foldCase>(result, resultLength, source, sourceLength, isError);
}

int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<toLower>(result, resultLength, source, sourceLength, isError);
}

int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<toUpper>(result, resultLength, source, sourceLength, isError);
}
*/

wchar_t toLower(wchar_t c)
{
    //Convert ascii A-Z to a-z and extended ascii A-O to a-o
    //http://www.unicode.org
    if (((c >= 0x41) && (c <= 0x5A)) || ((c >= 0xC0) && (c <= 0xDE)) ||
        ((c >= 0xFF21) && (c <= 0xFF3A))) {
        if (c == 0xD7) {
            return c;
        } else {
            return c+0x20;
        }
    } else if ((c >= 0x100) && (c <=0x17F)) {
        return lcase_tableA[c-0x100];
    } else if ((c >= 0x400) && (c <=0x481)) {
        return lcase_tableB[c-0x400];
    } else if (((c >= 0x531) && (c <=0x556)) ) {
        return c+0x30;
    } else if (c == 0x39C) {
        return 0xB5;
    }
    return c;
}

wchar_t toUpper(wchar_t c)
{
    //Convert ascii a-z to A-Z and extended ascii a-o to A-O
    //http://www.unicode.org
    if (((c >= 0x61) && (c <= 0x7A)) || ((c >= 0xE0) && (c <= 0xFE)) ||
        ((c >= 0xFF41) && (c <=0xFF5A)) ) {
        if (c == 0xF7) {
            return c;
        } else {
            return c-0x20;
        }
    } else if ((c >= 0x100) && (c <=0x17F)) {
        return ucase_tableA[c-0x100];
    } else if ((c == 0xB5)) {
        return 0x39C;
    } else if ((c == 0xFF)) {
        return 0x178;
    } else if (((c >= 0x561) && (c <=0x586))) {
        return c-0x30;
    } else if ((c >= 0x400) && (c <=0x481)) {
        return ucase_tableB[c-0x400];
    }  else if (c == 0x587) {
        return 0x535;
    }

    return c;
}

wchar_t foldCase(wchar_t c)
{
    return toLower(c);
}

bool isPrintableChar(wchar_t c)
{
    uint32_t props;
    GET_PROPS(c, props);
    /* comparing ==0 returns FALSE for the categories mentioned */
    //return (CAT_MASK(props)&U_GC_C_MASK)==0;
    return (U_MASK(GET_CATEGORY(props)) & U_GC_C_MASK)==0;
    //return !!iswprint(c);
}

bool isSpace(wchar_t c)
{
    return !!iswspace(c);
}

bool isLetter(wchar_t c)
{
    return !!iswalpha(c);
}

bool isUpper(wchar_t c)
{
    return !!iswupper(c);
}

bool isLower(wchar_t c)
{
    return !!iswlower(c);
}

bool isDigit(wchar_t c)
{
    return !!iswdigit(c);
}

bool isPunct(wchar_t c)
{
    uint32_t props;
    GET_PROPS(c, props);
    /* comparing ==0 returns FALSE for the categories mentioned */
    return (UBool)((CAT_MASK(props)&U_GC_C_MASK)==0);
    //return !!iswpunct(c);
}

int toLower(wchar_t* result, int resultLength, const wchar_t* src, int srcLength,  bool* error)
{
    const UChar *e = src + srcLength;
    const UChar *s = src;
    UChar *r = result;
    UChar *re = result + resultLength;

    int needed = 0;
    if (srcLength <= resultLength)
        while (s < e)
            *r++ = toLower(*s++);
    else
        while (r < re)
            *r++ = toLower(*s++);

    if (s < e)
        needed += e - s;
    *error = (needed != 0);
    if (r < re)
        *r = 0;
    return (r - result) + needed;
}

int toUpper(wchar_t* result, int resultLength, const wchar_t* src, int srcLength,  bool* error)
{
    const UChar *e = src + srcLength;
    const UChar *s = src;
    UChar *r = result;
    UChar *re = result + resultLength;

    int needed = 0;
    if (srcLength <= resultLength)
        while (s < e)
            *r++ = toUpper(*s++);
    else
        while (r < re)
            *r++ = toUpper(*s++);

    if (s < e)
        needed += e - s;
    *error = (needed != 0);
    if (r < re)
        *r = 0;
    return (r - result) + needed;
}

int foldCase(wchar_t* result, int resultLength, const wchar_t* src, int srcLength,  bool* error)
{
    *error = false;
    if (resultLength < srcLength)
    {
        *error = true;
        return srcLength;
    }
    for (int i = 0; i < srcLength; ++i)
        result[i] = foldCase(src[i]);
    return srcLength;
}

/*
wchar_t toTitleCase(wchar_t c)
{
    return towupper(c);
}
*/

} // namespace Unicode
} // namespace WTF
