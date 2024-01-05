#pragma once
#include "Wrapper/ImageWrapperBase.h"

namespace ImageDecoder {
/**
 * ICO implementation of the helper class.
 */
class FIcoImageWrapper : public FImageWrapperBase {
public:
    /** Default Constructor. */
    FIcoImageWrapper();

public:
    //~ FImageWrapper Interface

    virtual void Compress(int quality) override;
    virtual void Uncompress(const ERGBFormat inFormat, int inBitDepth) override;
    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;
    virtual bool GetRaw(const ERGBFormat inFormat, int inBitDepth, std::vector<uint8_t>& outRawData) override;

protected:
    /**
     * Load the header information.
     *
     * @return true if successful
     */
    bool LoadICOHeader();

private:
    /** Sub-wrapper component, as icons that contain PNG or BMP data */
    std::shared_ptr<FImageWrapperBase> subImageWrapper;

    /** Offset into file that we use as image data */
    uint32_t imageOffset;

    /** Size of image data in file */
    uint32_t imageSize;

    /** Whether we should use PNG or BMP data */
    bool bIsPng;
};
}  // namespace ImageDecoder
