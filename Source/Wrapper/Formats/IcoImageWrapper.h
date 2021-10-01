#pragma once
#include "Wrapper/ImageWrapperBase.h"

/**
 * ICO implementation of the helper class.
 */
class FIcoImageWrapper : public FImageWrapperBase {
public:
    /** Default Constructor. */
    FIcoImageWrapper();

public:
    //~ FImageWrapper Interface

    virtual void Compress(int Quality) override;
    virtual void Uncompress(const ERGBFormat InFormat, int InBitDepth) override;
    virtual bool SetCompressed(const void* InCompressedData, int64_t InCompressedSize) override;
    virtual bool GetRaw(const ERGBFormat InFormat, int InBitDepth, std::vector<uint8_t>& OutRawData) override;

protected:
    /**
     * Load the header information.
     *
     * @return true if successful
     */
    bool LoadICOHeader();

private:
    /** Sub-wrapper component, as icons that contain PNG or BMP data */
    std::shared_ptr<FImageWrapperBase> SubImageWrapper;

    /** Offset into file that we use as image data */
    uint32_t ImageOffset;

    /** Size of image data in file */
    uint32_t ImageSize;

    /** Whether we should use PNG or BMP data */
    bool bIsPng;
};
