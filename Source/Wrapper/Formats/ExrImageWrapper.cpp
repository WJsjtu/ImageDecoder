#include "ExrImageWrapper.h"
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <cmath>

typedef half Float16;

FExrImageWrapper::FExrImageWrapper() : FImageWrapperBase(), bUseCompression(true) {}

template <typename sourcetype>
class FSourceImageRaw {
public:
    FSourceImageRaw(const std::vector<uint8_t>& SourceImageBitmapIN, uint64_t ChannelsIN, uint64_t WidthIN, uint64_t HeightIN) : SourceImageBitmap(SourceImageBitmapIN), Width(WidthIN), Height(HeightIN), Channels(ChannelsIN) {
        check(SourceImageBitmap.size() == Channels * Width * Height * sizeof(sourcetype));
    }

    uint64_t GetXStride() const { return sizeof(sourcetype) * Channels; }
    uint64_t GetYStride() const { return GetXStride() * Width; }

    const sourcetype* GetHorzLine(uint64_t y, uint64_t Channel) { return &SourceImageBitmap[(sizeof(sourcetype) * Channel) + (GetYStride() * y)]; }

private:
    const std::vector<uint8_t>& SourceImageBitmap;
    uint64_t Width;
    uint64_t Height;
    uint64_t Channels;
};

class FMemFileOut : public Imf::OStream {
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    FMemFileOut(const char fileName[]) : Imf::OStream(fileName), Pos(0) {}

    // InN must be 32bit to match the abstract interface.
    virtual void write(const char c[/*n*/], int InN) {
        uint64_t SrcN = (uint64_t)InN;
        uint64_t DestPost = Pos + SrcN;
        if (DestPost > Data.size()) {
            Data.resize(Data.size() + std::max((uint64_t)Data.size() * 2, DestPost) - Data.size());
        }

        for (uint64_t i = 0; i < SrcN; ++i) {
            Data[Pos + i] = c[i];
        }
        Pos += SrcN;
    }

    //---------------------------------------------------------
    // Get the current writing position, in bytes from the
    // beginning of the file.  If the next call to write() will
    // start writing at the beginning of the file, tellp()
    // returns 0.
    //---------------------------------------------------------

    virtual uint64_t tellp() { return Pos; }

    //-------------------------------------------
    // Set the current writing position.
    // After calling seekp(i), tellp() returns i.
    //-------------------------------------------

    virtual void seekp(uint64_t pos) { Pos = pos; }

    uint64_t Pos;
    std::vector<uint8_t> Data;
};

class FMemFileIn : public Imf::IStream {
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    FMemFileIn(const void* InData, int64_t InSize) : Imf::IStream(""), Data((const char*)InData), Size(InSize), Pos(0) {}

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
    virtual bool read(char c[/*n*/], int InN) {
        uint64_t SrcN = (uint64_t)InN;

        if (Pos + SrcN > Size) {
            return false;
        }

        for (uint64_t i = 0; i < SrcN; ++i) {
            c[i] = Data[Pos];
            ++Pos;
        }

        return Pos >= Size;
    }

    //--------------------------------------------------------
    // Get the current reading position, in bytes from the
    // beginning of the file.  If the next call to read() will
    // read the first byte in the file, tellg() returns 0.
    //--------------------------------------------------------

    virtual uint64_t tellg() { return Pos; }

    //-------------------------------------------
    // Set the current reading position.
    // After calling seekg(i), tellg() returns i.
    //-------------------------------------------

    virtual void seekg(uint64_t pos) { Pos = pos; }

private:
    const char* Data;
    uint64_t Size;
    uint64_t Pos;
};

namespace {

/////////////////////////////////////////
// 8 bit per channel source
void ExtractAndConvertChannel(const uint8_t* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, float* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = *Src / 255.f;
    }
}

void ExtractAndConvertChannel(const uint8_t* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, Float16* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = Float16(*Src / 255.f);
    }
}

/////////////////////////////////////////
// 16 bit per channel source
void ExtractAndConvertChannel(const Float16* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, float* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = *Src;
    }
}

