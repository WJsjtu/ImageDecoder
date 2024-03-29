﻿#include "Decoder.h"
#include <algorithm>
#include <string>
#include <memory>
#include <unordered_map>
#include "Wrapper/Formats/BmpImageWrapper.h"
#include "Wrapper/Formats/ExrImageWrapper.h"
#include "Wrapper/Formats/IcoImageWrapper.h"
#include "Wrapper/Formats/JpegImageWrapper.h"
#include "Wrapper/Formats/PngImageWrapper.h"
#include "Wrapper/ImageWrapperBase.h"
#include "Utils/Utils.h"

namespace ImageDecoder {

struct ImagePixelsMemData {
    std::shared_ptr<ImagePixelData> pixels;
    std::vector<uint8_t> data;
};

std::unordered_map<void*, std::shared_ptr<ImagePixelsMemData>> decoded_image_mmem_data_pool;

std::shared_ptr<ImagePixelsMemData> AllocPixels() {
    std::shared_ptr<ImagePixelsMemData> result = std::make_shared<ImagePixelsMemData>();
    std::shared_ptr<ImagePixelData> pixels = std::make_shared<ImagePixelData>();
    result->pixels = pixels;
    decoded_image_mmem_data_pool.emplace(pixels.get(), result);
    return result;
}

// .PCX file header.
#pragma pack(push, 1)
class FPCXFileHeader {
public:
    uint8_t manufacturer;     // Always 10.
    uint8_t version;          // PCX file version.
    uint8_t encoding;         // 1=run-length, 0=none.
    uint8_t bitsPerPixel;     // 1,2,4, or 8.
    uint16_t xMin;            // Dimensions of the image.
    uint16_t yMin;            // Dimensions of the image.
    uint16_t xMax;            // Dimensions of the image.
    uint16_t yMax;            // Dimensions of the image.
    uint16_t xDotsPerInch;    // Horizontal printer resolution.
    uint16_t yDotsPerInch;    // Vertical printer resolution.
    uint8_t oldColorMap[48];  // Old colormap info data.
    uint8_t reserved1;        // Must be 0.
    uint8_t numPlanes;        // Number of color planes (1, 3, 4, etc).
    uint16_t bytesPerLine;    // Number of bytes per scanline.
    uint16_t paletteType;     // How to interpret palette: 1=color, 2=gray.
    uint16_t hScreenSize;     // Horizontal monitor size.
    uint16_t vScreenSize;     // Vertical monitor size.
    uint8_t reserved2[54];    // Must be 0.
};

#pragma pack(push, 1)

struct FTGAFileHeader {
    uint8_t idFieldLength;
    uint8_t colorMapType;
    uint8_t imageTypeCode;  // 2 for uncompressed RGB format
    uint16_t colorMapOrigin;
    uint16_t colorMapLength;
    uint8_t colorMapEntrySize;
    uint16_t xOrigin;
    uint16_t yOrigin;
    uint16_t width;
    uint16_t height;
    uint8_t bitsPerPixel;
    uint8_t imageDescriptor;
};

// Output B8-G8-R8-A8
void DecompressTGA_RLE_32bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* idData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = idData + TGA->idFieldLength;
    uint8_t* imageData = (uint8_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);
    uint32_t pixel = 0;
    int RLERun = 0;
    int RAWRun = 0;

    for (int y = TGA->height - 1; y >= 0; y--)  // Y-flipped.
    {
        for (int x = 0; x < TGA->width; x++) {
            if (RLERun > 0) {
                RLERun--;            // reuse current Pixel data.
            } else if (RAWRun == 0)  // new raw pixel or RLE-run.
            {
                uint8_t RLEChunk = *(imageData++);
                if (RLEChunk & 0x80) {
                    RLERun = (RLEChunk & 0x7F) + 1;
                    RAWRun = 1;
                } else {
                    RAWRun = (RLEChunk & 0x7F) + 1;
                }
            }
            // Retrieve new pixel data - raw run or single pixel for RLE stretch.
            if (RAWRun > 0) {
                pixel = *(uint32_t*)imageData;  // RGBA 32-bit dword.
                imageData += 4;
                RAWRun--;
                RLERun--;
            }
            // Store.
            *((textureData + y * TGA->width) + x) = pixel;
        }
    }
}

