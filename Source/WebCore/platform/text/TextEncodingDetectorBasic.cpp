/*
 * (C) Copyright 2010 Hewlett-Packard Development Company, L.P.
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


#include "config.h"

#include "TextEncodingDetector.h"
#include "TextEncoding.h"

#include <wtf/unicode/UTF8.h>

using namespace WTF;

namespace WebCore {

bool detectTextEncoding(const char* data, size_t len, const char* hintEncodingName, TextEncoding* detectedEncoding)
{
    if (!data || len == 0)
        return false;

    const char * detectedEncodingName = NULL;
    // This algorithm is about as simple as they get.  
    // It is not anywhere close to a universal detector, but it should do a good job of 
    // coming up with ASCII, UTF-8, UTF16-LE or UTF16-BE if the data is in fact
    // in one of those formats.
    int null_count = 0;
    int range_1_31 = 0;
    int range_32_127 = 0;
    int range_128_255 = 0;
    int odd_null_bytes = 0;

    for (int i = 0; i < len; i++)
    {
        if (data[i] == 0)
        {
            null_count++;
            if (i % 2)
                odd_null_bytes++;
        }
        else if (data[i] < 32 && data[i] != '\r' && data[i] != '\n' && data[i] != '\t')
            range_1_31++;
        else if (data[i] < 128)
            range_32_127++;
        else
            range_128_255++;
    }

    if (range_32_127 * 100 / len >= 99) // 99% ASCII characters
    {
        if (range_32_127 == len)
            detectedEncodingName = "ASCII";
        else
            detectedEncodingName = "UTF-8";
    }
    else if (null_count * 100 / len >= 2) // 3% (or more) are null bytes
    {
        if (odd_null_bytes * 100 / null_count > 90) 
            detectedEncodingName = "UTF16-LE";
        else if (odd_null_bytes * 100 / null_count < 10) 
            detectedEncodingName = "UTF16-BE";
    }
    else
    {
        UChar * utf16_bytes = new UChar[len];
        UChar * original_bytes = utf16_bytes;
		if (Unicode::convertUTF8ToUTF16(&data, data + len, &utf16_bytes, utf16_bytes + len, NULL, true) == Unicode::conversionOK)
            detectedEncodingName = "UTF-8";
        delete[] original_bytes;
    }

    if (detectedEncodingName)
        hintEncodingName = detectedEncodingName;

    if (hintEncodingName)
    {
        *detectedEncoding = TextEncoding(hintEncodingName);
        return true;
    }

    return false;
}

}
