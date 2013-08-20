/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *           (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 *           (C) 2010 Igalia S.L
 *           (C) 2013 Hewlett-Packard Development Co. All rights reserved.
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
#include "PixelDumpSupportCairo.h"

#include "DumpRenderTree.h"
#include "MD5.h"
#include "PixelDumpSupport.h"
#include <algorithm>
#include <ctype.h>
#include <wtf/Assertions.h>
#include <wtf/RefPtr.h>
#include <wtf/StringExtras.h>

#include <png.h>

using namespace std;

static void writeFunction(png_struct * png, png_byte * data, png_size_t length)
{
    Vector<unsigned char>* in = reinterpret_cast<Vector<unsigned char>*>(png_get_io_ptr (png));
    in->append(data, length);
}

/* Unpremultiplies data and converts native endian ARGB => RGBA bytes */
static void
unpremultiply_data (png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *b = &data[i];
        uint32_t pixel;
        uint8_t  alpha;

	memcpy (&pixel, b, sizeof (uint32_t));
	alpha = (pixel & 0xff000000) >> 24;
        if (alpha == 0) {
	    b[0] = b[1] = b[2] = b[3] = 0;
	} else {
            b[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
            b[1] = (((pixel & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
            b[2] = (((pixel & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
	    b[3] = alpha;
	}
    }
}

/* Use a couple of simple error callbacks that do not print anything to
 * stderr and rely on the user to check for errors via the #cairo_status_t
 * return.
 */
static void
png_simple_error_callback (png_structp png,
	                   png_const_charp error_msg)
{
    bool *success = static_cast<bool*>(png_get_error_ptr (png));
    *success = false;
#ifdef PNG_SETJMP_SUPPORTED
    longjmp (png_jmpbuf (png), 1);
#endif
    /* if we get here, then we have to choice but to abort ... */
}

static void
png_simple_warning_callback (png_structp png,
	                     png_const_charp error_msg)
{
    bool *success = static_cast<bool*>(png_get_error_ptr (png));
    *success = false;
    /* png does not expect to abort and will try to tidy up after a warning */
}

/* Starting with libpng-1.2.30, we must explicitly specify an output_flush_fn.
 * Otherwise, we will segfault if we are writing to a stream. */
static void
png_simple_output_flush_fn (png_structp png_ptr)
{
}


static bool writePNG(cairo_surface_t* surface, Vector<unsigned char>* pixelData)
{
    png_struct *png;
    png_info *info;
    png_color_16 white;

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    unsigned char * data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    /* PNG complains about "Image width or height is zero in IHDR" */
    if (width == 0 || height == 0)
	    return false;

    png_byte ** rows = static_cast<png_byte**>(malloc(height * sizeof (png_byte*)));

    for (int i = 0; i < height; i++)
        rows[i] = (png_byte *) data + i * stride;

    bool success = true;
    png = png_create_write_struct (PNG_LIBPNG_VER_STRING, &success,
	                           png_simple_error_callback,
	                           png_simple_warning_callback);

    info = png_create_info_struct (png);

#ifdef PNG_SETJMP_SUPPORTED
    if (setjmp (png_jmpbuf (png)))
	goto BAIL;
#endif

    png_set_write_fn (png, pixelData, writeFunction, png_simple_output_flush_fn);

    int depth = 8;

    png_set_IHDR (png, info,
		  width,
		  height, depth,
		  PNG_COLOR_TYPE_RGB_ALPHA,
		  PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT,
		  PNG_FILTER_TYPE_DEFAULT);

    white.gray = (1 << depth) - 1;
    white.red = white.blue = white.green = white.gray;
    png_set_bKGD (png, info, &white);

    /* We have to call png_write_info() before setting up the write
     * transformation, since it stores data internally in 'png'
     * that is needed for the write transformation functions to work.
     */
    png_write_info (png, info);

	png_set_write_user_transform_fn (png, unpremultiply_data);

    png_write_image (png, rows);
    png_write_end (png, info);

BAIL:
    png_destroy_write_struct (&png, &info);
    free (rows);

    return success;
}

static void printPNG(cairo_surface_t* image, const char* checksum)
{
    Vector<unsigned char> pixelData;
    // Only PNG output is supported for now.
    // It would be nice to use cairo_surface_write_to_png_stream,
    // but this method outputs a 24bpp image if all alpha values are 255.
    // Layout test images are 32bpp images for all cases.
    //cairo_surface_write_to_png_stream(image, writeFunction, &pixelData);

    writePNG(image, &pixelData);

    const size_t dataLength = pixelData.size();
    const unsigned char* data = pixelData.data();

    printPNG(data, dataLength, checksum);
}

void computeMD5HashStringForBitmapContext(BitmapContext* context, char hashString[33])
{
    cairo_t* bitmapContext = context->cairoContext();
    cairo_surface_t* surface = cairo_get_target(bitmapContext);

    ASSERT(cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32); // ImageDiff assumes 32 bit RGBA, we must as well.

    size_t pixelsHigh = cairo_image_surface_get_height(surface);
    size_t pixelsWide = cairo_image_surface_get_width(surface);
    size_t bytesPerRow = cairo_image_surface_get_stride(surface);

    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    unsigned char* bitmapData = static_cast<unsigned char*>(cairo_image_surface_get_data(surface));
    for (unsigned row = 0; row < pixelsHigh; row++) {
        MD5_Update(&md5Context, bitmapData, 4 * pixelsWide);
        bitmapData += bytesPerRow;
    }
    Vector<uint8_t, 16> hash(16);
    MD5_Final(hash.data(), &md5Context);

    snprintf(hashString, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
        hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
}

void dumpBitmap(BitmapContext* context, const char* checksum)
{
    cairo_surface_t* surface = cairo_get_target(context->cairoContext());
    printPNG(surface, checksum);
}