// Output B8-G8-R8-A8(255)
void DecompressTGA_RLE_24bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = IdData + TGA->idFieldLength;
    uint8_t* imageData = (uint8_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);
    uint8_t pixel[4] = {};
    int RLERun = 0;
    int RAWRun = 0;

    for (int y = TGA->height - 1; y >= 0; y--)  // Y-flipped.
    {
        for (int x = 0; x < TGA->width; x++) {
            if (RLERun > 0)
                RLERun--;          // reuse current Pixel data.
            else if (RAWRun == 0)  // new raw pixel or RLE-run.
            {
                uint8_t RLEChunk = *(imageData++);
                if (RLEChunk & 0x80) {
                    RLERun = (RLEChunk & 0x7F) + 1;
                    RAWRun = 1;
                } else {
                    RAWRun = (RLEChunk & 0x7F) + 1;
                }
            }
            // Retrieve new pixel data - raw run or single pixel for RLE stretch.
            if (RAWRun > 0) {
                pixel[0] = *(imageData++);
                pixel[1] = *(imageData++);
                pixel[2] = *(imageData++);
                pixel[3] = 255;
                RAWRun--;
                RLERun--;
            }
            // Store.
            *((textureData + y * TGA->width) + x) = *(uint32_t*)&pixel;
        }
    }
}

// Output B8-G8-R8-A8
void DecompressTGA_RLE_16bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = IdData + TGA->idFieldLength;
    uint16_t* imageData = (uint16_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);
    uint16_t filePixel = 0;
    uint32_t texturePixel = 0;
    int RLERun = 0;
    int RAWRun = 0;

    for (int y = TGA->height - 1; y >= 0; y--)  // Y-flipped.
    {
        for (int x = 0; x < TGA->width; x++) {
            if (RLERun > 0)
                RLERun--;          // reuse current Pixel data.
            else if (RAWRun == 0)  // new raw pixel or RLE-run.
            {
                uint8_t RLEChunk = *((uint8_t*)imageData);
                imageData = (uint16_t*)(((uint8_t*)imageData) + 1);
                if (RLEChunk & 0x80) {
                    RLERun = (RLEChunk & 0x7F) + 1;
                    RAWRun = 1;
                } else {
                    RAWRun = (RLEChunk & 0x7F) + 1;
                }
            }
            // Retrieve new pixel data - raw run or single pixel for RLE stretch.
            if (RAWRun > 0) {
                filePixel = *(imageData++);
                RAWRun--;
                RLERun--;
            }
            // Convert file format A1R5G5B5 into pixel format B8G8R8A8
            texturePixel = (filePixel & 0x001F) << 3;
            texturePixel |= (filePixel & 0x03E0) << 6;
            texturePixel |= (filePixel & 0x7C00) << 9;
            texturePixel |= (filePixel & 0x8000) << 16;
            // Store.
            *((textureData + y * TGA->width) + x) = texturePixel;
        }
    }
}

// Output B8-G8-R8-A8
void DecompressTGA_32bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = IdData + TGA->idFieldLength;
    uint32_t* imageData = (uint32_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);

    for (int Y = 0; Y < TGA->height; Y++) {
        memcpy(textureData + Y * TGA->width, imageData + (TGA->height - Y - 1) * TGA->width, TGA->width * 4);
    }
}

// Output B8-G8-R8-A8
void DecompressTGA_16bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = IdData + TGA->idFieldLength;
    uint16_t* imageData = (uint16_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);
    uint16_t filePixel = 0;
    uint32_t texturePixel = 0;

    for (int y = TGA->height - 1; y >= 0; y--) {
        for (int x = 0; x < TGA->width; x++) {
            filePixel = *imageData++;
            // Convert file format A1R5G5B5 into pixel format B8G8R8A8
            texturePixel = (filePixel & 0x001F) << 3;
            texturePixel |= (filePixel & 0x03E0) << 6;
            texturePixel |= (filePixel & 0x7C00) << 9;
            texturePixel |= (filePixel & 0x8000) << 16;
            // Store.
            *((textureData + y * TGA->width) + x) = texturePixel;
        }
    }
}

