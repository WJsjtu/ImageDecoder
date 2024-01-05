#pragma once
#include <cstdint>
#include <vector>
#include "Imath/ImathBox.h"
#include "OpenEXR/ImfArray.h"
#include "OpenEXR/ImfChannelList.h"
#include "OpenEXR/ImfHeader.h"
#include "OpenEXR/ImfIO.h"
#include "OpenEXR/ImfInputFile.h"
#include "OpenEXR/ImfOutputFile.h"
#include "OpenEXR/ImfRgbaFile.h"
#include "OpenEXR/ImfStdIO.h"
#include "Wrapper/ImageWrapperBase.h"

namespace ImageDecoder {
/**
 * OpenEXR implementation of the helper class
 */
class FExrImageWrapper : public FImageWrapperBase {
public:
    /**
     * Default Constructor.
     */
    FExrImageWrapper();

public:
    //~ FImageWrapper interface

    virtual void Compress(int quality) override;
    virtual void Uncompress(const ERGBFormat inFormat, int inBitDepth) override;
    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) override;

protected:
    template <Imf::PixelType OutputFormat, typename sourcetype>
    void WriteFrameBufferChannel(Imf::FrameBuffer& imfFrameBuffer, const char* channelName, const sourcetype* srcData, std::vector<uint8_t>& channelBuffer);

    template <Imf::PixelType OutputFormat, typename sourcetype>
    void CompressRaw(const sourcetype* srcData, bool bIgnoreAlpha);

    const char* GetRawChannelName(int channelIndex) const;

private:
    bool bUseCompression;
};
}  // namespace ImageDecoder
