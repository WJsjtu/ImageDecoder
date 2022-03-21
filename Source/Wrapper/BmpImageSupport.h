#pragma once
#include <cstdint>
#include "Decoder.h"

namespace ImageDecoder {

struct FBitmapFileHeader;
struct FBitmapInfoHeader;

// Bitmap compression types.
enum EBitmapCompression {
    BCBI_RGB = 0,
    BCBI_RLE8 = 1,
    BCBI_RLE4 = 2,
    BCBI_BITFIELDS = 3,
    BCBI_JPEG = 4,
    BCBI_PNG = 5,
    BCBI_ALPHABITFIELDS = 6,
};

// Bitmap info header versions.
enum class EBitmapHeaderVersion : uint8_t {
    BHV_BITMAPINFOHEADER = 0,
    BHV_BITMAPV2INFOHEADER = 1,
    BHV_BITMAPV3INFOHEADER = 2,
    BHV_BITMAPV4HEADER = 3,
    BHV_BITMAPV5HEADER = 4,
};

// Color space type of the bitmap, property introduced in Bitmap header version 4.
enum class EBitmapCSType : uint32_t {
    BCST_BLCS_CALIBRATED_RGB = 0x00000000,
    BCST_LCS_sRGB = 0x73524742,
    BCST_LCS_WINDOWS_COLOR_SPACE = 0x57696E20,
    BCST_PROFILE_LINKED = 0x4C494E4B,
    BCST_PROFILE_EMBEDDED = 0x4D424544,
};

// .BMP file header.
#pragma pack(push, 1)
struct FBitmapFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;

public:
    EBitmapHeaderVersion GetHeaderVersion() const {
        // Since there is no field indicating the header version of the bitmap in the FileHeader,
        // the only way to know the format version is to check the header offbits size.
        switch (bfOffBits - sizeof(FBitmapFileHeader)) {
            case 40:
            default: return EBitmapHeaderVersion::BHV_BITMAPINFOHEADER;
            case 52: return EBitmapHeaderVersion::BHV_BITMAPV2INFOHEADER;
            case 56: return EBitmapHeaderVersion::BHV_BITMAPV3INFOHEADER;
            case 108: return EBitmapHeaderVersion::BHV_BITMAPV4HEADER;
            case 124: return EBitmapHeaderVersion::BHV_BITMAPV5HEADER;
        }
    }
};
#pragma pack(pop)

// .BMP subheader.
#pragma pack(push, 1)
struct FBitmapInfoHeader {
    uint32_t biSize;
    uint32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

// .BMP subheader V4
#pragma pack(push, 1)
struct FBitmapInfoHeaderV4 {
    uint32_t biSize;
    uint32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
    uint32_t biRedMask;
    uint32_t biGreenMask;
    uint32_t biBlueMask;
    uint32_t biAlphaMask;
    uint32_t biCSType;
    int biEndPointRedX;
    int biEndPointRedY;
    int bibiEndPointRedZ;
    int bibiEndPointGreenX;
    int biEndPointGreenY;
    int biEndPointGreenZ;
    int biEndPointBlueX;
    int biEndPointBlueY;
    int biEndPointBlueZ;
    uint32_t biGammaRed;
    uint32_t biGammaGreen;
    uint32_t biGammaBlue;
};
#pragma pack(pop)

#pragma pack(push, 1)
// Used by InfoHeaders pre-version 4, a structure that is declared after the FBitmapInfoHeader.
struct FBmiColorsMask {
    // RGBA, in header pre-version 4, Alpha was only used as padding.
    uint32_t RGBAMask[4];

public:
    bool IsMaskRGB8() const;
};
#pragma pack(pop)
}  // namespace ImageDecoder