// Output B8-G8-R8-A8(255)
void DecompressTGA_24bpp(const FTGAFileHeader* TGA, uint32_t* textureData) {
    uint8_t* IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    uint8_t* colorMap = IdData + TGA->idFieldLength;
    uint8_t* imageData = (uint8_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);
    uint8_t pixel[4];

    for (int y = 0; y < TGA->height; y++) {
        for (int x = 0; x < TGA->width; x++) {
            pixel[0] = *((imageData + (TGA->height - y - 1) * TGA->width * 3) + x * 3 + 0);
            pixel[1] = *((imageData + (TGA->height - y - 1) * TGA->width * 3) + x * 3 + 1);
            pixel[2] = *((imageData + (TGA->height - y - 1) * TGA->width * 3) + x * 3 + 2);
            pixel[3] = 255;
            *((textureData + y * TGA->width) + x) = *(uint32_t*)&pixel;
        }
    }
}

void DecompressTGA_8bpp(const FTGAFileHeader* TGA, uint8_t* textureData) {
    const uint8_t* const IdData = (uint8_t*)TGA + sizeof(FTGAFileHeader);
    const uint8_t* const colorMap = IdData + TGA->idFieldLength;
    const uint8_t* const imageData = (uint8_t*)(colorMap + (TGA->colorMapEntrySize + 4) / 8 * TGA->colorMapLength);

    int RevY = 0;
    for (int y = TGA->height - 1; y >= 0; --y) {
        const uint8_t* ImageCol = imageData + (y * TGA->width);
        uint8_t* TextureCol = textureData + (RevY++ * TGA->width);
        memcpy(TextureCol, ImageCol, TGA->width);
    }
}

bool DecompressTGA_helper(const FTGAFileHeader* TGA, uint32_t*& textureData, const int textureDataSize) {
    if (TGA->imageTypeCode == 10)  // 10 = RLE compressed
    {
        // RLE compression: CHUNKS: 1 -byte header, high bit 0 = raw, 1 = compressed
        // bits 0-6 are a 7-bit count; count+1 = number of raw pixels following, or rle pixels to be expanded.
        if (TGA->bitsPerPixel == 32) {
            DecompressTGA_RLE_32bpp(TGA, textureData);
        } else if (TGA->bitsPerPixel == 24) {
            DecompressTGA_RLE_24bpp(TGA, textureData);
        } else if (TGA->bitsPerPixel == 16) {
            DecompressTGA_RLE_16bpp(TGA, textureData);
        } else {
            std::string error = "TGA uses an unsupported rle-compressed bit-depth: " + std::to_string(TGA->bitsPerPixel);
            LogMessage(ELogLevel::Error, error.data());
            return false;
        }
    } else if (TGA->imageTypeCode == 2)  // 2 = Uncompressed RGB
    {
        if (TGA->bitsPerPixel == 32) {
            DecompressTGA_32bpp(TGA, textureData);
        } else if (TGA->bitsPerPixel == 16) {
            DecompressTGA_16bpp(TGA, textureData);
        } else if (TGA->bitsPerPixel == 24) {
            DecompressTGA_24bpp(TGA, textureData);
        } else {
            std::string error = "TGA uses an unsupported bit-depth: " + std::to_string(TGA->bitsPerPixel);
            LogMessage(ELogLevel::Error, error.data());
            return false;
        }
    }
    // Support for alpha stored as pseudo-color 8-bit TGA
    else if (TGA->colorMapType == 1 && TGA->imageTypeCode == 1 && TGA->bitsPerPixel == 8) {
        DecompressTGA_8bpp(TGA, (uint8_t*)textureData);
    }
    // standard grayscale
    else if (TGA->colorMapType == 0 && TGA->imageTypeCode == 3 && TGA->bitsPerPixel == 8) {
        DecompressTGA_8bpp(TGA, (uint8_t*)textureData);
    } else {
        std::string error = "TGA is an unsupported type: " + std::to_string(TGA->imageTypeCode);
        LogMessage(ELogLevel::Error, error.data());
        return false;
    }

    // Flip the image data if the flip bits are set in the TGA header.
    bool flipX = (TGA->imageDescriptor & 0x10) ? 1 : 0;
    bool flipY = (TGA->imageDescriptor & 0x20) ? 1 : 0;
    if (flipY || flipX) {
        std::vector<uint8_t> FlippedData;
        FlippedData.resize(textureDataSize);

        int numBlocksX = TGA->width;
        int numBlocksY = TGA->height;
        int blockBytes = TGA->bitsPerPixel == 8 ? 1 : 4;

        uint8_t* mipData = (uint8_t*)textureData;

        for (int Y = 0; Y < numBlocksY; Y++) {
            for (int X = 0; X < numBlocksX; X++) {
                int DestX = flipX ? (numBlocksX - X - 1) : X;
                int DestY = flipY ? (numBlocksY - Y - 1) : Y;
                memcpy(&FlippedData[(DestX + DestY * numBlocksX) * blockBytes], &mipData[(X + Y * numBlocksX) * blockBytes], blockBytes);
            }
        }
        memcpy(mipData, FlippedData.data(), FlippedData.size());
    }

    return true;
}

