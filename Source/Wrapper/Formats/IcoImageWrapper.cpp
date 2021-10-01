#include "IcoImageWrapper.h"

#include "Wrapper/BmpImageSupport.h"
#include "Wrapper/Formats/BmpImageWrapper.h"
#include "Wrapper/Formats/PngImageWrapper.h"

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

void FIcoImageWrapper::Compress(int Quality) { std::cerr << "ICO compression not supported"; }

void FIcoImageWrapper::Uncompress(const ERGBFormat InFormat, const int InBitDepth) {
    const uint8_t* Buffer = CompressedData.data();

    if (ImageOffset != 0 && ImageSize != 0) {
        SubImageWrapper->Uncompress(InFormat, InBitDepth);
    }
}

bool FIcoImageWrapper::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(InCompressedData, InCompressedSize);

    return bResult && LoadICOHeader();  // Fetch the variables from the header info
}

bool FIcoImageWrapper::GetRaw(const ERGBFormat InFormat, int InBitDepth, std::vector<uint8_t>& OutRawData) {
    LastError.clear();
    Uncompress(InFormat, InBitDepth);

    if (LastError.empty()) {
        SubImageWrapper->MoveRawData(OutRawData);
    }

    return LastError.empty();
}

/* FImageWrapper implementation
 *****************************************************************************/

bool FIcoImageWrapper::LoadICOHeader() {
    const uint8_t* Buffer = CompressedData.data();

    std::shared_ptr<FPngImageWrapper> PngWrapper = std::make_shared<FPngImageWrapper>();
    std::shared_ptr<FBmpImageWrapper> BmpWrapper = std::make_shared<FBmpImageWrapper>(false, true);

    bool bFoundImage = false;
    const FIconDir* IconHeader = (FIconDir*)(Buffer);

    if (IconHeader->idReserved == 0 && IconHeader->idType == 1) {
        // use the largest-width 32-bit dir entry we find
        uint32_t LargestWidth = 0;
        const FIconDirEntry* IconDirEntry = IconHeader->idEntries;

        for (int Entry = 0; Entry < (int)IconHeader->idCount; Entry++, IconDirEntry++) {
            const uint32_t RealWidth = IconDirEntry->bWidth == 0 ? 256 : IconDirEntry->bWidth;
            if (IconDirEntry->wBitCount == 32 && RealWidth > LargestWidth) {
                if (PngWrapper->SetCompressed(Buffer + IconDirEntry->dwImageOffset, (int)IconDirEntry->dwBytesInRes)) {
                    Width = PngWrapper->GetWidth();
                    Height = PngWrapper->GetHeight();
                    Format = PngWrapper->GetFormat();
                    LargestWidth = RealWidth;
                    bFoundImage = true;
                    bIsPng = true;
                    ImageOffset = IconDirEntry->dwImageOffset;
                    ImageSize = IconDirEntry->dwBytesInRes;
                } else if (BmpWrapper->SetCompressed(Buffer + IconDirEntry->dwImageOffset, (int)IconDirEntry->dwBytesInRes)) {
                    // otherwise this should be a BMP icon
                    Width = BmpWrapper->GetWidth();
                    Height = BmpWrapper->GetHeight() / 2;  // ICO file spec says to divide by 2 here as height refers to combined image & mask height
                    Format = BmpWrapper->GetFormat();
                    LargestWidth = RealWidth;
                    bFoundImage = true;
                    bIsPng = false;
                    ImageOffset = IconDirEntry->dwImageOffset;
                    ImageSize = IconDirEntry->dwBytesInRes;
                }
            }
        }
    }

    if (bFoundImage) {
        if (bIsPng) {
            SubImageWrapper = PngWrapper;
        } else {
            SubImageWrapper = BmpWrapper;
        }
    }

    return bFoundImage;
}
