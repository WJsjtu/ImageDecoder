#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include "Export.h"
#include "IImageWrapper.h"

enum class IMAGE_PORT ETextureSourceFormat {
    Invalid,
    G8,
    G16,
    BGRA8,
    BGRE8,
    RGBA16,
    RGBA16F,
    RGBA8,
    RGBE8,
};

class IMAGE_PORT ImportImage {
public:
    struct Source {
        EImageFormat type = EImageFormat::Invalid;
        ERGBFormat RGBFormat = ERGBFormat::Invalid;
        int bitDepth = 8;
    } source;
    std::vector<uint8_t> data;
    ETextureSourceFormat textureformat = ETextureSourceFormat::Invalid;
    int bitDepth = 8;
    int numMips = 0;
    int width = 0;
    int height = 0;
};

bool IMAGE_PORT DecodeImage(EImageFormat imageFormat, const uint8_t* buffer, uint32_t length, bool bFillPNGZeroAlpha, std::string& warn, ImportImage& outImage);