bool DecompressTGA(const FTGAFileHeader* TGA, std::shared_ptr<ImagePixelsMemData>& PixelsMemData) {
    if (TGA->colorMapType == 1 && TGA->imageTypeCode == 1 && TGA->bitsPerPixel == 8) {
        PixelsMemData = AllocPixels();
        // Notes: The Scaleform GFx exporter (dll) strips all font glyphs into a single 8-bit texture.
        // The targa format uses this for a palette index; GFx uses a palette of (i,i,i,i) so the index
        // is also the alpha value.
        //
        // We store the image as PF_G8, where it will be used as alpha in the Glyph shader.
        PixelsMemData->pixels->width = TGA->width;
        PixelsMemData->pixels->height = TGA->height;
        PixelsMemData->pixels->texture_format = ETextureSourceFormat::G8;
        PixelsMemData->pixels->bit_depth = 8;
        PixelsMemData->pixels->bit_depth = 8;
        PixelsMemData->data.resize(TGA->width * TGA->height * 1);
        PixelsMemData->pixels->data = PixelsMemData->data.data();
        PixelsMemData->pixels->size = PixelsMemData->data.size();
    } else if (TGA->colorMapType == 0 && TGA->imageTypeCode == 3 && TGA->bitsPerPixel == 8) {
        // standard grayscale images
        PixelsMemData->pixels->width = TGA->width;
        PixelsMemData->pixels->height = TGA->height;
        PixelsMemData->pixels->texture_format = ETextureSourceFormat::G8;
        PixelsMemData->pixels->bit_depth = 8;
        PixelsMemData->data.resize(TGA->width * TGA->height * 1);
        PixelsMemData->pixels->data = PixelsMemData->data.data();
        PixelsMemData->pixels->size = PixelsMemData->data.size();
    } else {
        if (TGA->imageTypeCode == 10)  // 10 = RLE compressed
        {
            if (TGA->bitsPerPixel != 32 && TGA->bitsPerPixel != 24 && TGA->bitsPerPixel != 16) {
                std::string error = "TGA uses an unsupported rle-compressed bit-depth: " + std::to_string(TGA->bitsPerPixel);
                LogMessage(ELogLevel::Error, error.data());
                return false;
            }
        } else {
            if (TGA->bitsPerPixel != 32 && TGA->bitsPerPixel != 16 && TGA->bitsPerPixel != 24) {
                std::string error = "TGA uses an unsupported bit-depth: " + std::to_string(TGA->bitsPerPixel);
                LogMessage(ELogLevel::Error, error.data());
                return false;
            }
        }

        PixelsMemData->pixels->width = TGA->width;
        PixelsMemData->pixels->height = TGA->height;
        PixelsMemData->pixels->texture_format = ETextureSourceFormat::BGRA8;
        PixelsMemData->pixels->bit_depth = 8;
        PixelsMemData->data.resize(TGA->width * TGA->height * 4);
        PixelsMemData->pixels->data = PixelsMemData->data.data();
        PixelsMemData->pixels->size = PixelsMemData->data.size();
    }

    size_t TextureDataSize = PixelsMemData->data.size();
    uint32_t* TextureData = (uint32_t*)PixelsMemData->pixels->data;

    return DecompressTGA_helper(TGA, TextureData, static_cast<int>(TextureDataSize));
}

