#include "BmpImageWrapper.h"
#include <cmath>
#include "Utils/Utils.h"
#include "Wrapper/BmpImageSupport.h"

namespace ImageDecoder {
/**
 * BMP image wrapper class.
 * This code was adapted from UTextureFactory::ImportTexture, but has not been throughly tested.
 */

FBmpImageWrapper::FBmpImageWrapper(bool bInHasHeader, bool bInHalfHeight) : FImageWrapperBase(), bHasHeader(bInHasHeader), bHalfHeight(bInHalfHeight) {}

void FBmpImageWrapper::Compress(int quality) { std::cerr << "BMP compression not supported" << std::endl; }

void FBmpImageWrapper::Uncompress(const ERGBFormat inFormat, const int inBitDepth) {
    const uint8_t* buffer = compressedData.data();

    if (!bHasHeader || ((compressedData.size() >= sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader)) && buffer[0] == 'B' && buffer[1] == 'M')) {
        UncompressBMPData(inFormat, inBitDepth);
    }
}

void FBmpImageWrapper::UncompressBMPData(const ERGBFormat inFormat, const int inBitDepth) {
    const uint8_t* Buffer = compressedData.data();
    const FBitmapInfoHeader* bmhdr = nullptr;
    const uint8_t* bits = nullptr;
    EBitmapHeaderVersion headerVersion = EBitmapHeaderVersion::BHV_BITMAPINFOHEADER;

    if (bHasHeader) {
        bmhdr = (FBitmapInfoHeader*)(Buffer + sizeof(FBitmapFileHeader));
        bits = Buffer + ((FBitmapFileHeader*)Buffer)->bfOffBits;
        headerVersion = ((FBitmapFileHeader*)Buffer)->GetHeaderVersion();
    } else {
        bmhdr = (FBitmapInfoHeader*)Buffer;
        bits = Buffer + sizeof(FBitmapInfoHeader);
    }

    if (bmhdr->biCompression != BCBI_RGB && bmhdr->biCompression != BCBI_BITFIELDS) {
        std::cerr << "RLE compression of BMP images not supported" << std::endl;
        return;
    }

    struct FColor {
        uint8_t r, g, b, a = 0;
    };

    if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 8) {
        // Do palette.
        const uint8_t* bmpal = (uint8_t*)compressedData.data() + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader);

        // Set texture properties.
        width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        format = ERGBFormat::BGRA;
        rawData.resize(height * width * 4);

        FColor* imageData = (FColor*)rawData.data();

        // If the number for color palette entries is 0, we need to default to 2^biBitCount entries.  In this case 2^8 = 256
        int clrPaletteCount = bmhdr->biClrUsed ? bmhdr->biClrUsed : 256;
        std::vector<FColor> palette;

        for (int i = 0; i < clrPaletteCount; i++) {
            FColor color;
            color.r = bmpal[i * 4 + 2];
            color.g = bmpal[i * 4 + 1];
            color.b = bmpal[i * 4 + 0];
            color.a = 255;
            palette.push_back(std::move(color));
        }

        while (palette.size() < 256) {
            FColor color;
            color.r = 0;
            color.g = 0;
            color.b = 0;
            color.a = 255;
            palette.push_back(std::move(color));
        }

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int srcStride = Align(width, 4);
        const int srcPtrDiff = bNegativeHeight ? srcStride : -srcStride;
        const uint8_t* srcPtr = bits + (bNegativeHeight ? 0 : height - 1) * srcStride;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                *imageData++ = palette[srcPtr[x]];
            }

            srcPtr += srcPtrDiff;
        }
    } else if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 24) {
        // Set texture properties.
        width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        format = ERGBFormat::BGRA;
        rawData.resize(height * width * 4);

        uint8_t* imageData = rawData.data();

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int srcStride = Align(width * 3, 4);
        const int srcPtrDiff = bNegativeHeight ? srcStride : -srcStride;
        const uint8_t* srcPtr = bits + (bNegativeHeight ? 0 : height - 1) * srcStride;

        for (int y = 0; y < height; y++) {
            const uint8_t* srcRowPtr = srcPtr;
            for (int x = 0; x < width; x++) {
                *imageData++ = *srcRowPtr++;
                *imageData++ = *srcRowPtr++;
                *imageData++ = *srcRowPtr++;
                *imageData++ = 0xFF;
            }

            srcPtr += srcPtrDiff;
        }
    } else if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 32) {
        // Set texture properties.
        width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        format = ERGBFormat::BGRA;
        rawData.resize(height * width * 4);

        uint8_t* imageData = rawData.data();

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int srcStride = width * 4;
        const int srcPtrDiff = bNegativeHeight ? srcStride : -srcStride;
        const uint8_t* srcPtr = bits + (bNegativeHeight ? 0 : height - 1) * srcStride;

        // Getting the bmiColors member from the BITMAPINFO, which is used as a mask on BitFields compression.
        const FBmiColorsMask* colorMask = (FBmiColorsMask*)(compressedData.data() + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader));
        // Header version 4 introduced the option to declare custom color space, so we can't just assume sRGB past that version.
        const bool bAssumeRGBCompression = bmhdr->biCompression == BCBI_RGB || (bmhdr->biCompression == BCBI_BITFIELDS && colorMask->IsMaskRGB8() && headerVersion < EBitmapHeaderVersion::BHV_BITMAPV4HEADER);

        if (bAssumeRGBCompression) {
            for (int y = 0; y < height; y++) {
                const uint8_t* srcRowPtr = srcPtr;
                for (int x = 0; x < width; x++) {
                    *imageData++ = *srcRowPtr++;
                    *imageData++ = *srcRowPtr++;
                    *imageData++ = *srcRowPtr++;
                    *imageData++ = 0xFF;  // In BCBI_RGB compression the last 8 bits of the pixel are not used.
                    srcRowPtr++;
                }

                srcPtr += srcPtrDiff;
            }
        } else if (bmhdr->biCompression == BCBI_BITFIELDS) {
            // If the header version is V4 or higher we need to make sure we are still using sRGB format
            if (headerVersion >= EBitmapHeaderVersion::BHV_BITMAPV4HEADER) {
                const FBitmapInfoHeaderV4* bmhdrV4 = (FBitmapInfoHeaderV4*)(Buffer + sizeof(FBitmapFileHeader));

                if (bmhdrV4->biCSType != (uint32_t)EBitmapCSType::BCST_LCS_sRGB && bmhdrV4->biCSType != (uint32_t)EBitmapCSType::BCST_LCS_WINDOWS_COLOR_SPACE) {
                    std::cerr << "BMP uses an unsupported custom color space definition, sRGB color space will be used instead." << std::endl;
                }
            }

            // Calculating the bit mask info needed to remap the pixels' color values.
            uint32_t trailingBits[4];
            float mappingRatio[4];
            for (uint32_t MaskIndex = 0; MaskIndex < 4; MaskIndex++) {
                trailingBits[MaskIndex] = CountTrailingZeros(colorMask->RGBAMask[MaskIndex]);
                const uint32_t NumberOfBits = 32 - (trailingBits[MaskIndex] + CountLeadingZeros(colorMask->RGBAMask[MaskIndex]));
                mappingRatio[MaskIndex] = static_cast<float>(NumberOfBits == 0 ? 0 : (exp2(8) - 1) / (exp2(NumberOfBits) - 1));
            }

            // In header pre-version 4, we should ignore the last 32bit (alpha) content.
            const bool bHasAlphaChannel = colorMask->RGBAMask[3] != 0 && headerVersion >= EBitmapHeaderVersion::BHV_BITMAPV4HEADER;

            for (int y = 0; y < height; y++) {
                const uint32_t* srcPixel = (uint32_t*)srcPtr;
                for (int x = 0; x < width; x++) {
                    // Set the color values in BGRA order.
                    for (int ColorIndex = 2; ColorIndex >= 0; ColorIndex--) {
                        *imageData++ = (int)(round(((*srcPixel & colorMask->RGBAMask[ColorIndex]) >> trailingBits[ColorIndex]) * mappingRatio[ColorIndex]));
                    }

                    *imageData++ = bHasAlphaChannel ? (int)(round(((*srcPixel & colorMask->RGBAMask[3]) >> trailingBits[3]) * mappingRatio[3])) : 0xFF;

                    srcPixel++;
                }

                srcPtr += srcPtrDiff;
            }
        } else {
            std::cerr << "BMP uses an unsupported compression format " << bmhdr->biCompression << std::endl;
        }
    } else if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 16) {
        std::cerr << "BMP 16 bit format no longer supported. Use terrain tools for importing/exporting heightmaps." << std::endl;
    } else {
        std::cerr << "BMP uses an unsupported format (" << bmhdr->biPlanes << "/" << bmhdr->biBitCount << ")" << std::endl;
    }
}

