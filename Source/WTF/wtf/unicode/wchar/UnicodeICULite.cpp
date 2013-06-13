/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2007-2008 Torch Mobile, Inc.
 *  Copyright (C) 2010 Crank Software
 *  Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

/********************************************************************************
*   Copyright (C) 1996-2007, International Business Machines Corporation and others. 
********************************************************************************/

// Implemented with ICU code
//
// This file is based on work done by Crank Software, which is based on work done by
// Torch Mobile (see https://bugs.webkit.org/show_bug.cgi?id=27305).  Torch created
// the original slimmed down ICU unicode path, named UnicodeWince.

typedef bool UBool;

enum {
    UTRIE_SHIFT=5,
    UTRIE_DATA_BLOCK_LENGTH=1<<UTRIE_SHIFT,
    UTRIE_MASK=UTRIE_DATA_BLOCK_LENGTH-1,
    UTRIE_LEAD_INDEX_DISP=0x2800>>UTRIE_SHIFT,
    UTRIE_INDEX_SHIFT=2,
    UTRIE_DATA_GRANULARITY=1<<UTRIE_INDEX_SHIFT,
    UTRIE_SURROGATE_BLOCK_BITS=10-UTRIE_SHIFT,
    UTRIE_SURROGATE_BLOCK_COUNT=(1<<UTRIE_SURROGATE_BLOCK_BITS),
    UTRIE_BMP_INDEX_LENGTH=0x10000>>UTRIE_SHIFT
};

typedef int32_t UTrieGetFoldingOffset(uint32_t data);

struct UTrie {
    const uint16_t *index;
    const uint32_t *data32; /* NULL if 16b data is used via index */

    /**
     * This function is not used in _FROM_LEAD, _FROM_BMP, and _FROM_OFFSET_TRAIL macros.
     * If convenience macros like _GET16 or _NEXT32 are used, this function must be set.
     *
     * utrie_unserialize() sets a default function which simply returns
     * the lead surrogate's value itself - which is the inverse of the default
     * folding function used by utrie_serialize().
     *
     * @see UTrieGetFoldingOffset
     */
    UTrieGetFoldingOffset *getFoldingOffset;

    int32_t indexLength, dataLength;
    uint32_t initialValue;
    UBool isLatin1Linear;
};

int32_t utrie_defaultGetFoldingOffset(uint32_t data) {
    return (int32_t)data;
}

#define _UTRIE_GET_RAW(trie, data, offset, c16) \
    (trie)->data[ \
        ((int32_t)((trie)->index[(offset)+((c16)>>UTRIE_SHIFT)])<<UTRIE_INDEX_SHIFT)+ \
        ((c16)&UTRIE_MASK) \
    ]

#define _UTRIE_GET_FROM_PAIR(trie, data, c, c2, result, resultType) { \
    int32_t __offset; \
\
    /* get data for lead surrogate */ \
    (result)=_UTRIE_GET_RAW((trie), data, 0, (c)); \
    __offset=(trie)->getFoldingOffset(result); \
\
    /* get the real data from the folded lead/trail units */ \
    if(__offset>0) { \
        (result)=_UTRIE_GET_RAW((trie), data, __offset, (c2)&0x3ff); \
    } else { \
        (result)=(resultType)((trie)->initialValue); \
    } \
}

#define _UTRIE_GET_FROM_BMP(trie, data, c16) \
    _UTRIE_GET_RAW(trie, data, 0xd800<=(c16) && (c16)<=0xdbff ? UTRIE_LEAD_INDEX_DISP : 0, c16);

#define _UTRIE_GET(trie, data, c32, result, resultType) \
    if((uint32_t)(c32)<=0xffff) { \
        /* BMP code points */ \
        (result)=_UTRIE_GET_FROM_BMP(trie, data, c32); \
    } else if((uint32_t)(c32)<=0x10ffff) { \
        /* supplementary code point */ \
        UChar __lead16=(UChar)(((c32)>>10)+0xd7c0); \
        _UTRIE_GET_FROM_PAIR(trie, data, __lead16, c32, result, resultType); \
    } else { \
        /* out of range */ \
        (result)=(resultType)((trie)->initialValue); \
    }

#define UTRIE_GET16(trie, c16, result) _UTRIE_GET(trie, index, c16, result, uint16_t)
#define UTRIE_GET32(trie, c32, result) _UTRIE_GET(trie, data32, c32, result, uint32_t)