bool DecodeImage(EImageFormat imageFormat, const uint8_t* buffer, uint32_t length, ImageInfo& info, std::shared_ptr<ImagePixelsMemData>& PixelsMemData) {
    //
    // PNG
    //
    if (imageFormat == EImageFormat::PNG) {
        std::shared_ptr<IImageWrapper> pngImageWrapper = std::make_shared<FPngImageWrapper>();
        if (pngImageWrapper && pngImageWrapper->SetCompressed(buffer, length)) {
            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = pngImageWrapper->GetBitDepth();
            ERGBFormat format = pngImageWrapper->GetFormat();
            info.type = EImageFormat::PNG;
            info.rgb_format = format;
            info.bit_depth = bitDepth;

            if (format == ERGBFormat::Gray) {
                if (bitDepth <= 8) {
                    textureFormat = ETextureSourceFormat::RGBA8;
                    format = ERGBFormat::RGBA;
                    bitDepth = 8;
                } else if (bitDepth == 16) {
                    textureFormat = ETextureSourceFormat::RGBA16;
                    format = ERGBFormat::RGBA;
                    bitDepth = 16;
                }
            } else if (format == ERGBFormat::RGBA || format == ERGBFormat::BGRA) {
                if (bitDepth <= 8) {
                    textureFormat = ETextureSourceFormat::RGBA8;
                    format = ERGBFormat::RGBA;
                    bitDepth = 8;
                } else if (bitDepth == 16) {
                    textureFormat = ETextureSourceFormat::RGBA16;
                    format = ERGBFormat::RGBA;
                    bitDepth = 16;
                }
            }

            if (textureFormat == ETextureSourceFormat::Invalid) {
                LogMessage(ELogLevel::Error, "PNG file contains data in an unsupported format.");
                return false;
            }

            PixelsMemData = AllocPixels();
            info.width = PixelsMemData->pixels->width = pngImageWrapper->GetWidth();
            info.height = PixelsMemData->pixels->height = pngImageWrapper->GetHeight();
            PixelsMemData->pixels->texture_format = textureFormat;
            PixelsMemData->pixels->bit_depth = bitDepth;

            if (!pngImageWrapper->GetRaw(format, bitDepth, PixelsMemData->data)) {
                decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
                PixelsMemData = nullptr;
                LogMessage(ELogLevel::Error, "Failed to decode PNG.");
                return false;
            }
            PixelsMemData->pixels->data = PixelsMemData->data.data();
            PixelsMemData->pixels->size = PixelsMemData->data.size();
            return true;
        }
    }

    //
    // JPEG
    //
    if (imageFormat == EImageFormat::JPEG) {
        std::shared_ptr<IImageWrapper> jpegImageWrapper = std::make_shared<FJpegImageWrapper>();
        if (jpegImageWrapper && jpegImageWrapper->SetCompressed(buffer, length)) {
            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = jpegImageWrapper->GetBitDepth();
            ERGBFormat format = jpegImageWrapper->GetFormat();
            info.type = EImageFormat::JPEG;
            info.rgb_format = format;
            info.bit_depth = bitDepth;

            if (format == ERGBFormat::Gray) {
                if (bitDepth <= 8) {
                    textureFormat = ETextureSourceFormat::RGBA8;
                    format = ERGBFormat::RGBA;
                    bitDepth = 8;
                } else {
                    textureFormat = ETextureSourceFormat::RGBA16;
                    format = ERGBFormat::RGBA;
                    bitDepth = 16;
                }
            } else if (format == ERGBFormat::RGBA) {
                if (bitDepth <= 8) {
                    textureFormat = ETextureSourceFormat::RGBA8;
                    format = ERGBFormat::RGBA;
                    bitDepth = 8;
                } else {
                    textureFormat = ETextureSourceFormat::RGBA16;
                    format = ERGBFormat::RGBA;
                    bitDepth = 16;
                }
            }

            if (textureFormat == ETextureSourceFormat::Invalid) {
                LogMessage(ELogLevel::Error, "JPEG file contains data in an unsupported format.");
                return false;
            }

            PixelsMemData = AllocPixels();
            info.width = PixelsMemData->pixels->width = jpegImageWrapper->GetWidth();
            info.height = PixelsMemData->pixels->height = jpegImageWrapper->GetHeight();
            PixelsMemData->pixels->texture_format = textureFormat;
            PixelsMemData->pixels->bit_depth = bitDepth;

            if (!jpegImageWrapper->GetRaw(format, bitDepth, PixelsMemData->data)) {
                decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
                PixelsMemData = nullptr;
                LogMessage(ELogLevel::Error, "Failed to decode JPEG.");
                return false;
            }
            PixelsMemData->pixels->data = PixelsMemData->data.data();
            PixelsMemData->pixels->size = PixelsMemData->data.size();
            return true;
        }
    }

    //
    // EXR
    //
    if (imageFormat == EImageFormat::EXR) {
        std::shared_ptr<IImageWrapper> exrImageWrapper = std::make_shared<FExrImageWrapper>();
        if (exrImageWrapper && exrImageWrapper->SetCompressed(buffer, length)) {
            int width = exrImageWrapper->GetWidth();
            int height = exrImageWrapper->GetHeight();

            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = exrImageWrapper->GetBitDepth();
            ERGBFormat format = exrImageWrapper->GetFormat();
            info.type = EImageFormat::EXR;
            info.rgb_format = format;
            info.bit_depth = bitDepth;

            if (format == ERGBFormat::RGBA && bitDepth == 16) {
                textureFormat = ETextureSourceFormat::RGBA16F;
                format = ERGBFormat::RGBA;
            }

            if (textureFormat == ETextureSourceFormat::Invalid) {
                LogMessage(ELogLevel::Error, "EXR file contains data in an unsupported format.");
                return false;
            }

            PixelsMemData = AllocPixels();
            info.width = PixelsMemData->pixels->width = width;
            info.height = PixelsMemData->pixels->height = height;
            PixelsMemData->pixels->texture_format = textureFormat;
            PixelsMemData->pixels->bit_depth = bitDepth;

            if (!exrImageWrapper->GetRaw(format, bitDepth, PixelsMemData->data)) {
                decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
                PixelsMemData = nullptr;
                LogMessage(ELogLevel::Error, "Failed to decode EXR.");
                return false;
            }
            PixelsMemData->pixels->data = PixelsMemData->data.data();
            PixelsMemData->pixels->size = PixelsMemData->data.size();
            return true;
        }
    }

    //
    // BMP
    //
    if (imageFormat == EImageFormat::BMP) {
        std::shared_ptr<IImageWrapper> bmpImageWrapper = std::make_shared<FBmpImageWrapper>();
        if (bmpImageWrapper && bmpImageWrapper->SetCompressed(buffer, length)) {
            int bitDepth = bmpImageWrapper->GetBitDepth();
            ERGBFormat format = bmpImageWrapper->GetFormat();
            info.type = EImageFormat::BMP;
            info.rgb_format = format;
            info.bit_depth = bitDepth;

            PixelsMemData = AllocPixels();
            info.width = PixelsMemData->pixels->width = bmpImageWrapper->GetWidth();
            info.height = PixelsMemData->pixels->height = bmpImageWrapper->GetHeight();
            PixelsMemData->pixels->texture_format = ETextureSourceFormat::BGRA8;
            PixelsMemData->pixels->bit_depth = bmpImageWrapper->GetBitDepth();
            std::vector<uint8_t> decoded;
            if (!bmpImageWrapper->GetRaw(bmpImageWrapper->GetFormat(), bmpImageWrapper->GetBitDepth(), PixelsMemData->data)) {
                decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
                PixelsMemData = nullptr;
                LogMessage(ELogLevel::Error, "Failed to decode BMP.");
                return false;
            }
            PixelsMemData->pixels->data = PixelsMemData->data.data();
            PixelsMemData->pixels->size = PixelsMemData->data.size();
            return true;
        }
    }

    //
    // PCX
    //
    if (imageFormat == EImageFormat::PCX) {
        struct FColor {
            union {
                struct {
                    uint8_t B, G, R, A;
                };
                uint32_t AlignmentDummy;
            };
        };
        const FPCXFileHeader* PCX = (FPCXFileHeader*)buffer;
        if (length >= sizeof(FPCXFileHeader) && PCX->manufacturer == 10) {
            int NewU = PCX->xMax + 1 - PCX->xMin;
            int NewV = PCX->yMax + 1 - PCX->yMin;

            if (PCX->numPlanes == 1 && PCX->bitsPerPixel == 8) {
                info.type = EImageFormat::PCX;
                info.rgb_format = ERGBFormat::BGRA;
                info.bit_depth = 8;
                // Set texture properties.
                PixelsMemData = AllocPixels();
                info.width = PixelsMemData->pixels->width = NewU;
                info.height = PixelsMemData->pixels->height = NewV;
                PixelsMemData->pixels->texture_format = ETextureSourceFormat::BGRA8;
                PixelsMemData->pixels->bit_depth = 8;
                PixelsMemData->data.resize(NewU * NewV * 4);
                PixelsMemData->pixels->data = PixelsMemData->data.data();
                PixelsMemData->pixels->size = PixelsMemData->data.size();
                FColor* DestPtr = (FColor*)PixelsMemData->pixels->data;

                // Import the palette.
                uint8_t* PCXPalette = (uint8_t*)(buffer + length - 256 * 3);
                std::vector<FColor> Palette;
                for (uint32_t i = 0; i < 256; i++) {
                    FColor color;
                    color.R = PCXPalette[i * 3 + 0];
                    color.G = PCXPalette[i * 3 + 1];
                    color.B = PCXPalette[i * 3 + 2];
                    color.A = i == 0 ? 0 : 255;
                    Palette.push_back(color);
                }

                // Import it.
                FColor* DestEnd = DestPtr + NewU * NewV;
                buffer += 128;
                while (DestPtr < DestEnd) {
                    uint8_t Color = *buffer++;
                    if ((Color & 0xc0) == 0xc0) {
                        uint32_t RunLength = Color & 0x3f;
                        Color = *buffer++;

                        for (uint32_t Index = 0; Index < RunLength; Index++) {
                            *DestPtr++ = Palette[Color];
                        }
                    } else
                        *DestPtr++ = Palette[Color];
                }
            } else if (PCX->numPlanes == 3 && PCX->bitsPerPixel == 8) {
                info.type = EImageFormat::PCX;
                info.rgb_format = ERGBFormat::BGRA;
                info.bit_depth = 8;
                // Set texture properties.
                PixelsMemData = AllocPixels();
                info.width = PixelsMemData->pixels->width = NewU;
                info.height = PixelsMemData->pixels->height = NewV;
                PixelsMemData->pixels->texture_format = ETextureSourceFormat::BGRA8;
                PixelsMemData->pixels->bit_depth = 8;
                // Doing a memset to make sure the alpha channel is set to 0xff since we only have 3 color planes.
                PixelsMemData->data.resize(NewU * NewV * 4, 0xff);
                PixelsMemData->pixels->data = PixelsMemData->data.data();
                PixelsMemData->pixels->size = PixelsMemData->data.size();

                uint8_t* Dest = PixelsMemData->pixels->data;

                // Copy upside-down scanlines.
                buffer += 128;
                int CountU = std::min<int>(PCX->bytesPerLine, NewU);
                for (int i = 0; i < NewV; i++) {
                    // We need to decode image one line per time building RGB image color plane by color plane.
                    int RunLength, Overflow = 0;
                    uint8_t Color = 0;
                    for (int32_t ColorPlane = 2; ColorPlane >= 0; ColorPlane--) {
                        for (int32_t j = 0; j < CountU; j++) {
                            if (!Overflow) {
                                Color = *buffer++;
                                if ((Color & 0xc0) == 0xc0) {
                                    RunLength = std::min<int>((Color & 0x3f), CountU - j);
                                    Overflow = (Color & 0x3f) - RunLength;
                                    Color = *buffer++;
                                } else
                                    RunLength = 1;
                            } else {
                                RunLength = std::min<int>(Overflow, CountU - j);
                                Overflow = Overflow - RunLength;
                            }

                            // checkf(((i*NewU + RunLength) * 4 + ColorPlane) < (Texture->Source.CalcMipSize(0)),
                            //	TEXT("RLE going off the end of buffer"));
                            for (int32_t k = j; k < j + RunLength; k++) {
                                Dest[(i * NewU + k) * 4 + ColorPlane] = Color;
                            }
                            j += RunLength - 1;
                        }
                    }
                }
            } else {
                std::string error = "PCX uses an unsupported format (" + std::to_string(PCX->numPlanes) + "/" + std::to_string(PCX->bitsPerPixel) + ")";
                LogMessage(ELogLevel::Error, error.data());
                return false;
            }

            return true;
        }
    }

    //
    // TGA
    //
    if (imageFormat == EImageFormat::TGA) {
        // Support for alpha stored as pseudo-color 8-bit TGA
        const FTGAFileHeader* TGA = (FTGAFileHeader*)buffer;
        if (length >= sizeof(FTGAFileHeader) && ((TGA->colorMapType == 0 && TGA->imageTypeCode == 2) ||
                                                 // ImageTypeCode 3 is greyscale
                                                 (TGA->colorMapType == 0 && TGA->imageTypeCode == 3) || (TGA->colorMapType == 0 && TGA->imageTypeCode == 10) || (TGA->colorMapType == 1 && TGA->imageTypeCode == 1 && TGA->bitsPerPixel == 8))) {
            const bool bResult = DecompressTGA(TGA, PixelsMemData);
            if (!bResult) {
                decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
                PixelsMemData = nullptr;
            } else {
                info.type = EImageFormat::TGA;
                info.rgb_format = PixelsMemData->pixels->texture_format == ETextureSourceFormat::BGRA8 ? ERGBFormat::BGRA : ERGBFormat::Gray;
                info.bit_depth = PixelsMemData->pixels->bit_depth;
                info.width = PixelsMemData->pixels->width;
                info.height = PixelsMemData->pixels->height;
            }
            return bResult;
        }
    }
    return false;
}

