#include "Decoder.h"
#include <algorithm>
#include <string>
namespace ImageDecoder {
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

bool DecompressTGA_helper(const FTGAFileHeader* TGA, uint32_t*& textureData, const int textureDataSize, std::string& warn) {
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
            warn = "TGA uses an unsupported rle-compressed bit-depth: " + std::to_string(TGA->bitsPerPixel);
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
            warn = "TGA uses an unsupported bit-depth: " + std::to_string(TGA->bitsPerPixel);
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
        warn = "TGA is an unsupported type: " + std::to_string(TGA->imageTypeCode);
        return false;
    }

    // Flip the image data if the flip bits are set in the TGA header.
    bool flipX = (TGA->imageDescriptor & 0x10) ? 1 : 0;
    bool flipY = (TGA->imageDescriptor & 0x20) ? 1 : 0;
    if (flipY || flipX) {
        Vector<uint8_t> FlippedData;
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

bool DecompressTGA(const FTGAFileHeader* TGA, ImportImage& outImage, std::string& warn) {
    if (TGA->colorMapType == 1 && TGA->imageTypeCode == 1 && TGA->bitsPerPixel == 8) {
        // Notes: The Scaleform GFx exporter (dll) strips all font glyphs into a single 8-bit texture.
        // The targa format uses this for a palette index; GFx uses a palette of (i,i,i,i) so the index
        // is also the alpha value.
        //
        // We store the image as PF_G8, where it will be used as alpha in the Glyph shader.
        outImage.width = TGA->width;
        outImage.height = TGA->height;
        outImage.textureformat = ETextureSourceFormat::G8;
        outImage.bitDepth = 8;
        outImage.data.resize(outImage.width * outImage.height * 1);
    } else if (TGA->colorMapType == 0 && TGA->imageTypeCode == 3 && TGA->bitsPerPixel == 8) {
        // standard grayscale images
        outImage.width = TGA->width;
        outImage.height = TGA->height;
        outImage.textureformat = ETextureSourceFormat::G8;
        outImage.bitDepth = 8;
        outImage.data.resize(outImage.width * outImage.height * 1);
    } else {
        if (TGA->imageTypeCode == 10)  // 10 = RLE compressed
        {
            if (TGA->bitsPerPixel != 32 && TGA->bitsPerPixel != 24 && TGA->bitsPerPixel != 16) {
                warn = "TGA uses an unsupported rle-compressed bit-depth: " + std::to_string(TGA->bitsPerPixel);
                return false;
            }
        } else {
            if (TGA->bitsPerPixel != 32 && TGA->bitsPerPixel != 16 && TGA->bitsPerPixel != 24) {
                warn = "TGA uses an unsupported bit-depth: " + std::to_string(TGA->bitsPerPixel);
                return false;
            }
        }

        outImage.width = TGA->width;
        outImage.height = TGA->height;
        outImage.textureformat = ETextureSourceFormat::BGRA8;
        outImage.bitDepth = 8;
        outImage.data.resize(outImage.width * outImage.height * 4);
    }

    size_t TextureDataSize = outImage.data.size();
    uint32_t* TextureData = (uint32_t*)outImage.data.data();

    return DecompressTGA_helper(TGA, TextureData, static_cast<int>(TextureDataSize), warn);
}

bool DecodeImage(EImageFormat imageFormat, const uint8_t* buffer, uint32_t length, bool bFillPNGZeroAlpha, std::string& warn, ImportImage& outImage) {
    IImageWrapperModule& ImageWrapperModule = GetImageWrapperModule();
    //
    // PNG
    //
    if (imageFormat == EImageFormat::PNG) {
        std::shared_ptr<IImageWrapper> pngImageWrapper(ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG));
        if (pngImageWrapper && pngImageWrapper->SetCompressed(buffer, length)) {
            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = pngImageWrapper->GetBitDepth();
            ERGBFormat format = pngImageWrapper->GetFormat();
            outImage.source.type = EImageFormat::PNG;
            outImage.source.RGBFormat = format;
            outImage.source.bitDepth = bitDepth;

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
                warn = "PNG file contains data in an unsupported format.";
                return false;
            }

            outImage.width = pngImageWrapper->GetWidth();
            outImage.height = pngImageWrapper->GetHeight();
            outImage.textureformat = textureFormat;
            outImage.bitDepth = bitDepth;

            if (pngImageWrapper->GetRaw(format, bitDepth, outImage.data)) {
                if (bFillPNGZeroAlpha) {
                    // Replace the pixels with 0.0 alpha with a color value from the nearest neighboring color which has a non-zero alpha
                    // FillZeroAlphaPNGData(outImage.width, outImage.height, outImage.format, outImage.data.data());
                }
            } else {
                warn = "Failed to decode PNG.";
                return false;
            }

            return true;
        }
    }

    //
    // JPEG
    //
    if (imageFormat == EImageFormat::JPEG) {
        std::shared_ptr<IImageWrapper> jpegImageWrapper(ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG));
        if (jpegImageWrapper && jpegImageWrapper->SetCompressed(buffer, length)) {
            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = jpegImageWrapper->GetBitDepth();
            ERGBFormat format = jpegImageWrapper->GetFormat();
            outImage.source.type = EImageFormat::JPEG;
            outImage.source.RGBFormat = format;
            outImage.source.bitDepth = bitDepth;

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
                warn = "JPEG file contains data in an unsupported format.";
                return false;
            }

