#pragma once
#include "Wrapper/ImageWrapperBase.h"
#include "Imath/ImathBox.h"
#include "OpenEXR/ImfArray.h"
#include "OpenEXR/ImfChannelList.h"
#include "OpenEXR/ImfHeader.h"
#include "OpenEXR/ImfIO.h"
#include "OpenEXR/ImfInputFile.h"
#include "OpenEXR/ImfOutputFile.h"
#include "OpenEXR/ImfRgbaFile.h"
#include "OpenEXR/ImfStdIO.h"
#include <cstdint>
#include <vector>

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

    virtual void Compress(int Quality) override;
    virtual void Uncompress(const ERGBFormat InFormat, int InBitDepth) override;
    virtual bool SetCompressed(const void* InCompressedData, int64_t InCompressedSize) override;

protected:
    template <Imf::PixelType OutputFormat, typename sourcetype>
    void WriteFrameBufferChannel(Imf::FrameBuffer& ImfFrameBuffer, const char* ChannelName, const sourcetype* SrcData, std::vector<uint8_t>& ChannelBuffer);

    template <Imf::PixelType OutputFormat, typename sourcetype>
    void CompressRaw(const sourcetype* SrcData, bool bIgnoreAlpha);

    const char* GetRawChannelName(int ChannelIndex) const;

private:
    bool bUseCompression;
};