void __cdecl SetLogFunction(LogFunc func) { SetLogMessageFunc(func); }

bool __cdecl CreatePixelDataFromFile(EImageFormat image_format, const char* file_name, ImageInfo& info, ImagePixelData*& pixel_data) {
    std::vector<uint8_t> dataBinary;
    std::ifstream fileStream(file_name, std::ios_base::in | std::ios::binary);
    if (fileStream) {
        fileStream.unsetf(std::ios::skipws);
        fileStream.seekg(0, std::ios::end);
        dataBinary.resize(static_cast<size_t>(fileStream.tellg()));
        fileStream.seekg(0, std::ios::beg);
        fileStream.read(reinterpret_cast<char*>(dataBinary.data()), dataBinary.size());
        dataBinary.resize(static_cast<size_t>(fileStream.gcount()));
        return CreatePixelData(image_format, dataBinary.data(), dataBinary.size(), info, pixel_data);
    } else {
        return false;
    }
}

bool __cdecl CreatePixelData(EImageFormat imageFormat, const uint8_t* buffer, uint64_t length, ImageInfo& info, ImagePixelData*& pixel_data) {
    std::shared_ptr<ImagePixelsMemData> PixelsMemData;
    bool result = DecodeImage(imageFormat, buffer, length, info, PixelsMemData);
    if (result && PixelsMemData) {
        pixel_data = PixelsMemData->pixels.get();
        return true;
    } else if (PixelsMemData) {
        decoded_image_mmem_data_pool.erase(PixelsMemData->pixels.get());
        PixelsMemData = nullptr;
    }
    pixel_data = nullptr;
    return false;
}

