#include "BmpImageWrapper.h"
#include <cmath>
#include "Utils.h"
#include "Wrapper/BmpImageSupport.h"

/**
 * BMP image wrapper class.
 * This code was adapted from UTextureFactory::ImportTexture, but has not been throughly tested.
 */

FBmpImageWrapper::FBmpImageWrapper(bool bInHasHeader, bool bInHalfHeight) : FImageWrapperBase(), bHasHeader(bInHasHeader), bHalfHeight(bInHalfHeight) {}

void FBmpImageWrapper::Compress(int Quality) { std::cerr << "BMP compression not supported" << std::endl; }

void FBmpImageWrapper::Uncompress(const ERGBFormat InFormat, const int InBitDepth) {
    const uint8_t* Buffer = CompressedData.data();

    if (!bHasHeader || ((CompressedData.size() >= sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader)) && Buffer[0] == 'B' && Buffer[1] == 'M')) {
        UncompressBMPData(InFormat, InBitDepth);
    }
}

void FBmpImageWrapper::UncompressBMPData(const ERGBFormat InFormat, const int InBitDepth) {
    const uint8_t* Buffer = CompressedData.data();
    const FBitmapInfoHeader* bmhdr = nullptr;
    const uint8_t* Bits = nullptr;
    EBitmapHeaderVersion HeaderVersion = EBitmapHeaderVersion::BHV_BITMAPINFOHEADER;

    if (bHasHeader) {
        bmhdr = (FBitmapInfoHeader*)(Buffer + sizeof(FBitmapFileHeader));
        Bits = Buffer + ((FBitmapFileHeader*)Buffer)->bfOffBits;
        HeaderVersion = ((FBitmapFileHeader*)Buffer)->GetHeaderVersion();
    } else {
        bmhdr = (FBitmapInfoHeader*)Buffer;
        Bits = Buffer + sizeof(FBitmapInfoHeader);
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
        const uint8_t* bmpal = (uint8_t*)CompressedData.data() + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader);

        // Set texture properties.
        Width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        Height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        Format = ERGBFormat::BGRA;
        RawData.resize(Height * Width * 4);

        FColor* ImageData = (FColor*)RawData.data();

        // If the number for color palette entries is 0, we need to default to 2^biBitCount entries.  In this case 2^8 = 256
        int clrPaletteCount = bmhdr->biClrUsed ? bmhdr->biClrUsed : 256;
        std::vector<FColor> Palette;

        for (int i = 0; i < clrPaletteCount; i++) {
            FColor color;
            color.r = bmpal[i * 4 + 2];
            color.g = bmpal[i * 4 + 1];
            color.b = bmpal[i * 4 + 0];
            color.a = 255;
            Palette.push_back(std::move(color));
        }

        while (Palette.size() < 256) {
            FColor color;
            color.r = 0;
            color.g = 0;
            color.b = 0;
            color.a = 255;
            Palette.push_back(std::move(color));
        }

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int SrcStride = Align(Width, 4);
        const int SrcPtrDiff = bNegativeHeight ? SrcStride : -SrcStride;
        const uint8_t* SrcPtr = Bits + (bNegativeHeight ? 0 : Height - 1) * SrcStride;

        for (int Y = 0; Y < Height; Y++) {
            for (int X = 0; X < Width; X++) {
                *ImageData++ = Palette[SrcPtr[X]];
            }

            SrcPtr += SrcPtrDiff;
        }
    } else if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 24) {
        // Set texture properties.
        Width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        Height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        Format = ERGBFormat::BGRA;
        RawData.resize(Height * Width * 4);

        uint8_t* ImageData = RawData.data();

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int SrcStride = Align(Width * 3, 4);
        const int SrcPtrDiff = bNegativeHeight ? SrcStride : -SrcStride;
        const uint8_t* SrcPtr = Bits + (bNegativeHeight ? 0 : Height - 1) * SrcStride;

        for (int Y = 0; Y < Height; Y++) {
            const uint8_t* SrcRowPtr = SrcPtr;
            for (int X = 0; X < Width; X++) {
                *ImageData++ = *SrcRowPtr++;
                *ImageData++ = *SrcRowPtr++;
                *ImageData++ = *SrcRowPtr++;
                *ImageData++ = 0xFF;
            }

            SrcPtr += SrcPtrDiff;
        }
    } else if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 32) {
        // Set texture properties.
        Width = bmhdr->biWidth;
        const bool bNegativeHeight = (bmhdr->biHeight < 0);
        Height = abs(bHalfHeight ? bmhdr->biHeight / 2 : bmhdr->biHeight);
        Format = ERGBFormat::BGRA;
        RawData.resize(Height * Width * 4);

        uint8_t* ImageData = RawData.data();

        // Copy scanlines, accounting for scanline direction according to the Height field.
        const int SrcStride = Width * 4;
        const int SrcPtrDiff = bNegativeHeight ? SrcStride : -SrcStride;
        const uint8_t* SrcPtr = Bits + (bNegativeHeight ? 0 : Height - 1) * SrcStride;

        // Getting the bmiColors member from the BITMAPINFO, which is used as a mask on BitFields compression.
        const FBmiColorsMask* ColorMask = (FBmiColorsMask*)(CompressedData.data() + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader));
        // Header version 4 introduced the option to declare custom color space, so we can't just assume sRGB past that version.
        const bool bAssumeRGBCompression = bmhdr->biCompression == BCBI_RGB || (bmhdr->biCompression == BCBI_BITFIELDS && ColorMask->IsMaskRGB8() && HeaderVersion < EBitmapHeaderVersion::BHV_BITMAPV4HEADER);

        if (bAssumeRGBCompression) {
            for (int Y = 0; Y < Height; Y++) {
                const uint8_t* SrcRowPtr = SrcPtr;
                for (int X = 0; X < Width; X++) {
                    *ImageData++ = *SrcRowPtr++;
                    *ImageData++ = *SrcRowPtr++;
                    *ImageData++ = *SrcRowPtr++;
                    *ImageData++ = 0xFF;  // In BCBI_RGB compression the last 8 bits of the pixel are not used.
                    SrcRowPtr++;
                }

                SrcPtr += SrcPtrDiff;
            }
        } else if (bmhdr->biCompression == BCBI_BITFIELDS) {
            // If the header version is V4 or higher we need to make sure we are still using sRGB format
            if (HeaderVersion >= EBitmapHeaderVersion::BHV_BITMAPV4HEADER) {
                const FBitmapInfoHeaderV4* bmhdrV4 = (FBitmapInfoHeaderV4*)(Buffer + sizeof(FBitmapFileHeader));

                if (bmhdrV4->biCSType != (uint32_t)EBitmapCSType::BCST_LCS_sRGB && bmhdrV4->biCSType != (uint32_t)EBitmapCSType::BCST_LCS_WINDOWS_COLOR_SPACE) {
                    std::cerr << "BMP uses an unsupported custom color space definition, sRGB color space will be used instead." << std::endl;
                }
            }

            // Calculating the bit mask info needed to remap the pixels' color values.
            uint32_t TrailingBits[4];
            float MappingRatio[4];
            for (uint32_t MaskIndex = 0; MaskIndex < 4; MaskIndex++) {
                TrailingBits[MaskIndex] = CountTrailingZeros(ColorMask->RGBAMask[MaskIndex]);
                const uint32_t NumberOfBits = 32 - (TrailingBits[MaskIndex] + CountLeadingZeros(ColorMask->RGBAMask[MaskIndex]));
                MappingRatio[MaskIndex] = NumberOfBits == 0 ? 0 : (exp2(8) - 1) / (exp2(NumberOfBits) - 1);
            }

            // In header pre-version 4, we should ignore the last 32bit (alpha) content.
            const bool bHasAlphaChannel = ColorMask->RGBAMask[3] != 0 && HeaderVersion >= EBitmapHeaderVersion::BHV_BITMAPV4HEADER;

            for (int Y = 0; Y < Height; Y++) {
                const uint32_t* SrcPixel = (uint32_t*)SrcPtr;
                for (int X = 0; X < Width; X++) {
                    // Set the color values in BGRA order.
                    for (int ColorIndex = 2; ColorIndex >= 0; ColorIndex--) {
                        *ImageData++ = (int)(round(((*SrcPixel & ColorMask->RGBAMask[ColorIndex]) >> TrailingBits[ColorIndex]) * MappingRatio[ColorIndex]));
                    }

                    *ImageData++ = bHasAlphaChannel ? (int)(round(((*SrcPixel & ColorMask->RGBAMask[3]) >> TrailingBits[3]) * MappingRatio[3])) : 0xFF;

                    SrcPixel++;
                }

                SrcPtr += SrcPtrDiff;
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

bool FBmpImageWrapper::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(InCompressedData, InCompressedSize);

    return bResult && (bHasHeader ? LoadBMPHeader() : LoadBMPInfoHeader());  // Fetch the variables from the header info
}

bool FBmpImageWrapper::LoadBMPHeader() {
    const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader*)(CompressedData.data() + sizeof(FBitmapFileHeader));
    const FBitmapFileHeader* bmf = (FBitmapFileHeader*)(CompressedData.data() + 0);
    if ((CompressedData.size() >= sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader)) && CompressedData.data()[0] == 'B' && CompressedData.data()[1] == 'M') {
        if (bmhdr->biCompression != BCBI_RGB && bmhdr->biCompression != BCBI_BITFIELDS) {
            std::cerr << "RLE compression of BMP images not supported" << std::endl;
            return false;
        }

        if (bmhdr->biPlanes == 1 && (bmhdr->biBitCount == 8 || bmhdr->biBitCount == 24 || bmhdr->biBitCount == 32)) {
            // Set texture properties.
            Width = bmhdr->biWidth;
            Height = abs(bmhdr->biHeight);
            Format = ERGBFormat::BGRA;
            BitDepth = bmhdr->biBitCount;

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
    const FBitmapInfoHeader* bmhdr = (FBitmapInfoHeader*)CompressedData.data();

    if (bmhdr->biCompression != BCBI_RGB && bmhdr->biCompression != BCBI_BITFIELDS) {
        std::cerr << "RLE compression of BMP images not supported" << std::endl;
        return false;
    }

    if (bmhdr->biPlanes == 1 && (bmhdr->biBitCount == 8 || bmhdr->biBitCount == 24 || bmhdr->biBitCount == 32)) {
        // Set texture properties.
        Width = bmhdr->biWidth;
        Height = abs(bmhdr->biHeight);
        Format = ERGBFormat::BGRA;
        BitDepth = bmhdr->biBitCount;

        return true;
    }

    if (bmhdr->biPlanes == 1 && bmhdr->biBitCount == 16) {
        std::cerr << "BMP 16 bit format no longer supported. Use terrain tools for importing/exporting heightmaps." << std::endl;
    } else {
        std::cerr << "BMP uses an unsupported format (" << bmhdr->biPlanes << "/" << bmhdr->biBitCount << ")" << std::endl;
    }

    return false;
}
