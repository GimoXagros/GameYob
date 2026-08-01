#pragma once

struct DecodedImage {
    int width;
    int height;
    unsigned char* pixels;
};

bool decodeImageFile(const char* filename, DecodedImage* image);
void freeDecodedImage(DecodedImage* image);