void __cdecl ReleasePixelData(ImagePixelData*& pixel_data) {
    if (!pixel_data) {
        return;
    }
    if (decoded_image_mmem_data_pool.find(pixel_data) != decoded_image_mmem_data_pool.end()) {
        decoded_image_mmem_data_pool.erase(pixel_data);
        pixel_data = nullptr;
    }
}

static const uint8_t IMAGE_MAGIC_PNG[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const uint8_t IMAGE_MAGIC_JPEG[] = {0xFF, 0xD8, 0xFF};
static const uint8_t IMAGE_MAGIC_BMP[] = {0x42, 0x4D};
static const uint8_t IMAGE_MAGIC_ICO[] = {0x00, 0x00, 0x01, 0x00};
static const uint8_t IMAGE_MAGIC_EXR[] = {0x76, 0x2F, 0x31, 0x01};
static const uint8_t IMAGE_MAGIC_ICNS[] = {0x69, 0x63, 0x6E, 0x73};

/** Internal helper function to verify image signature. */
template <int magicCount>
bool StartsWith(const uint8_t* content, int64_t contentSize, const uint8_t (&Magic)[magicCount]) {
    if (contentSize < magicCount) {
        return false;
    }

    for (int i = 0; i < magicCount; ++i) {
        if (content[i] != Magic[i]) {
            return false;
        }
    }

    return true;
}

EImageFormat __cdecl DetectFormat(const void* compressedData, int64_t compressedSize) {
    EImageFormat Format = EImageFormat::Invalid;
    if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_PNG)) {
        Format = EImageFormat::PNG;
    } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_JPEG)) {
        Format = EImageFormat::JPEG;  // @Todo: Should we detect grayscale vs non-grayscale?
    } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_BMP)) {
        Format = EImageFormat::BMP;
    } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_ICO)) {
        Format = EImageFormat::ICO;
    } else if (StartsWith((uint8_t*)compressedData, compressedSize, IMAGE_MAGIC_EXR)) {
        Format = EImageFormat::EXR;
    }

    return Format;
}
}  // namespace ImageDecoder
