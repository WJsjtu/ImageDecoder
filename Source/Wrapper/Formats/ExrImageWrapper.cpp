#include "ExrImageWrapper.h"
#include "Utils/Utils.h"
#include <stdio.h>
#include <algorithm>
#include <cmath>

namespace ImageDecoder {
typedef half Float16;

FExrImageWrapper::FExrImageWrapper() : FImageWrapperBase(), bUseCompression(true) {}

template <typename sourcetype>
class FSourceImageRaw {
public:
    FSourceImageRaw(const std::vector<uint8_t>& inSourceImageBitmap, uint64_t inChannels, uint64_t inWidth, uint64_t inHeight) : sourceImageBitmap(inSourceImageBitmap), width(inWidth), height(inHeight), channels(inChannels) {
        check(sourceImageBitmap.size() == channels * width * height * sizeof(sourcetype));
    }

    uint64_t GetXStride() const { return sizeof(sourcetype) * channels; }
    uint64_t GetYStride() const { return GetXStride() * width; }

    const sourcetype* GetHorzLine(uint64_t y, uint64_t Channel) { return &sourceImageBitmap[(sizeof(sourcetype) * Channel) + (GetYStride() * y)]; }

private:
    const std::vector<uint8_t>& sourceImageBitmap;
    uint64_t width;
    uint64_t height;
    uint64_t channels;
};

class FMemFileOut : public Imf::OStream {
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    FMemFileOut(const char fileName[]) : Imf::OStream(fileName), pos(0) {}

    // InN must be 32bit to match the abstract interface.
    virtual void write(const char c[/*n*/], int inN) {
        uint64_t srcN = (uint64_t)inN;
        uint64_t destPost = pos + srcN;
        if (destPost > data.size()) {
            data.resize(data.size() + std::max((uint64_t)data.size() * 2, destPost) - data.size());
        }

        for (uint64_t i = 0; i < srcN; ++i) {
            data[pos + i] = c[i];
        }
        pos += srcN;
    }

    //---------------------------------------------------------
    // Get the current writing position, in bytes from the
    // beginning of the file.  If the next call to write() will
    // start writing at the beginning of the file, tellp()
    // returns 0.
    //---------------------------------------------------------

    virtual uint64_t tellp() { return pos; }

    //-------------------------------------------
    // Set the current writing position.
    // After calling seekp(i), tellp() returns i.
    //-------------------------------------------

    virtual void seekp(uint64_t pos) { pos = pos; }

    uint64_t pos;
    std::vector<uint8_t> data;
};

class FMemFileIn : public Imf::IStream {
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    FMemFileIn(const void* inData, int64_t inSize) : Imf::IStream(""), data((const char*)inData), size(inSize), pos(0) {}

    //------------------------------------------------------
    // Read from the stream:
    //
    // read(c,n) reads n bytes from the stream, and stores
    // them in array c.  If the stream contains less than n
    // bytes, or if an I/O error occurs, read(c,n) throws
    // an exception.  If read(c,n) reads the last byte from
    // the file it returns false, otherwise it returns true.
    //------------------------------------------------------

    // InN must be 32bit to match the abstract interface.
    virtual bool read(char c[/*n*/], int inN) {
        uint64_t srcN = (uint64_t)inN;

        if (pos + srcN > size) {
            return false;
        }

        for (uint64_t i = 0; i < srcN; ++i) {
            c[i] = data[pos];
            ++pos;
        }

        return pos >= size;
    }

    //--------------------------------------------------------
    // Get the current reading position, in bytes from the
    // beginning of the file.  If the next call to read() will
    // read the first byte in the file, tellg() returns 0.
    //--------------------------------------------------------

    virtual uint64_t tellg() { return pos; }

    //-------------------------------------------
    // Set the current reading position.
    // After calling seekg(i), tellg() returns i.
    //-------------------------------------------

    virtual void seekg(uint64_t pos) { pos = pos; }

private:
    const char* data;
    uint64_t size;
    uint64_t pos;
};

namespace {

/////////////////////////////////////////
// 8 bit per channel source
void ExtractAndConvertChannel(const uint8_t* src, uint64_t srcChannels, uint64_t x, uint64_t y, float* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = *src / 255.f;
    }
}

void ExtractAndConvertChannel(const uint8_t* src, uint64_t srcChannels, uint64_t x, uint64_t y, Float16* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = Float16(*src / 255.f);
    }
}

