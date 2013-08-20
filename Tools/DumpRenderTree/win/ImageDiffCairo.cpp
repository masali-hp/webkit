/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2005 Ben La Monica <ben.lamonica@gmail.com>.  All rights reserved.
 * Copyright (C) 2011 Brent Fulgham. All rights reserved.
 * Copyright (C) 2013 Hewlett-Packard Development Co. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// FIXME: We need to be able to include these defines from a config.h somewhere.
#define JS_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE

#include <cairo.h>
#include <stdio.h>
#include <wtf/Platform.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

#if PLATFORM(WIN)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <wtf/MathExtras.h>
#endif

#include <png.h>

using namespace std;

static const int s_bufferSize = 2048;
static const int s_bytesPerPixel = 4;
static cairo_user_data_key_t s_imageDataKey;


#if PLATFORM(WIN)
#undef min
#undef max

static inline float strtof(const char* inputString, char** endptr)
{
    return strtod(inputString, endptr);
}
#endif

struct Buffer {
    char * data;
    unsigned int capacity;
    unsigned int position;

    Buffer()
        : data(NULL)
        , capacity(0)
        , position(0) {
    }

    ~Buffer() {
        delete data;
    }

    void append(unsigned char* bytes, unsigned int length) {
        if (capacity < length + position) {
            size_t newCapacity = length + position;
            char * newBuffer = new char[newCapacity];
            memcpy(newBuffer, data, position);
            delete data;
            data = newBuffer;
            capacity = newCapacity;
        }
        memcpy(data + position, bytes, length);
        position += length;
    }

    void readFromBuffer(unsigned char* bytes, unsigned int length) {
        memcpy(bytes, data + position, length);
        position += length;
    }

    void reset() {
        capacity = position;
        position = 0;
    }
};

static cairo_status_t readFromData(void* closure, unsigned char* data, unsigned int length)
{
    reinterpret_cast<Buffer*>(closure)->readFromBuffer(data, length);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_surface_t* createImageFromStdin(int bytesRemaining)
{
    Buffer readBuffer;
    unsigned char buffer[s_bufferSize];

    while (bytesRemaining > 0) {
        size_t bytesToRead = min(bytesRemaining, s_bufferSize);
        size_t bytesRead = fread(buffer, 1, bytesToRead, stdin);
        readBuffer.append(buffer, bytesRead);
        bytesRemaining -= static_cast<int>(bytesRead);
    }

    readBuffer.reset();
    return cairo_image_surface_create_from_png_stream (static_cast<cairo_read_func_t>(readFromData), &readBuffer);
}

static void releaseMallocBuffer(void* data)
{
    free(data);
}

static inline float pixelDifference(float expected, float actual)
{
    return (actual - expected) / max<float>(255 - expected, expected);
}

static inline void normalizeBuffer(float maxDistance, unsigned char* buffer, size_t length)
{
    if (maxDistance >= 1)
        return;

    for (size_t p = 0; p < length; ++p)
        buffer[p] /= maxDistance;
}

static unsigned char* createDifferenceImage(cairo_surface_t* baselineImage, cairo_surface_t* actualImage, float& difference)
{
    size_t width = cairo_image_surface_get_width(baselineImage);
    size_t height = cairo_image_surface_get_height(baselineImage);

    unsigned char* baselinePixel = cairo_image_surface_get_data(baselineImage);
    unsigned char* actualPixel = cairo_image_surface_get_data(actualImage);

    // Compare the content of the 2 bitmaps
    unsigned char* diffBuffer = reinterpret_cast<unsigned char*>(malloc(width * height));
    unsigned char* diffPixel = diffBuffer;

    float count = 0;
    float sum = 0;
    float maxDistance = 0;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float red = pixelDifference(baselinePixel[0], actualPixel[0]);
            float green = pixelDifference(baselinePixel[1], actualPixel[1]);
            float blue = pixelDifference(baselinePixel[2], actualPixel[2]);
            float alpha = pixelDifference(baselinePixel[3], actualPixel[3]);

            float distance = sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0;
            
            *diffPixel++ = static_cast<unsigned char>(distance * 255);
            
            if (distance >= 1.0 / 255.0) {
                ++count;
                sum += distance;
                if (distance > maxDistance)
                    maxDistance = distance;
            }
            
            baselinePixel += s_bytesPerPixel;
            actualPixel += s_bytesPerPixel;
        }
    }
    
    // Compute the difference as a percentage combining both the number of different pixels and their difference amount i.e. the average distance over the entire image
    if (count > 0)
        difference = 100.0f * sum / (height * width);
    else
        difference = 0;

    if (!difference) {
        free(diffBuffer);
        return 0;
    }

    // Generate a normalized diff image
    normalizeBuffer(maxDistance, reinterpret_cast<unsigned char*>(diffBuffer), height * width);
    
    return diffBuffer;
}

