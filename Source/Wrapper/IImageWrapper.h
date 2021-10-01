#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include "Export.h"

/**
 * Enumerates the types of image formats this class can handle.
 */
enum class IMAGE_PORT EImageFormat : int8_t {
    /** Invalid or unrecognized format. */
    Invalid = -1,

    /** Portable Network Graphics. */
    PNG = 0,

    /** Joint Photographic Experts Group. */
    JPEG,

    /** Single channel JPEG. */
    GrayscaleJPEG,

    /** Windows Bitmap. */
    BMP,

    /** Windows Icon resource. */
    ICO,

    /** OpenEXR (HDR) image file format. */
    EXR,
    PCX,
    TGA
};

/**
 * Enumerates the types of RGB formats this class can handle.
 */
enum class IMAGE_PORT ERGBFormat : int8_t {
    Invalid = -1,
    RGBA = 0,
    BGRA = 1,
    Gray = 2,
};

/**
 * Enumerates available image compression qualities.
 */
enum class IMAGE_PORT EImageCompressionQuality : int8_t {
    Default = 0,
    Uncompressed = 1,
};

/**
 * Interface for image wrappers.
 */
class IMAGE_PORT IImageWrapper {
public:
    /**
     * Sets the compressed data.
     *
     * @param InCompressedData The memory address of the start of the compressed data.
     * @param InCompressedSize The size of the compressed data parsed.
     * @return true if data was the expected format.
     */
    virtual bool SetCompressed(const void* InCompressedData, int64_t InCompressedSize) = 0;

    /**
     * Sets the compressed data.
     *
     * @param InRawData The memory address of the start of the raw data.
     * @param InRawSize The size of the compressed data parsed.
     * @param InWidth The width of the image data.
     * @param InHeight the height of the image data.
     * @param InFormat the format the raw data is in, normally RGBA.
     * @param InBitDepth the bit-depth per channel, normally 8.
     * @return true if data was the expected format.
     */
    virtual bool SetRaw(const void* InRawData, int64_t InRawSize, const int InWidth, const int InHeight, const ERGBFormat InFormat, const int InBitDepth) = 0;

    /**
     * Set information for animated formats
     * @param InNumFrames The number of frames in the animation (the RawData from SetRaw will need to be a multiple of NumFrames)
     * @param InFramerate The playback rate of the animation
     * @return true if successful
     */
    virtual bool SetAnimationInfo(int InNumFrames, int InFramerate) = 0;

    /**
     * Gets the compressed data.
     *
     * @return Array of the compressed data.
     */
    virtual const std::vector<uint8_t>& GetCompressed(int Quality = 0) = 0;

    /**
     * Gets the raw data in a TArray. Only use this if you're certain that the image is less than 2 GB in size.
     * Prefer using the overload which takes a TArray64 in general.
     *
     * @param InFormat How we want to manipulate the RGB data.
     * @param InBitDepth The output bit-depth per channel, normally 8.
     * @param OutRawData Will contain the uncompressed raw data.
     * @return true on success, false otherwise.
     */
    virtual bool GetRaw(const ERGBFormat InFormat, int InBitDepth, std::vector<uint8_t>& OutRawData) {
        std::vector<uint8_t> TmpRawData;
        if (GetRaw(InFormat, InBitDepth, TmpRawData)) {
            if (TmpRawData.size() != TmpRawData.size()) {
                std::cerr << "Tried to get " << GetWidth() << "x" << GetHeight() << " " << InBitDepth << "bpp image with format " << static_cast<int8_t>(InFormat) << " into 32-bit TArray (%" << TmpRawData.size() << " bytes)" << std::endl;
                return false;
            }
            // todo check
            std::swap(OutRawData, TmpRawData);
            return true;
        } else {
            return false;
        }
    }

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

/** Type definition for shared pointers to instances of IImageWrapper. */
typedef std::shared_ptr<IImageWrapper> IImageWrapperPtr;