bool FBmpImageWrapper::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(inCompressedData, inCompressedSize);

    return bResult && (bHasHeader ? LoadBMPHeader() : LoadBMPInfoHeader());  // Fetch the variables from the header info
}

bool FBmpImageWrapper::LoadBMPHeader() {
    const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader*)(compressedData.data() + sizeof(FBitmapFileHeader));
    const FBitmapFileHeader* bmf = (FBitmapFileHeader*)(compressedData.data() + 0);
    if ((compressedData.size() >= sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader)) && compressedData.data()[0] == 'B' && compressedData.data()[1] == 'M') {
        if (bmhdr->biCompression != BCBI_RGB && bmhdr->biCompression != BCBI_BITFIELDS) {
            std::cerr << "RLE compression of BMP images not supported" << std::endl;
            return false;
        }

        if (bmhdr->biPlanes == 1 && (bmhdr->biBitCount == 8 || bmhdr->biBitCount == 24 || bmhdr->biBitCount == 32)) {
            // Set texture properties.
            width = bmhdr->biWidth;
            height = abs(bmhdr->biHeight);
            format = ERGBFormat::BGRA;
            bitDepth = bmhdr->biBitCount;

            return true;
        }

        if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 16) {
            std::cerr << "BMP 16 bit format no longer supported. Use terrain tools for importing/exporting heightmaps." << std::endl;
        } else {
            std::cerr << "BMP uses an unsupported format (" << bmhdr->biPlanes << "/" << bmhdr->biBitCount << ")" << std::endl;
        }
    }

    return false;
}

bool FBmpImageWrapper::LoadBMPInfoHeader() {
    const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader*)compressedData.data();

    if (bmhdr->biCompression != BCBI_RGB && bmhdr->biCompression != BCBI_BITFIELDS) {
        std::cerr << "RLE compression of BMP images not supported" << std::endl;
        return false;
    }

    if (bmhdr->biPlanes == 1 && (bmhdr->biBitCount == 8 || bmhdr->biBitCount == 24 || bmhdr->biBitCount == 32)) {
        // Set texture properties.
        width = bmhdr->biWidth;
        height = abs(bmhdr->biHeight);
        format = ERGBFormat::BGRA;
        bitDepth = bmhdr->biBitCount;

        return true;
    }

    if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 16) {
        std::cerr << "BMP 16 bit format no longer supported. Use terrain tools for importing/exporting heightmaps." << std::endl;
    } else {
        std::cerr << "BMP uses an unsupported format (" << bmhdr->biPlanes << "/" << bmhdr->biBitCount << ")" << std::endl;
    }

    return false;
}
}  // namespace ImageDecoder