/////////////////////////////////////////
// 16 bit per channel source
void ExtractAndConvertChannel(const Float16* src, uint64_t srcChannels, uint64_t x, uint64_t y, float* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = *src;
    }
}

void ExtractAndConvertChannel(const Float16* src, uint64_t srcChannels, uint64_t x, uint64_t y, Float16* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = *src;
    }
}

/////////////////////////////////////////
// 32 bit per channel source
void ExtractAndConvertChannel(const float* src, uint64_t srcChannels, uint64_t x, uint64_t y, float* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = *src;
    }
}

void ExtractAndConvertChannel(const float* src, uint64_t srcChannels, uint64_t x, uint64_t y, Float16* outChannel) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, src += srcChannels) {
        outChannel[i] = *src;
    }
}

/////////////////////////////////////////
int GetNumChannelsFromFormat(ERGBFormat format) {
    switch (format) {
        case ERGBFormat::RGBA:
        case ERGBFormat::BGRA: return 4;
        case ERGBFormat::Gray: return 1;
    }
    Assert(false);
    return 1;
}
}  // namespace

const char* FExrImageWrapper::GetRawChannelName(int channelIndex) const {
    const int maxChannels = 4;
    static const char* RGBAChannelNames[] = {"R", "G", "B", "A"};
    static const char* BGRAChannelNames[] = {"B", "G", "R", "A"};
    static const char* GrayChannelNames[] = {"G"};
    Assert(channelIndex < maxChannels);

    const char** channelNames = BGRAChannelNames;

    switch (rawFormat) {
        case ERGBFormat::RGBA: {
            channelNames = RGBAChannelNames;
        } break;
        case ERGBFormat::BGRA: {
            channelNames = BGRAChannelNames;
        } break;
        case ERGBFormat::Gray: {
            Assert(channelIndex < 1);
            channelNames = GrayChannelNames;
        } break;
        default: Assert(false);
    }
    return channelNames[channelIndex];
}

template <typename Imf::PixelType OutputFormat>
struct TExrImageOutputChannelType;

template <>
struct TExrImageOutputChannelType<Imf::HALF> {
    typedef Float16 Type;
};
template <>
struct TExrImageOutputChannelType<Imf::FLOAT> {
    typedef float Type;
};

template <Imf::PixelType OutputFormat, typename sourcetype>
void FExrImageWrapper::WriteFrameBufferChannel(Imf::FrameBuffer& imfFrameBuffer, const char* channelName, const sourcetype* srcData, std::vector<uint8_t>& channelBuffer) {
    const int64_t outputPixelSize = ((OutputFormat == Imf::HALF) ? 2 : 4);
    channelBuffer.resize(uint64_t(width) * uint64_t(height) * uint64_t(outputPixelSize));
    uint8_t* offsetHead = channelBuffer.data();
    uint64_t srcChannels = GetNumChannelsFromFormat(rawFormat);
    ExtractAndConvertChannel(srcData, srcChannels, width, height, (typename TExrImageOutputChannelType<OutputFormat>::Type*)(offsetHead));
    Imf::Slice frameChannel = Imf::Slice(OutputFormat, (char*)offsetHead, outputPixelSize, uint64_t(width) * uint64_t(outputPixelSize));
    imfFrameBuffer.insert(channelName, frameChannel);
}

