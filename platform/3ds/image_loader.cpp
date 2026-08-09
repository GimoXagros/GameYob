#include <stdlib.h>
#include <vector>

#include "io.h"
#include "image_loader.h"

#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool decodeImageFile(const char* filename, DecodedImage* image) {
    if (!filename || !image)
        return false;
    image->width = 0;
    image->height = 0;
    image->pixels = NULL;

    FileHandle* file = file_open(filename, "rb");
    if (!file)
        return false;
    const int size = file_getSize(file);
    if (size <= 0 || size > 16 * 1024 * 1024) {
        file_close(file);
        return false;
    }
    std::vector<unsigned char> data(size);
    file_read(&data[0], 1, size, file);
    file_close(file);

    int components = 0;
    unsigned char* pixels = stbi_load_from_memory(&data[0], size,
        &image->width, &image->height, &components, 3);
    if (!pixels || image->width <= 0 || image->height <= 0 ||
            image->width > 4096 || image->height > 4096) {
        if (pixels)
            stbi_image_free(pixels);
        image->width = image->height = 0;
        return false;
    }
    image->pixels = pixels;
    return true;
}

void freeDecodedImage(DecodedImage* image) {
    if (!image)
        return;
    if (image->pixels)
        stbi_image_free(image->pixels);
    image->pixels = NULL;
    image->width = image->height = 0;
}