#define GET_PROPS(c, result) UTRIE_GET16(&CharProp::propsTrie, c, result);

typedef uint8_t UVersionInfo[4];

enum {
    UPROPS_INDEX_COUNT=16,

    /* general category shift==0                                0 (5 bits) */
    UPROPS_NUMERIC_TYPE_SHIFT=5,                            /*  5 (3 bits) */
    UPROPS_NUMERIC_VALUE_SHIFT=8                            /*  8 (8 bits) */
};

// From uprops.h:
/*
 * Properties in vector word 2
 * Bits
 * 31..26   reserved
 * 25..20   Line Break
 * 19..15   Sentence Break
 * 14..10   Word Break
 *  9.. 5   Grapheme Cluster Break
 *  4.. 0   Decomposition Type
 */
#define UPROPS_LB_MASK          0x03f00000
#define UPROPS_LB_SHIFT         20
#define UPROPS_LB_VWORD         2

#define UPROPS_SB_MASK          0x000f8000
#define UPROPS_SB_SHIFT         15

#define UPROPS_WB_MASK          0x00007c00
#define UPROPS_WB_SHIFT         10

#define UPROPS_GCB_MASK         0x000003e0
#define UPROPS_GCB_SHIFT        5

#define UPROPS_DT_MASK          0x0000001f

// From uchar.h:
/**
 * Line Break constants.
 *
 * @see UCHAR_LINE_BREAK
 * @stable ICU 2.2
 */
typedef enum ULineBreak {
    U_LB_UNKNOWN = 0,           /*[XX]*/ /*See note !!*/
    U_LB_AMBIGUOUS = 1,         /*[AI]*/
    U_LB_ALPHABETIC = 2,        /*[AL]*/
    U_LB_BREAK_BOTH = 3,        /*[B2]*/
    U_LB_BREAK_AFTER = 4,       /*[BA]*/
    U_LB_BREAK_BEFORE = 5,      /*[BB]*/
    U_LB_MANDATORY_BREAK = 6,   /*[BK]*/
    U_LB_CONTINGENT_BREAK = 7,  /*[CB]*/
    U_LB_CLOSE_PUNCTUATION = 8, /*[CL]*/
    U_LB_COMBINING_MARK = 9,    /*[CM]*/
    U_LB_CARRIAGE_RETURN = 10,   /*[CR]*/
    U_LB_EXCLAMATION = 11,       /*[EX]*/
    U_LB_GLUE = 12,              /*[GL]*/
    U_LB_HYPHEN = 13,            /*[HY]*/
    U_LB_IDEOGRAPHIC = 14,       /*[ID]*/
    U_LB_INSEPERABLE = 15,
    /** Renamed from the misspelled "inseperable" in Unicode 4.0.1/ICU 3.0 @stable ICU 3.0 */
    U_LB_INSEPARABLE=U_LB_INSEPERABLE,/*[IN]*/
    U_LB_INFIX_NUMERIC = 16,     /*[IS]*/
    U_LB_LINE_FEED = 17,         /*[LF]*/
    U_LB_NONSTARTER = 18,        /*[NS]*/
    U_LB_NUMERIC = 19,           /*[NU]*/
    U_LB_OPEN_PUNCTUATION = 20,  /*[OP]*/
    U_LB_POSTFIX_NUMERIC = 21,   /*[PO]*/
    U_LB_PREFIX_NUMERIC = 22,    /*[PR]*/
    U_LB_QUOTATION = 23,         /*[QU]*/
    U_LB_COMPLEX_CONTEXT = 24,   /*[SA]*/
    U_LB_SURROGATE = 25,         /*[SG]*/
    U_LB_SPACE = 26,             /*[SP]*/
    U_LB_BREAK_SYMBOLS = 27,     /*[SY]*/
    U_LB_ZWSPACE = 28,           /*[ZW]*/
    U_LB_NEXT_LINE = 29,         /*[NL]*/ /* from here on: new in Unicode 4/ICU 2.6 */
    U_LB_WORD_JOINER = 30,       /*[WJ]*/
    U_LB_H2 = 31,                /*[H2]*/ /* from here on: new in Unicode 4.1/ICU 3.4 */
    U_LB_H3 = 32,                /*[H3]*/
    U_LB_JL = 33,                /*[JL]*/
    U_LB_JT = 34,                /*[JT]*/
    U_LB_JV = 35,                /*[JV]*/
    U_LB_COUNT = 36
} ULineBreak;

