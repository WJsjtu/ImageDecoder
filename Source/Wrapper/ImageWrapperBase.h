#pragma once

#include <iostream>
#include <string>
#include "Decoder.h"

namespace ImageDecoder {
/**
 * The abstract helper class for handling the different image formats
 */
class FImageWrapperBase : public IImageWrapper {
public:
    /** Default Constructor. */
    FImageWrapperBase();

public:
    /**
     * Gets the image's raw data.
     *
     * @return A read-only byte array containing the data.
     */
    const Vector<uint8_t>& GetRawData() const { return rawData; }

    /**
     * Moves the image's raw data into the provided array.
     *
     * @param OutRawData The destination array.
     */
    void MoveRawData(Vector<uint8_t>& outRawData) { outRawData = std::move(rawData); }

public:
    /**
     * Compresses the data.
     *
     * @param Quality The compression quality.
     */
    virtual void Compress(int quality) = 0;

    /**
     * Resets the local variables.
     */
    virtual void Reset();

    /**
     * Sets last error message.
     *
     * @param ErrorMessage The error message to set.
     */
    virtual void SetError(const char* errorMessage);

    /**
     * Function to uncompress our data
     *
     * @param InFormat How we want to manipulate the RGB data
     */
    virtual void Uncompress(const ERGBFormat inFormat, int inBitDepth) = 0;

public:
    //~ IImageWrapper interface

    virtual const Vector<uint8_t>& GetCompressed(int quality = 0) override;

    virtual int GetBitDepth() const override { return bitDepth; }

    virtual ERGBFormat GetFormat() const override { return format; }

    virtual int GetHeight() const override { return height; }

    virtual bool GetRaw(const ERGBFormat inFormat, int inBitDepth, Vector<uint8_t>& outRawData) override;

    virtual int GetWidth() const override { return width; }

    virtual int GetNumFrames() const override { return numFrames; }

    virtual int GetFramerate() const override { return framerate; }

    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;
    virtual bool SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) override;
    virtual bool SetAnimationInfo(int inNumFrames, int inFramerate) override;

protected:
    /** Arrays of compressed/raw data */
    Vector<uint8_t> rawData;
    Vector<uint8_t> compressedData;

    /** Format of the raw data */
    ERGBFormat rawFormat;
    uint8_t rawBitDepth;

    /** Format of the image */
    ERGBFormat format;

    /** Bit depth of the image */
    uint32_t bitDepth;

    /** Width/Height of the image data */
    int width;
    int height;

    /** Animation information */
    int numFrames;
    int framerate;

    /** Last Error Message. */
    std::string lastError;
};
}  // namespace ImageDecoder