void ExtractAndConvertChannel(const Float16* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, Float16* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = *Src;
    }
}

/////////////////////////////////////////
// 32 bit per channel source
void ExtractAndConvertChannel(const float* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, float* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = *Src;
    }
}

void ExtractAndConvertChannel(const float* Src, uint64_t SrcChannels, uint64_t x, uint64_t y, Float16* ChannelOUT) {
    for (uint64_t i = 0; i < uint64_t(x) * uint64_t(y); i++, Src += SrcChannels) {
        ChannelOUT[i] = *Src;
    }
}

/////////////////////////////////////////
int GetNumChannelsFromFormat(ERGBFormat Format) {
    switch (Format) {
        case ERGBFormat::RGBA:
        case ERGBFormat::BGRA: return 4;
        case ERGBFormat::Gray: return 1;
    }
    assert(false);
    return 1;
}
}  // namespace

const char* FExrImageWrapper::GetRawChannelName(int ChannelIndex) const {
    const int MaxChannels = 4;
    static const char* RGBAChannelNames[] = {"R", "G", "B", "A"};
    static const char* BGRAChannelNames[] = {"B", "G", "R", "A"};
    static const char* GrayChannelNames[] = {"G"};
    assert(ChannelIndex < MaxChannels);

    const char** ChannelNames = BGRAChannelNames;

    switch (RawFormat) {
        case ERGBFormat::RGBA: {
            ChannelNames = RGBAChannelNames;
        } break;
        case ERGBFormat::BGRA: {
            ChannelNames = BGRAChannelNames;
        } break;
        case ERGBFormat::Gray: {
            assert(ChannelIndex < 1);
            ChannelNames = GrayChannelNames;
        } break;
        default: assert(false);
    }
    return ChannelNames[ChannelIndex];
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
void FExrImageWrapper::WriteFrameBufferChannel(Imf::FrameBuffer& ImfFrameBuffer, const char* ChannelName, const sourcetype* SrcData, std::vector<uint8_t>& ChannelBuffer) {
    const int64_t OutputPixelSize = ((OutputFormat == Imf::HALF) ? 2 : 4);
    ChannelBuffer.resize(uint64_t(Width) * uint64_t(Height) * uint64_t(OutputPixelSize));
    uint8_t* OffsetHead = ChannelBuffer.data();
    uint64_t SrcChannels = GetNumChannelsFromFormat(RawFormat);
    ExtractAndConvertChannel(SrcData, SrcChannels, Width, Height, (typename TExrImageOutputChannelType<OutputFormat>::Type*)(OffsetHead));
    Imf::Slice FrameChannel = Imf::Slice(OutputFormat, (char*)OffsetHead, OutputPixelSize, uint64_t(Width) * uint64_t(OutputPixelSize));
    ImfFrameBuffer.insert(ChannelName, FrameChannel);
}

template <Imf::PixelType OutputFormat, typename sourcetype>
void FExrImageWrapper::CompressRaw(const sourcetype* SrcData, bool bIgnoreAlpha) {
    // const double StartTime = FPlatformTime::Seconds();
    uint32_t NumWriteComponents = GetNumChannelsFromFormat(RawFormat);
    if (bIgnoreAlpha && NumWriteComponents == 4) {
        NumWriteComponents = 3;
    }

    Imf::Compression Comp = bUseCompression ? Imf::Compression::ZIP_COMPRESSION : Imf::Compression::NO_COMPRESSION;
    Imf::Header Header(Width, Height, 1, Imath::V2f(0, 0), 1, Imf::LineOrder::INCREASING_Y, Comp);

    for (uint32_t Channel = 0; Channel < NumWriteComponents; Channel++) {
        Header.channels().insert(GetRawChannelName(Channel), Imf::Channel(OutputFormat));
    }

    // OutputFormat is 0 UINT, 1 HALF or 2 FLOAT. Size evaluates to 2/4/8 or 1/2/4 compressed
    const int OutputPixelSize = (bUseCompression ? 1 : 2) << OutputFormat;
    assert(OutputPixelSize >= 1 && OutputPixelSize <= 8);

    FMemFileOut MemFile("");
    Imf::FrameBuffer ImfFrameBuffer;
    std::vector<uint8_t> ChannelOutputBuffers[4];

    for (uint32_t Channel = 0; Channel < NumWriteComponents; Channel++) {
        WriteFrameBufferChannel<OutputFormat>(ImfFrameBuffer, GetRawChannelName(Channel), SrcData + Channel, ChannelOutputBuffers[Channel]);
    }

    int64_t FileLength;
    {
        // This scope ensures that IMF::Outputfile creates a complete file by closing the file when it goes out of scope.
        // To complete the file, EXR seeks back into the file and writes the scanline offsets when the file is closed,
        // which moves the tellp location. So file length is stored in advance for later use.

        Imf::OutputFile ImfFile(MemFile, Header);
        ImfFile.setFrameBuffer(ImfFrameBuffer);

        MemFile.Data.resize(MemFile.Data.size() + int64_t(Width) * int64_t(Height) * int64_t(NumWriteComponents) * int64_t(OutputFormat == 2 ? 4 : 2));
        ImfFile.writePixels(Height);
        FileLength = MemFile.tellp();
    }

    for (int64_t i = 0; i < FileLength; i++) {
        CompressedData.push_back(MemFile.Data[i]);
    }

    // const double DeltaTime = FPlatformTime::Seconds() - StartTime;
    // UE_LOG(LogImageWrapper, Verbose, TEXT("Compressed image in %.3f seconds"), DeltaTime);
}

void FExrImageWrapper::Compress(int Quality) {
    assert(RawData.size() != 0);
    assert(Width > 0);
    assert(Height > 0);
    assert(RawBitDepth == 8 || RawBitDepth == 16 || RawBitDepth == 32);

    const int MaxComponents = 4;

    bUseCompression = (Quality != (int)EImageCompressionQuality::Uncompressed);

    switch (RawBitDepth) {
        case 8: CompressRaw<Imf::HALF>(RawData.data(), false); break;
        case 16: CompressRaw<Imf::HALF>((const Float16*)(RawData.data()), false); break;
        case 32: CompressRaw<Imf::FLOAT>((const float*)(RawData.data()), false); break;
        default: assert(false);
    }
}

void FExrImageWrapper::Uncompress(const ERGBFormat InFormat, const int InBitDepth) {
    // Ensure we haven't already uncompressed the file.
    if (RawData.size() != 0) {
        return;
    }

    FMemFileIn MemFile(CompressedData.data(), CompressedData.size());

    Imf::RgbaInputFile ImfFile(MemFile);

    Imath::Box2i win = ImfFile.dataWindow();

    assert(BitDepth == 16);
    assert(Width);
    assert(Height);

    uint32_t Channels = 4;

    RawData.resize(int64_t(Width) * int64_t(Height) * int64_t(Channels) * int64_t(BitDepth / 8));

    int dx = win.min.x;
    int dy = win.min.y;

    ImfFile.setFrameBuffer((Imf::Rgba*)(RawData.data()) - int64_t(dx) - int64_t(dy) * int64_t(Width), 1, Width);
    ImfFile.readPixels(win.min.y, win.max.y);
}

// from http://www.openexr.com/ReadingAndWritingImageFiles.pdf
bool IsThisAnOpenExrFile(Imf::IStream& f) {
    char b[4];
    f.read(b, sizeof(b));

    f.seekg(0);

    return b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01;
}

bool FExrImageWrapper::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) {
    if (!FImageWrapperBase::SetCompressed(InCompressedData, InCompressedSize)) {
        return false;
    }

    FMemFileIn MemFile(InCompressedData, InCompressedSize);

    if (!IsThisAnOpenExrFile(MemFile)) {
        return false;
    }

    Imf::RgbaInputFile ImfFile(MemFile);

    Imath::Box2i win = ImfFile.dataWindow();

    Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);

    BitDepth = 16;

    Width = dim.x;
    Height = dim.y;

    // ideally we can specify float here
    Format = ERGBFormat::RGBA;

    return true;
}