namespace CharProp
{
#include <ICUData/uchar_props_data.c>
}

#define GET_BIDI_PROPS(bdp, c, result) \
    UTRIE_GET16(&(bdp)->trie, c, result);

enum {
    UBIDI_IX_MIRROR_LENGTH = 3,

    UBIDI_MAX_VALUES_INDEX=15,
    UBIDI_IX_TOP=16,

    /* UBIDI_CLASS_SHIFT=0, */     /* bidi class: 5 bits (4..0) */
    UBIDI_JT_SHIFT=5,           /* joining type: 3 bits (7..5) */

    /* UBIDI__SHIFT=8, reserved: 2 bits (9..8) */

    UBIDI_JOIN_CONTROL_SHIFT=10,
    UBIDI_BIDI_CONTROL_SHIFT=11,

    UBIDI_IS_MIRRORED_SHIFT=12,         /* 'is mirrored' */
    UBIDI_MIRROR_DELTA_SHIFT=13,        /* bidi mirroring delta: 3 bits (15..13) */

    UBIDI_MAX_JG_SHIFT=16,              /* max JG value in indexes[UBIDI_MAX_VALUES_INDEX] bits 23..16 */

    UBIDI_ESC_MIRROR_DELTA=-4,
    UBIDI_MIN_MIRROR_DELTA=-3,
    UBIDI_MAX_MIRROR_DELTA=3,

    /* the source Unicode code point takes 21 bits (20..0) */
    UBIDI_MIRROR_INDEX_SHIFT=21,
    UBIDI_MAX_MIRROR_INDEX=0x7ff
};

#define UBIDI_GET_MIRROR_CODE_POINT(m) (UChar32)((m)&0x1fffff)
#define UBIDI_GET_MIRROR_INDEX(m) ((m)>>UBIDI_MIRROR_INDEX_SHIFT)

#define UBIDI_CLASS_MASK        0x0000001f
#define UBIDI_JT_MASK           0x000000e0
#define UBIDI_MAX_JG_MASK       0x00ff0000
#define UBIDI_GET_CLASS(props) ((props)&UBIDI_CLASS_MASK)
#define UBIDI_GET_FLAG(props, shift) (((props)>>(shift))&1)
#define UPROPS_DT_MASK          0x0000001f

namespace BidiProp
{
struct UBiDiProps {
    void *mem;
    const int32_t *indexes;
    const uint32_t *mirrors;
    const uint8_t *jgArray;

    UTrie trie;
    uint8_t formatVersion[4];
};

#include <ICUData/ubidi_props_data.c>
}

#define _NORM_MIN_SPECIAL       0xfc000000
#define _NORM_SURROGATES_TOP    0xfff00000
#define _NORM_AUX_FNC_MASK          (uint32_t)(_NORM_AUX_MAX_FNC-1)
#define _NORM_AUX_MAX_FNC           ((int32_t)1<<_NORM_AUX_COMP_EX_SHIFT)

enum {
    _NORM_AUX_COMP_EX_SHIFT=10,
    _NORM_AUX_UNSAFE_SHIFT=11,
    _NORM_AUX_NFC_SKIPPABLE_F_SHIFT=12,

    _NORM_INDEX_TOP=32,                  /* changing this requires a new formatVersion */

    _NORM_CC_SHIFT=8,           /* UnicodeData.txt combining class in bits 15..8 */

    _NORM_EXTRA_SHIFT=16,               /* 16 bits for the index to UChars and other extra data */
};

namespace NormProp
{
    static int32_t getFoldingNormOffset(uint32_t norm32) {
        if (_NORM_MIN_SPECIAL<=norm32 && norm32<_NORM_SURROGATES_TOP) {
            return
                UTRIE_BMP_INDEX_LENGTH+
                    (((int32_t)norm32>>(_NORM_EXTRA_SHIFT-UTRIE_SURROGATE_BLOCK_BITS))&
                     (0x3ff<<UTRIE_SURROGATE_BLOCK_BITS));
        } else {
            return 0;
        }
    }

    static int32_t getFoldingAuxOffset(uint32_t data) {
        return (int32_t)(data&_NORM_AUX_FNC_MASK)<<UTRIE_SURROGATE_BLOCK_BITS;
    }

#include <ICUData/unorm_props_data.c>
}

