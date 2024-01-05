#pragma once

#ifdef _WIN32
#ifdef IMAGE_DLL
#define IMAGE_PORT __declspec(dllexport)
#else
#define IMAGE_PORT __declspec(dllimport)
#endif
#else
#if __has_attribute(visibility)
#define IMAGE_PORT __attribute__((visibility("default")))
#else
#define IMAGE_PORT
#endif
#endif

#include <iostream>
#include <vector>

namespace ImageDecoder {

extern "C" {

/**
 * Enumerates the types of image formats this class can handle.
 */
enum class EImageFormat : int8_t {
    /** Invalid or unrecognized format. */
    Invalid = -1,

    /** Portable Network Graphics. */
    PNG = 0,

    /** Joint Photographic Experts Group. */
    JPEG,

    /** Windows Bitmap. */
    BMP,

    /** Windows Icon resource. */
    ICO,

    /** OpenEXR (HDR) image file format. */
    EXR,
    PCX,
    TGA
};

/**
 * Enumerates the types of RGB formats this class can handle.
 */
enum class ERGBFormat : int8_t {
    Invalid = -1,
    RGBA = 0,
    BGRA = 1,
    Gray = 2,
};

/**
 * Enumerates available image compression qualities.
 */
enum class EImageCompressionQuality : int8_t {
    Default = 0,
    Uncompressed = 1,
};

enum class ETextureSourceFormat {
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

struct ImageSurce {
    EImageFormat type = EImageFormat::Invalid;
    ERGBFormat rgb_format = ERGBFormat::Invalid;
    int bit_depth = 8;
};

struct ImportImage {
    ImageSurce source;
    uint8_t* decoded = 0;
    int64_t decoded_size = 0;
    ETextureSourceFormat texture_format = ETextureSourceFormat::Invalid;
    int bit_depth = 8;
    int num_mips = 0;
    int width = 0;
    int height = 0;
};

bool IMAGE_PORT __cdecl Decode(EImageFormat image_format, const uint8_t* buffer, uint32_t length, ImportImage* image);

EImageFormat IMAGE_PORT __cdecl DetectFormat(const void* compressed_data, int64_t compressed_size);
}
}  // namespace ImageDecoder
