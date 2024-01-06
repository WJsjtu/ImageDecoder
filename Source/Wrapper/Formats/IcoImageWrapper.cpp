#include "IcoImageWrapper.h"

#include "Wrapper/BmpImageSupport.h"
#include "Wrapper/Formats/BmpImageWrapper.h"
#include "Wrapper/Formats/PngImageWrapper.h"
#include "Utils/Utils.h"

namespace ImageDecoder {
#pragma pack(push, 1)
struct FIconDirEntry {
    uint8_t bWidth;          // Width, in pixels, of the image
    uint8_t bHeight;         // Height, in pixels, of the image
    uint8_t bColorCount;     // Number of colors in image (0 if >=8bpp)
    uint8_t bReserved;       // Reserved ( must be 0)
    uint16_t wPlanes;        // Color Planes
    uint16_t wBitCount;      // Bits per pixel
    uint32_t dwBytesInRes;   // How many bytes in this resource?
    uint32_t dwImageOffset;  // Where in the file is this image?
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FIconDir {
    uint16_t idReserved;         // Reserved (must be 0)
    uint16_t idType;             // Resource Type (1 for icons)
    uint16_t idCount;            // How many images?
    FIconDirEntry idEntries[1];  // An entry for each image (idCount of 'em)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FRGBQuad {
    uint8_t rgbBlue;      // Blue channel
    uint8_t rgbGreen;     // Green channel
    uint8_t rgbRed;       // Red channel
    uint8_t rgbReserved;  // Reserved (alpha)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FIconImage {
    FBitmapInfoHeader icHeader;  // DIB header
    FRGBQuad icColors[1];        // Color table
    uint8_t icXOR[1];            // DIB bits for XOR mask
    uint8_t icAND[1];            // DIB bits for AND mask
};
#pragma pack(pop)

/* FJpegImageWrapper structors
 *****************************************************************************/

FIcoImageWrapper::FIcoImageWrapper() : FImageWrapperBase() {}

/* FImageWrapper interface
 *****************************************************************************/

void FIcoImageWrapper::Compress(int quality) { LogMessage(ELogLevel::Error, "ICO compression not supported."); }

void FIcoImageWrapper::Uncompress(const ERGBFormat inFormat, const int inBitDepth) {
    const uint8_t* buffer = compressedData.data();

    if (imageOffset != 0 && imageSize != 0) {
        subImageWrapper->Uncompress(inFormat, inBitDepth);
    }
}

bool FIcoImageWrapper::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(inCompressedData, inCompressedSize);

    return bResult && LoadICOHeader();  // Fetch the variables from the header info
}

bool FIcoImageWrapper::GetRaw(const ERGBFormat inFormat, int inBitDepth, std::vector<uint8_t>& outRawData) {
    lastError.clear();
    Uncompress(inFormat, inBitDepth);

    if (lastError.empty()) {
        subImageWrapper->MoveRawData(outRawData);
    }

    return lastError.empty();
}

/* FImageWrapper implementation
 *****************************************************************************/

bool FIcoImageWrapper::LoadICOHeader() {
    const uint8_t* buffer = compressedData.data();

    std::shared_ptr<FPngImageWrapper> pngWrapper = std::make_shared<FPngImageWrapper>();
    std::shared_ptr<FBmpImageWrapper> bmpWrapper = std::make_shared<FBmpImageWrapper>(false, true);

    bool bFoundImage = false;
    const FIconDir* iconHeader = (FIconDir*)(buffer);

    if (iconHeader->idReserved == 0 && iconHeader->idType == 1) {
        // use the largest-width 32-bit dir entry we find
        uint32_t largestWidth = 0;
        const FIconDirEntry* iconDirEntry = iconHeader->idEntries;

        for (int entry = 0; entry < (int)iconHeader->idCount; entry++, iconDirEntry++) {
            const uint32_t realWidth = iconDirEntry->bWidth == 0 ? 256 : iconDirEntry->bWidth;
            if (iconDirEntry->wBitCount == 32 && realWidth > largestWidth) {
                if (pngWrapper->SetCompressed(buffer + iconDirEntry->dwImageOffset, (int)iconDirEntry->dwBytesInRes)) {
                    width = pngWrapper->GetWidth();
                    height = pngWrapper->GetHeight();
                    format = pngWrapper->GetFormat();
                    largestWidth = realWidth;
                    bFoundImage = true;
                    bIsPng = true;
                    imageOffset = iconDirEntry->dwImageOffset;
                    imageSize = iconDirEntry->dwBytesInRes;
                } else if (bmpWrapper->SetCompressed(buffer + iconDirEntry->dwImageOffset, (int)iconDirEntry->dwBytesInRes)) {
                    // otherwise this should be a BMP icon
                    width = bmpWrapper->GetWidth();
                    height = bmpWrapper->GetHeight() / 2;  // ICO file spec says to divide by 2 here as height refers to combined image & mask height
                    format = bmpWrapper->GetFormat();
                    largestWidth = realWidth;
                    bFoundImage = true;
                    bIsPng = false;
                    imageOffset = iconDirEntry->dwImageOffset;
                    imageSize = iconDirEntry->dwBytesInRes;
                }
            }
        }
    }

    if (bFoundImage) {
        if (bIsPng) {
            subImageWrapper = pngWrapper;
        } else {
            subImageWrapper = bmpWrapper;
        }
    }

    return bFoundImage;
}
}  // namespace ImageDecoder