uint32_t u_getUnicodeProperties(UChar32 c, int32_t column) {
    uint16_t vecIndex;

    if (column == -1) {
        uint32_t props;
        GET_PROPS(c, props);
        return props;
    } else if (column < 0 || column >= CharProp::propsVectorsColumns) {
        return 0;
    } else {
        UTRIE_GET16(&CharProp::propsVectorsTrie, c, vecIndex);
        return CharProp::propsVectors[vecIndex+column];
    }
}

#define TO_MASK(x) (1 << (x))

#define GET_CATEGORY(props) ((props)&0x1f)
#define CAT_MASK(props) U_MASK(GET_CATEGORY(props))

// From uchar.h:

/**
 * Data for enumerated Unicode general category types.
 * See http://www.unicode.org/Public/UNIDATA/UnicodeData.html .
 * @stable ICU 2.0
 */
typedef enum UCharCategory
{
    /** See note !!.  Comments of the form "Cn" are read by genpname. */

    /** Non-category for unassigned and non-character code points. @stable ICU 2.0 */
    U_UNASSIGNED              = 0,
    /** Cn "Other, Not Assigned (no characters in [UnicodeData.txt] have this property)" (same as U_UNASSIGNED!) @stable ICU 2.0 */
    U_GENERAL_OTHER_TYPES     = 0,
    /** Lu @stable ICU 2.0 */
    U_UPPERCASE_LETTER        = 1,
    /** Ll @stable ICU 2.0 */
    U_LOWERCASE_LETTER        = 2,
    /** Lt @stable ICU 2.0 */
    U_TITLECASE_LETTER        = 3,
    /** Lm @stable ICU 2.0 */
    U_MODIFIER_LETTER         = 4,
    /** Lo @stable ICU 2.0 */
    U_OTHER_LETTER            = 5,
    /** Mn @stable ICU 2.0 */
    U_NON_SPACING_MARK        = 6,
    /** Me @stable ICU 2.0 */
    U_ENCLOSING_MARK          = 7,
    /** Mc @stable ICU 2.0 */
    U_COMBINING_SPACING_MARK  = 8,
    /** Nd @stable ICU 2.0 */
    U_DECIMAL_DIGIT_NUMBER    = 9,
    /** Nl @stable ICU 2.0 */
    U_LETTER_NUMBER           = 10,
    /** No @stable ICU 2.0 */
    U_OTHER_NUMBER            = 11,
    /** Zs @stable ICU 2.0 */
    U_SPACE_SEPARATOR         = 12,
    /** Zl @stable ICU 2.0 */
    U_LINE_SEPARATOR          = 13,
    /** Zp @stable ICU 2.0 */
    U_PARAGRAPH_SEPARATOR     = 14,
    /** Cc @stable ICU 2.0 */
    U_CONTROL_CHAR            = 15,
    /** Cf @stable ICU 2.0 */
    U_FORMAT_CHAR             = 16,
    /** Co @stable ICU 2.0 */
    U_PRIVATE_USE_CHAR        = 17,
    /** Cs @stable ICU 2.0 */
    U_SURROGATE               = 18,
    /** Pd @stable ICU 2.0 */
    U_DASH_PUNCTUATION        = 19,
    /** Ps @stable ICU 2.0 */
    U_START_PUNCTUATION       = 20,
    /** Pe @stable ICU 2.0 */
    U_END_PUNCTUATION         = 21,
    /** Pc @stable ICU 2.0 */
    U_CONNECTOR_PUNCTUATION   = 22,
    /** Po @stable ICU 2.0 */
    U_OTHER_PUNCTUATION       = 23,
    /** Sm @stable ICU 2.0 */
    U_MATH_SYMBOL             = 24,
    /** Sc @stable ICU 2.0 */
    U_CURRENCY_SYMBOL         = 25,
    /** Sk @stable ICU 2.0 */
    U_MODIFIER_SYMBOL         = 26,
    /** So @stable ICU 2.0 */
    U_OTHER_SYMBOL            = 27,
    /** Pi @stable ICU 2.0 */
    U_INITIAL_PUNCTUATION     = 28,
    /** Pf @stable ICU 2.0 */
    U_FINAL_PUNCTUATION       = 29,
    /** One higher than the last enum UCharCategory constant. @stable ICU 2.0 */
    U_CHAR_CATEGORY_COUNT
} UCharCategory;

