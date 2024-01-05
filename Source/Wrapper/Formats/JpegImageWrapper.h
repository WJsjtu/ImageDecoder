#pragma once
#include "Wrapper/ImageWrapperBase.h"

namespace ImageDecoder {
using tjhandle = void*;

/**
 * Uncompresses JPEG data to raw 24bit RGB image that can be used by Unreal textures.
 *
 * Source code for JPEG decompression from http://code.google.com/p/jpeg-compressor/
 */
class FJpegImageWrapper : public FImageWrapperBase {
public:
    /** Default constructor. */
    FJpegImageWrapper(int inNumComponents = 4);

    virtual ~FJpegImageWrapper();

public:
    //~ FImageWrapperBase interface

    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;
    virtual bool SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) override;
    virtual void Uncompress(const ERGBFormat inFormat, int inBitDepth) override;
    virtual void Compress(int quality) override;

    bool SetCompressedTurbo(const void* inCompressedData, int64_t inCompressedSize);
    void CompressTurbo(int quality);
    void UncompressTurbo(const ERGBFormat inFormat, int inBitDepth);

private:
    int numComponents;

    tjhandle compressor;
    tjhandle decompressor;
};
}  // namespace ImageDecoder
