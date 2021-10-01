#include <memory>
#include "Formats/BmpImageWrapper.h"
#include "Formats/ExrImageWrapper.h"
#include "Formats/IcoImageWrapper.h"
#include "Formats/JpegImageWrapper.h"
#include "Formats/PngImageWrapper.h"
#include "IImageWrapperModule.h"

namespace {
static const uint8_t IMAGE_MAGIC_PNG[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const uint8_t IMAGE_MAGIC_JPEG[] = {0xFF, 0xD8, 0xFF};
static const uint8_t IMAGE_MAGIC_BMP[] = {0x42, 0x4D};
static const uint8_t IMAGE_MAGIC_ICO[] = {0x00, 0x00, 0x01, 0x00};
static const uint8_t IMAGE_MAGIC_EXR[] = {0x76, 0x2F, 0x31, 0x01};
static const uint8_t IMAGE_MAGIC_ICNS[] = {0x69, 0x63, 0x6E, 0x73};

/** Internal helper function to verify image signature. */
template <int MagicCount>
bool StartsWith(const uint8_t* Content, int64_t ContentSize, const uint8_t (&Magic)[MagicCount]) {
    if (ContentSize < MagicCount) {
        return false;
    }

    for (int I = 0; I < MagicCount; ++I) {
        if (Content[I] != Magic[I]) {
            return false;
        }
    }

    return true;
}
}  // namespace

/**
 * Image Wrapper module.
 */
class ImageWrapperModule : public IImageWrapperModule {
public:
    //~ IImageWrapperModule interface

    virtual std::shared_ptr<IImageWrapper> CreateImageWrapper(const EImageFormat InFormat) override {
        std::shared_ptr<FImageWrapperBase> ImageWrapper = NULL;

        // Allocate a helper for the format type
        switch (InFormat) {
            case EImageFormat::PNG: ImageWrapper = std::make_shared<FPngImageWrapper>(); break;
            case EImageFormat::JPEG: ImageWrapper = std::make_shared<FJpegImageWrapper>(); break;
            case EImageFormat::GrayscaleJPEG: ImageWrapper = std::make_shared<FJpegImageWrapper>(1); break;
            case EImageFormat::BMP: ImageWrapper = std::make_shared<FBmpImageWrapper>(); break;
            case EImageFormat::ICO: ImageWrapper = std::make_shared<FIcoImageWrapper>(); break;
            case EImageFormat::EXR: ImageWrapper = std::make_shared<FExrImageWrapper>(); break;

            default: break;
        }
        return ImageWrapper;
    }

    virtual EImageFormat DetectImageFormat(const void* CompressedData, int64_t CompressedSize) override {
        EImageFormat Format = EImageFormat::Invalid;
        if (StartsWith((uint8_t*)CompressedData, CompressedSize, IMAGE_MAGIC_PNG)) {
            Format = EImageFormat::PNG;
        } else if (StartsWith((uint8_t*)CompressedData, CompressedSize, IMAGE_MAGIC_JPEG)) {
            Format = EImageFormat::JPEG;  // @Todo: Should we detect grayscale vs non-grayscale?
        } else if (StartsWith((uint8_t*)CompressedData, CompressedSize, IMAGE_MAGIC_BMP)) {
            Format = EImageFormat::BMP;
        } else if (StartsWith((uint8_t*)CompressedData, CompressedSize, IMAGE_MAGIC_ICO)) {
            Format = EImageFormat::ICO;
        } else if (StartsWith((uint8_t*)CompressedData, CompressedSize, IMAGE_MAGIC_EXR)) {
            Format = EImageFormat::EXR;
        }

        return Format;
    }
};

ImageWrapperModule SharedImageWrapperModule;

IImageWrapperModule& GetImageWrapperModule() { return SharedImageWrapperModule; }