static inline bool imageHasAlpha(cairo_surface_t* image)
{
    return (cairo_image_surface_get_format(image) == CAIRO_FORMAT_ARGB32);
}

static void writeToVector(png_struct * png, png_byte * data, png_size_t length)
{
    Vector<unsigned char>* in = reinterpret_cast<Vector<unsigned char>*>(png_get_io_ptr (png));
    in->append(data, length);
}

void convert_grayscale_bmp_to_png(unsigned char * bmp, int width, int height, Vector<unsigned char> & imageData) {
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    int bitsPixel = 8;
    png_set_IHDR(png, info, width, height,
             bitsPixel, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_set_write_fn (png, &imageData, writeToVector, NULL);
    png_write_info(png, info);
    png_bytepp row_pointers = new png_bytep[height];
    for (int y=0; y < height; y++){
        row_pointers[y] = bmp + y * width;
    }
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);
    delete [] row_pointers;
    png_destroy_write_struct(&png, &info);
}

int main(int argc, const char* argv[])
{
#if PLATFORM(WIN)
    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
#endif

    float tolerance = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--tolerance")) {
            if (i >= argc - 1)
                exit(1);
            tolerance = strtof(argv[i + 1], 0);
            ++i;
            continue;
        }
    }

    char buffer[s_bufferSize];
    cairo_surface_t* actualImage = 0;
    cairo_surface_t* baselineImage = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        char* newLineCharacter = strchr(buffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strncmp("Content-Length: ", buffer, 16)) {
            strtok(buffer, " ");
            int imageSize = strtol(strtok(0, " "), 0, 10);

            if (imageSize > 0 && !actualImage)
                actualImage = createImageFromStdin(imageSize);
            else if (imageSize > 0 && !baselineImage)
                baselineImage = createImageFromStdin(imageSize);
            else
                fputs("error, image size must be specified.\n", stdout);
        }

        if (actualImage && baselineImage) {
            unsigned char* diffImage = 0;
            float difference = 100.0;
            
            if ((cairo_image_surface_get_width(actualImage) == cairo_image_surface_get_width(baselineImage))
                && (cairo_image_surface_get_height(actualImage) == cairo_image_surface_get_height(baselineImage))
                && (imageHasAlpha(actualImage) == imageHasAlpha(baselineImage))) {
                diffImage = createDifferenceImage(actualImage, baselineImage, difference); // difference is passed by reference
                if (difference <= tolerance)
                    difference = 0;
                else {
                    difference = roundf(difference * 100.0) / 100.0;
                    difference = max<float>(difference, 0.01); // round to 2 decimal places
                }
            } else
                fputs("error, test and reference image have different properties.\n", stderr);
                
            if (difference > 0.0) {
                if (diffImage) {
                    Vector<unsigned char> imageData;
                    convert_grayscale_bmp_to_png(diffImage, cairo_image_surface_get_width(actualImage), cairo_image_surface_get_height(actualImage), imageData);
                    printf("Content-Length: %lu\n", imageData.size());
                    fwrite(imageData.data(), 1, imageData.size(), stdout);
                }
                
                fprintf(stdout, "diff: %01.2f%% failed\n", difference);
            } else
                fprintf(stdout, "diff: %01.2f%% passed\n", difference);
            
            cairo_surface_destroy(actualImage);
            cairo_surface_destroy(baselineImage);
            actualImage = 0;
            baselineImage = 0;
            if (diffImage)
                free(diffImage);
        }

        fflush(stdout);
    }

    return 0;
}
