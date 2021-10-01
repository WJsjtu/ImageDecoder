#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "IImageWrapper.h"

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
    const std::vector<uint8_t>& GetRawData() const { return RawData; }

    /**
     * Moves the image's raw data into the provided array.
     *
     * @param OutRawData The destination array.
     */
    void MoveRawData(std::vector<uint8_t>& OutRawData) { OutRawData = std::move(RawData); }

public:
    /**
     * Compresses the data.
     *
     * @param Quality The compression quality.
     */
    virtual void Compress(int Quality) = 0;

    /**
     * Resets the local variables.
     */
    virtual void Reset();

    /**
     * Sets last error message.
     *
     * @param ErrorMessage The error message to set.
     */
    virtual void SetError(const char* ErrorMessage);

    /**
     * Function to uncompress our data
     *
     * @param InFormat How we want to manipulate the RGB data
     */
    virtual void Uncompress(const ERGBFormat InFormat, int InBitDepth) = 0;

public:
    //~ IImageWrapper interface

    virtual const std::vector<uint8_t>& GetCompressed(int Quality = 0) override;

    virtual int GetBitDepth() const override { return BitDepth; }

    virtual ERGBFormat GetFormat() const override { return Format; }

    virtual int GetHeight() const override { return Height; }

    virtual bool GetRaw(const ERGBFormat InFormat, int InBitDepth, std::vector<uint8_t>& OutRawData) override;

    virtual int GetWidth() const override { return Width; }

    virtual int GetNumFrames() const override { return NumFrames; }

    virtual int GetFramerate() const override { return Framerate; }

    virtual bool SetCompressed(const void* InCompressedData, int64_t InCompressedSize) override;
    virtual bool SetRaw(const void* InRawData, int64_t InRawSize, const int InWidth, const int InHeight, const ERGBFormat InFormat, const int InBitDepth) override;
    virtual bool SetAnimationInfo(int InNumFrames, int InFramerate) override;

protected:
    /** Arrays of compressed/raw data */
    std::vector<uint8_t> RawData;
    std::vector<uint8_t> CompressedData;

    /** Format of the raw data */
    ERGBFormat RawFormat;
    uint8_t RawBitDepth;

    /** Format of the image */
    ERGBFormat Format;

    /** Bit depth of the image */
    uint8_t BitDepth;

    /** Width/Height of the image data */
    int Width;
    int Height;

    /** Animation information */
    int NumFrames;
    int Framerate;

    /** Last Error Message. */
    std::string LastError;
};
