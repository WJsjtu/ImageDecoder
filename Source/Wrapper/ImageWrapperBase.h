#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "Decoder.h"

namespace ImageDecoder {

/**
 * Interface for image wrappers.
 */
class IImageWrapper {
public:
    /**
     * Sets the compressed data.
     *
     * @param inCompressedData The memory address of the start of the compressed data.
     * @param inCompressedSize The size of the compressed data parsed.
     * @return true if data was the expected format.
     */
    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) = 0;

    /**
     * Sets the compressed data.
     *
     * @param inRawData The memory address of the start of the raw data.
     * @param inRawSize The size of the compressed data parsed.
     * @param inWidth The width of the image data.
     * @param inHeight the height of the image data.
     * @param inFormat the format the raw data is in, normally RGBA.
     * @param inBitDepth the bit-depth per channel, normally 8.
     * @return true if data was the expected format.
     */
    virtual bool SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) = 0;

    /**
     * Set information for animated formats
     * @param inNumFrames The number of frames in the animation (the RawData from SetRaw will need to be a multiple of NumFrames)
     * @param inFramerate The playback rate of the animation
     * @return true if successful
     */
    virtual bool SetAnimationInfo(int inNumFrames, int inFramerate) = 0;

    /**
     * Gets the compressed data.
     *
     * @return Array of the compressed data.
     */
    virtual const std::vector<uint8_t>& GetCompressed(int quality = 0) = 0;

    /**
     * Gets the raw data in a TArray. Only use this if you're certain that the image is less than 2 GB in size.
     * Prefer using the overload which takes a TArray64 in general.
     *
     * @param inFormat How we want to manipulate the RGB data.
     * @param inBitDepth The output bit-depth per channel, normally 8.
     * @param outRawData Will contain the uncompressed raw data.
     * @return true on success, false otherwise.
     */
    virtual bool GetRaw(const ERGBFormat inFormat, int inBitDepth, std::vector<uint8_t>& outRawData);

    /**
     * Gets the width of the image.
     *
     * @return Image width.
     * @see GetHeight
     */
    virtual int GetWidth() const = 0;

    /**
     * Gets the height of the image.
     *
     * @return Image height.
     * @see GetWidth
     */
    virtual int GetHeight() const = 0;

    /**
     * Gets the bit depth of the image.
     *
     * @return The bit depth per-channel of the image.
     */
    virtual int GetBitDepth() const = 0;

    /**
     * Gets the format of the image.
     * Theoretically, this is the format it would be best to call GetRaw() with, if you support it.
     *
     * @return The format the image data is in
     */
    virtual ERGBFormat GetFormat() const = 0;

    /**
     * @return The number of frames in an animated image
     */
    virtual int GetNumFrames() const = 0;

    /**
     * @return The playback framerate of animated images (or 0 for non-animated)
     */
    virtual int GetFramerate() const = 0;

public:
    /** Virtual destructor. */
    virtual ~IImageWrapper() {}
};

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
    const std::vector<uint8_t>& GetRawData() const { return rawData; }

    /**
     * Moves the image's raw data into the provided array.
     *
     * @param OutRawData The destination array.
     */
    void MoveRawData(std::vector<uint8_t>& outRawData) { outRawData = std::move(rawData); }

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

    virtual const std::vector<uint8_t>& GetCompressed(int quality = 0) override;

    virtual int GetBitDepth() const override { return bitDepth; }

    virtual ERGBFormat GetFormat() const override { return format; }

    virtual int GetHeight() const override { return height; }

    virtual bool GetRaw(const ERGBFormat inFormat, int inBitDepth, std::vector<uint8_t>& outRawData) override;

    virtual int GetWidth() const override { return width; }

    virtual int GetNumFrames() const override { return numFrames; }

    virtual int GetFramerate() const override { return framerate; }

    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;
    virtual bool SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) override;
    virtual bool SetAnimationInfo(int inNumFrames, int inFramerate) override;

protected:
    /** Arrays of compressed/raw data */
    std::vector<uint8_t> rawData;
    std::vector<uint8_t> compressedData;

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