/**
 * U_GC_XX_MASK constants are bit flags corresponding to Unicode
 * general category values.
 * For each category, the nth bit is set if the numeric value of the
 * corresponding UCharCategory constant is n.
 *
 * There are also some U_GC_Y_MASK constants for groups of general categories
 * like L for all letter categories.
 *
 * @see u_charType
 * @see U_GET_GC_MASK
 * @see UCharCategory
 * @stable ICU 2.1
 */
#define U_GC_CN_MASK    U_MASK(U_GENERAL_OTHER_TYPES)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LU_MASK    U_MASK(U_UPPERCASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LL_MASK    U_MASK(U_LOWERCASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LT_MASK    U_MASK(U_TITLECASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LM_MASK    U_MASK(U_MODIFIER_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LO_MASK    U_MASK(U_OTHER_LETTER)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_MN_MASK    U_MASK(U_NON_SPACING_MARK)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ME_MASK    U_MASK(U_ENCLOSING_MARK)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_MC_MASK    U_MASK(U_COMBINING_SPACING_MARK)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ND_MASK    U_MASK(U_DECIMAL_DIGIT_NUMBER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_NL_MASK    U_MASK(U_LETTER_NUMBER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_NO_MASK    U_MASK(U_OTHER_NUMBER)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZS_MASK    U_MASK(U_SPACE_SEPARATOR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZL_MASK    U_MASK(U_LINE_SEPARATOR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZP_MASK    U_MASK(U_PARAGRAPH_SEPARATOR)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CC_MASK    U_MASK(U_CONTROL_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CF_MASK    U_MASK(U_FORMAT_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CO_MASK    U_MASK(U_PRIVATE_USE_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CS_MASK    U_MASK(U_SURROGATE)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PD_MASK    U_MASK(U_DASH_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PS_MASK    U_MASK(U_START_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PE_MASK    U_MASK(U_END_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PC_MASK    U_MASK(U_CONNECTOR_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PO_MASK    U_MASK(U_OTHER_PUNCTUATION)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SM_MASK    U_MASK(U_MATH_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SC_MASK    U_MASK(U_CURRENCY_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SK_MASK    U_MASK(U_MODIFIER_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SO_MASK    U_MASK(U_OTHER_SYMBOL)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PI_MASK    U_MASK(U_INITIAL_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PF_MASK    U_MASK(U_FINAL_PUNCTUATION)


/** Mask constant for multiple UCharCategory bits (L Letters). @stable ICU 2.1 */
#define U_GC_L_MASK \
            (U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK|U_GC_LM_MASK|U_GC_LO_MASK)

/** Mask constant for multiple UCharCategory bits (LC Cased Letters). @stable ICU 2.1 */
#define U_GC_LC_MASK \
            (U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK)

/** Mask constant for multiple UCharCategory bits (M Marks). @stable ICU 2.1 */
#define U_GC_M_MASK (U_GC_MN_MASK|U_GC_ME_MASK|U_GC_MC_MASK)

/** Mask constant for multiple UCharCategory bits (N Numbers). @stable ICU 2.1 */
#define U_GC_N_MASK (U_GC_ND_MASK|U_GC_NL_MASK|U_GC_NO_MASK)

/** Mask constant for multiple UCharCategory bits (Z Separators). @stable ICU 2.1 */
#define U_GC_Z_MASK (U_GC_ZS_MASK|U_GC_ZL_MASK|U_GC_ZP_MASK)

/** Mask constant for multiple UCharCategory bits (C Others). @stable ICU 2.1 */
#define U_GC_C_MASK \
            (U_GC_CN_MASK|U_GC_CC_MASK|U_GC_CF_MASK|U_GC_CO_MASK|U_GC_CS_MASK)

/** Mask constant for multiple UCharCategory bits (P Punctuation). @stable ICU 2.1 */
#define U_GC_P_MASK \
            (U_GC_PD_MASK|U_GC_PS_MASK|U_GC_PE_MASK|U_GC_PC_MASK|U_GC_PO_MASK| \
             U_GC_PI_MASK|U_GC_PF_MASK)

/** Mask constant for multiple UCharCategory bits (S Symbols). @stable ICU 2.1 */
#define U_GC_S_MASK (U_GC_SM_MASK|U_GC_SC_MASK|U_GC_SK_MASK|U_GC_SO_MASK)

//#define GET_NUMERIC_TYPE(props) (((props)>>UPROPS_NUMERIC_TYPE_SHIFT)&7)
//#define GET_NUMERIC_VALUE(props) (((props)>>UPROPS_NUMERIC_VALUE_SHIFT)&0xff)
