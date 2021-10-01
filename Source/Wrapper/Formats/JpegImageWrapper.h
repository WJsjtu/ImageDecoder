#pragma once
#include "Wrapper/ImageWrapperBase.h"

using tjhandle = void*;

/**
 * Uncompresses JPEG data to raw 24bit RGB image that can be used by Unreal textures.
 *
 * Source code for JPEG decompression from http://code.google.com/p/jpeg-compressor/
 */
class FJpegImageWrapper : public FImageWrapperBase {
public:
    /** Default constructor. */
    FJpegImageWrapper(int InNumComponents = 4);

    virtual ~FJpegImageWrapper();

public:
    //~ FImageWrapperBase interface

    virtual bool SetCompressed(const void* InCompressedData, int64_t InCompressedSize) override;
    virtual bool SetRaw(const void* InRawData, int64_t InRawSize, const int InWidth, const int InHeight, const ERGBFormat InFormat, const int InBitDepth) override;
    virtual void Uncompress(const ERGBFormat InFormat, int InBitDepth) override;
    virtual void Compress(int Quality) override;

    bool SetCompressedTurbo(const void* InCompressedData, int64_t InCompressedSize);
    void CompressTurbo(int Quality);
    void UncompressTurbo(const ERGBFormat InFormat, int InBitDepth);

private:
    int NumComponents;

    tjhandle Compressor;
    tjhandle Decompressor;
};