            outImage.width = jpegImageWrapper->GetWidth();
            outImage.height = jpegImageWrapper->GetHeight();
            outImage.textureformat = textureFormat;
            outImage.bitDepth = bitDepth;

            if (!jpegImageWrapper->GetRaw(format, bitDepth, outImage.data)) {
                warn = "Failed to decode JPEG.";
                return false;
            }

            return true;
        }
    }

    //
    // EXR
    //
    if (imageFormat == EImageFormat::EXR) {
        std::shared_ptr<IImageWrapper> exrImageWrapper(ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR));
        if (exrImageWrapper && exrImageWrapper->SetCompressed(buffer, length)) {
            int width = exrImageWrapper->GetWidth();
            int height = exrImageWrapper->GetHeight();

            // Select the texture's source format
            ETextureSourceFormat textureFormat = ETextureSourceFormat::Invalid;
            int bitDepth = exrImageWrapper->GetBitDepth();
            ERGBFormat format = exrImageWrapper->GetFormat();
            outImage.source.type = EImageFormat::EXR;
            outImage.source.RGBFormat = format;
            outImage.source.bitDepth = bitDepth;

            if (format == ERGBFormat::RGBA && bitDepth == 16) {
                textureFormat = ETextureSourceFormat::RGBA16F;
                format = ERGBFormat::RGBA;
            }

            if (textureFormat == ETextureSourceFormat::Invalid) {
                warn = "EXR file contains data in an unsupported format.";
                return false;
            }

            outImage.width = width;
            outImage.height = height;
            outImage.textureformat = textureFormat;
            outImage.bitDepth = bitDepth;

            if (!exrImageWrapper->GetRaw(format, bitDepth, outImage.data)) {
                warn = "Failed to decode EXR.";
                return false;
            }

            return true;
        }
    }

    //
    // BMP
    //
    if (imageFormat == EImageFormat::BMP) {
        std::shared_ptr<IImageWrapper> bmpImageWrapper(ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP));
        if (bmpImageWrapper && bmpImageWrapper->SetCompressed(buffer, length)) {
            int bitDepth = bmpImageWrapper->GetBitDepth();
            ERGBFormat format = bmpImageWrapper->GetFormat();
            outImage.source.type = EImageFormat::BMP;
            outImage.source.RGBFormat = format;
            outImage.source.bitDepth = bitDepth;
            outImage.width = bmpImageWrapper->GetWidth();
            outImage.height = bmpImageWrapper->GetHeight();
            outImage.textureformat = ETextureSourceFormat::BGRA8;
            outImage.bitDepth = bmpImageWrapper->GetBitDepth();
            return bmpImageWrapper->GetRaw(bmpImageWrapper->GetFormat(), bmpImageWrapper->GetBitDepth(), outImage.data);
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
                outImage.source.type = EImageFormat::PCX;
                outImage.source.RGBFormat = ERGBFormat::BGRA;
                outImage.source.bitDepth = 8;
                // Set texture properties.
                outImage.width = NewU;
                outImage.height = NewV;
                outImage.textureformat = ETextureSourceFormat::BGRA8;
                outImage.bitDepth = 8;
                outImage.data.resize(outImage.width * outImage.height * 4);
                FColor* DestPtr = (FColor*)outImage.data.data();

                // Import the palette.
                uint8_t* PCXPalette = (uint8_t*)(buffer + length - 256 * 3);
                Vector<FColor> Palette;
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
                outImage.source.type = EImageFormat::PCX;
                outImage.source.RGBFormat = ERGBFormat::BGRA;
                outImage.source.bitDepth = 8;
                // Set texture properties.
                outImage.width = NewU;
                outImage.height = NewV;
                outImage.textureformat = ETextureSourceFormat::BGRA8;
                outImage.bitDepth = 8;
                outImage.data.resize(outImage.width * outImage.height * 4);

                uint8_t* Dest = outImage.data.data();

                // Doing a memset to make sure the alpha channel is set to 0xff since we only have 3 color planes.
                memset(Dest, 0xff, NewU * NewV * 4);

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
                warn = "PCX uses an unsupported format (" + std::to_string(PCX->numPlanes) + "/" + std::to_string(PCX->bitsPerPixel) + ")";
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
            const bool bResult = DecompressTGA(TGA, outImage, warn);
            outImage.source.type = EImageFormat::TGA;
            outImage.source.RGBFormat = outImage.textureformat == ETextureSourceFormat::BGRA8 ? ERGBFormat::BGRA : ERGBFormat::Gray;
            outImage.source.bitDepth = outImage.bitDepth;
            return bResult;
        }
    }
    return false;
}

bool DecodeImage(EImageFormat imageFormat, const uint8_t* buffer, uint32_t length, bool bFillPNGZeroAlpha, Vector<char>& warn, ImportImage& outImage) {
    std::string warnStr;
    bool result = DecodeImage(imageFormat, buffer, length, bFillPNGZeroAlpha, warnStr, outImage);
    warn.clear();
    for (auto chr : warnStr) {
        warn.push_back(chr);
    }
    return result;
}
}  // namespace ImageDecoder