template <Imf::PixelType OutputFormat, typename sourcetype>
void FExrImageWrapper::CompressRaw(const sourcetype* srcData, bool bIgnoreAlpha) {
    // const double StartTime = FPlatformTime::Seconds();
    uint32_t numWriteComponents = GetNumChannelsFromFormat(rawFormat);
    if (bIgnoreAlpha && numWriteComponents == 4) {
        numWriteComponents = 3;
    }

    Imf::Compression comp = bUseCompression ? Imf::Compression::ZIP_COMPRESSION : Imf::Compression::NO_COMPRESSION;
    Imf::Header header(width, height, 1, Imath::V2f(0, 0), 1, Imf::LineOrder::INCREASING_Y, comp);

    for (uint32_t channel = 0; channel < numWriteComponents; channel++) {
        header.channels().insert(GetRawChannelName(channel), Imf::Channel(OutputFormat));
    }

    // OutputFormat is 0 UINT, 1 HALF or 2 FLOAT. Size evaluates to 2/4/8 or 1/2/4 compressed
    const int outputPixelSize = (bUseCompression ? 1 : 2) << OutputFormat;
    Assert(outputPixelSize >= 1 && outputPixelSize <= 8);

    FMemFileOut memFile("");
    Imf::FrameBuffer imfFrameBuffer;
    std::vector<uint8_t> channelOutputBuffers[4];

    for (uint32_t channel = 0; channel < numWriteComponents; channel++) {
        WriteFrameBufferChannel<OutputFormat>(imfFrameBuffer, GetRawChannelName(channel), srcData + channel, channelOutputBuffers[channel]);
    }

    int64_t fileLength;
    {
        // This scope ensures that IMF::Outputfile creates a complete file by closing the file when it goes out of scope.
        // To complete the file, EXR seeks back into the file and writes the scanline offsets when the file is closed,
        // which moves the tellp location. So file length is stored in advance for later use.

        Imf::OutputFile imfFile(memFile, header);
        imfFile.setFrameBuffer(imfFrameBuffer);

        memFile.data.resize(memFile.data.size() + int64_t(width) * int64_t(height) * int64_t(numWriteComponents) * int64_t(OutputFormat == 2 ? 4 : 2));
        imfFile.writePixels(height);
        fileLength = memFile.tellp();
    }

    for (int64_t i = 0; i < fileLength; i++) {
        compressedData.push_back(memFile.data[i]);
    }

    // const double DeltaTime = FPlatformTime::Seconds() - StartTime;
    // UE_LOG(LogImageWrapper, Verbose, TEXT("Compressed image in %.3f seconds"), DeltaTime);
}

void FExrImageWrapper::Compress(int quality) {
    Assert(rawData.size() != 0);
    Assert(width > 0);
    Assert(height > 0);
    Assert(rawBitDepth == 8 || rawBitDepth == 16 || rawBitDepth == 32);

    const int maxComponents = 4;

    bUseCompression = (quality != (int)EImageCompressionQuality::Uncompressed);

    switch (rawBitDepth) {
        case 8: CompressRaw<Imf::HALF>(rawData.data(), false); break;
        case 16: CompressRaw<Imf::HALF>((const Float16*)(rawData.data()), false); break;
        case 32: CompressRaw<Imf::FLOAT>((const float*)(rawData.data()), false); break;
        default: Assert(false);
    }
}

void FExrImageWrapper::Uncompress(const ERGBFormat inFormat, const int inBitDepth) {
    // Ensure we haven't already uncompressed the file.
    if (rawData.size() != 0) {
        return;
    }

    FMemFileIn memFile(compressedData.data(), compressedData.size());

    Imf::RgbaInputFile imfFile(memFile);

    Imath::Box2i win = imfFile.dataWindow();

    Assert(bitDepth == 16);
    Assert(width);
    Assert(height);

    uint32_t channels = 4;

    rawData.resize(int64_t(width) * int64_t(height) * int64_t(channels) * int64_t(bitDepth / 8));

    int dx = win.min.x;
    int dy = win.min.y;

    imfFile.setFrameBuffer((Imf::Rgba*)(rawData.data()) - int64_t(dx) - int64_t(dy) * int64_t(width), 1, width);
    imfFile.readPixels(win.min.y, win.max.y);
}

// from http://www.openexr.com/ReadingAndWritingImageFiles.pdf
bool IsThisAnOpenExrFile(Imf::IStream& f) {
    char b[4];
    f.read(b, sizeof(b));

    f.seekg(0);

    return b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01;
}

bool FExrImageWrapper::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) {
    if (!FImageWrapperBase::SetCompressed(inCompressedData, inCompressedSize)) {
        return false;
    }

    FMemFileIn MemFile(inCompressedData, inCompressedSize);

    if (!IsThisAnOpenExrFile(MemFile)) {
        return false;
    }

    Imf::RgbaInputFile imfFile(MemFile);

    Imath::Box2i win = imfFile.dataWindow();

    Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);

    bitDepth = 16;

    width = dim.x;
    height = dim.y;

    // ideally we can specify float here
    format = ERGBFormat::RGBA;

    return true;
}
}  // namespace ImageDecoder
