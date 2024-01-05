#pragma once

#include "Wrapper/ImageWrapperBase.h"

namespace ImageDecoder {
/**
 * BMP implementation of the helper class
 */
class FBmpImageWrapper : public FImageWrapperBase {
public:
    /** Default Constructor. */
    FBmpImageWrapper(bool bInHasHeader = true, bool bInHalfHeight = false);

public:
    /** Helper function used to uncompress BMP data from a buffer */
    void UncompressBMPData(const ERGBFormat inFormat, const int inBitDepth);

    /**
     * Load the header information, returns true if successful.
     *
     * @return true if successful
     */
    bool LoadBMPHeader();

    /**
     * Load the sub-header information, returns true if successful.
     *
     * @return true if successful
     */
    bool LoadBMPInfoHeader();

public:
    //~ FImageWrapper interface

    virtual void Compress(int quality) override;
    virtual void Uncompress(const ERGBFormat inFormat, int inBitDepth) override;
    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;

private:
    /** Whether this file has a BMP file header */
    bool bHasHeader;

    /** BMP as a sub-format of ICO stores its height as half their actual size */
    bool bHalfHeight;
};
}  // namespace ImageDecoder
