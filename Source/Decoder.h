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

#include <stdint.h>

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

struct ImageInfo {
    EImageFormat type;
    ERGBFormat rgb_format;
    int bit_depth;
    int width;
    int height;
};

struct ImagePixelData {
    ETextureSourceFormat texture_format;
    int bit_depth;
    uint8_t* data;
    int width;
    int height;
    int size;  // should equals to width * height * components * bit_depth / 8
};

enum class ELogLevel { Info, Warning, Error };

typedef void(__cdecl* LogFunc)(ELogLevel, const char*);

IMAGE_PORT void __cdecl SetLogFunction(LogFunc func);

IMAGE_PORT bool __cdecl CreatePixelDataFromFile(EImageFormat image_format, const char* file_name, ImageInfo& info, ImagePixelData*& pixel_data);

IMAGE_PORT bool __cdecl CreatePixelData(EImageFormat image_format, const uint8_t* buffer, uint64_t length, ImageInfo& info, ImagePixelData*& pixel_data);

IMAGE_PORT void __cdecl ReleasePixelData(ImagePixelData*& pixel_data);

IMAGE_PORT EImageFormat __cdecl DetectFormat(const void* compressed_data, int64_t compressed_size);
}
}  // namespace ImageDecoder
