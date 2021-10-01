#include "PngImageWrapper.h"
#include <assert.h>
#include <mutex>

// Disable warning "interaction between '_setjmp' and C++ object destruction is non-portable"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)
#endif

bool IsLitleEndian() {
    int num = 1;
    if (*(char*)&num == 1) {
        return true;
    } else {
        return false;
    }
}

/** Only allow one thread to use libpng at a time (it's not thread safe) */
std::mutex GPNGSection;

/* Local helper classes
 *****************************************************************************/

/**
 * Error type for PNG reading issue.
 */
struct FPNGImageCRCError {
    FPNGImageCRCError(std::string InErrorText) : ErrorText(std::move(InErrorText)) {}

    std::string ErrorText;
};

/**
 * Guard that safely releases PNG reader resources.
 */
class PNGReadGuard {
public:
    PNGReadGuard(png_structp* InReadStruct, png_infop* InInfo) : png_ptr(InReadStruct), info_ptr(InInfo), PNGRowPointers(NULL) {}

    ~PNGReadGuard() {
        if (PNGRowPointers != NULL) {
            png_free(*png_ptr, PNGRowPointers);
        }
        png_destroy_read_struct(png_ptr, info_ptr, NULL);
    }

    void SetRowPointers(png_bytep* InRowPointers) { PNGRowPointers = InRowPointers; }

private:
    png_structp* png_ptr;
    png_infop* info_ptr;
    png_bytep* PNGRowPointers;
};

/**
 * Guard that safely releases PNG Writer resources
 */
class PNGWriteGuard {
public:
    PNGWriteGuard(png_structp* InWriteStruct, png_infop* InInfo) : PNGWriteStruct(InWriteStruct), info_ptr(InInfo), PNGRowPointers(NULL) {}

    ~PNGWriteGuard() {
        if (PNGRowPointers != NULL) {
            png_free(*PNGWriteStruct, PNGRowPointers);
        }
        png_destroy_write_struct(PNGWriteStruct, info_ptr);
    }

    void SetRowPointers(png_bytep* InRowPointers) { PNGRowPointers = InRowPointers; }

private:
    png_structp* PNGWriteStruct;
    png_infop* info_ptr;
    png_bytep* PNGRowPointers;
};

/* FPngImageWrapper structors
 *****************************************************************************/

FPngImageWrapper::FPngImageWrapper() : FImageWrapperBase(), ReadOffset(0), ColorType(0), Channels(0) {}

/* FImageWrapper interface
 *****************************************************************************/

void FPngImageWrapper::Compress(int Quality) {
    if (!CompressedData.size()) {
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // thread safety
        std::lock_guard<std::mutex> PNGLock(GPNGSection);
#endif

        assert(RawData.size());
        assert(Width > 0);
        assert(Height > 0);

        // Reset to the beginning of file so we can use png_read_png(), which expects to start at the beginning.
        ReadOffset = 0;

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn);
        assert(png_ptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        assert(info_ptr);

        png_bytep* row_pointers = (png_bytep*)png_malloc(png_ptr, Height * sizeof(png_bytep));
        PNGWriteGuard PNGGuard(&png_ptr, &info_ptr);
        PNGGuard.SetRowPointers(row_pointers);

        // Store the current stack pointer in the jump buffer. setjmp will return non-zero in the case of a write error.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
        if (setjmp(SetjmpBuffer) != 0)
#else
        // Use libPNG jump buffer solution to allow concurrent compression\decompression on concurrent threads.
        if (setjmp(png_jmpbuf(png_ptr)) != 0)
#endif
        {
            return;
        }

        // ---------------------------------------------------------------------------------------------------------
        // Anything allocated on the stack after this point will not be destructed correctly in the case of an error

        {
            png_set_compression_level(png_ptr, Z_BEST_SPEED);
            png_set_IHDR(png_ptr, info_ptr, Width, Height, RawBitDepth, (RawFormat == ERGBFormat::Gray) ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_set_write_fn(png_ptr, this, FPngImageWrapper::user_write_compressed, FPngImageWrapper::user_flush_data);

            const uint64_t PixelChannels = (RawFormat == ERGBFormat::Gray) ? 1 : 4;
            const uint64_t BytesPerPixel = (RawBitDepth * PixelChannels) / 8;
            const uint64_t BytesPerRow = BytesPerPixel * Width;

            for (int64_t i = 0; i < Height; i++) {
                row_pointers[i] = &RawData[i * BytesPerRow];
            }
            png_set_rows(png_ptr, info_ptr, row_pointers);

            uint32_t Transform = (RawFormat == ERGBFormat::BGRA) ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

            // PNG files store 16-bit pixels in network byte order (big-endian, ie. most significant bits first).
            if (IsLitleEndian()) {
                // We're little endian so we need to swap
                if (RawBitDepth == 16) {
                    Transform |= PNG_TRANSFORM_SWAP_ENDIAN;
                }
            }

            png_write_png(png_ptr, info_ptr, Transform, NULL);
        }
    }
}

void FPngImageWrapper::Reset() {
    FImageWrapperBase::Reset();

    ReadOffset = 0;
    ColorType = 0;
    Channels = 0;
}

bool FPngImageWrapper::SetCompressed(const void* InCompressedData, int64_t InCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(InCompressedData, InCompressedSize);

    return bResult && LoadPNGHeader();  // Fetch the variables from the header info
}

void FPngImageWrapper::Uncompress(const ERGBFormat InFormat, const int InBitDepth) {
    if (!RawData.size() || InFormat != RawFormat || InBitDepth != RawBitDepth) {
        assert(CompressedData.size());
        UncompressPNGData(InFormat, InBitDepth);
    }
}

void FPngImageWrapper::UncompressPNGData(const ERGBFormat InFormat, const int InBitDepth) {
    // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
    // thread safety
    std::lock_guard<std::mutex> PNGLock(GPNGSection);
#endif

    assert(CompressedData.size());
    assert(Width > 0);
    assert(Height > 0);

    // Note that PNGs on PC tend to be BGR
    assert(InFormat == ERGBFormat::BGRA || InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::Gray);  // Other formats unsupported at present
    assert(InBitDepth == 8 || InBitDepth == 16);                                                           // Other formats unsupported at present

    // Reset to the beginning of file so we can use png_read_png(), which expects to start at the beginning.
    ReadOffset = 0;

    png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn, NULL, FPngImageWrapper::user_malloc, FPngImageWrapper::user_free);
    assert(png_ptr);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    assert(info_ptr);
    try {
        png_bytep* row_pointers = (png_bytep*)png_malloc(png_ptr, Height * sizeof(png_bytep));
        PNGReadGuard PNGGuard(&png_ptr, &info_ptr);
        PNGGuard.SetRowPointers(row_pointers);

        // Store the current stack pointer in the jump buffer. setjmp will return non-zero in the case of a read error.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
        if (setjmp(SetjmpBuffer) != 0)
#else
        // Use libPNG jump buffer solution to allow concurrent compression\decompression on concurrent threads.
        if (setjmp(png_jmpbuf(png_ptr)) != 0)
#endif
        {
            return;
        }

        // ---------------------------------------------------------------------------------------------------------
        // Anything allocated on the stack after this point will not be destructed correctly in the case of an error
        {
            if (ColorType == PNG_COLOR_TYPE_PALETTE) {
                png_set_palette_to_rgb(png_ptr);
            }

            if ((ColorType & PNG_COLOR_MASK_COLOR) == 0 && BitDepth < 8) {
                png_set_expand_gray_1_2_4_to_8(png_ptr);
            }

            // Insert alpha channel with full opacity for RGB images without alpha
            if ((ColorType & PNG_COLOR_MASK_ALPHA) == 0 && (InFormat == ERGBFormat::BGRA || InFormat == ERGBFormat::RGBA)) {
                // png images don't set PNG_COLOR_MASK_ALPHA if they have alpha from a tRNS chunk, but png_set_add_alpha seems to be safe regardless
                if ((ColorType & PNG_COLOR_MASK_COLOR) == 0) {
                    png_set_tRNS_to_alpha(png_ptr);
                } else if (ColorType == PNG_COLOR_TYPE_PALETTE) {
                    png_set_tRNS_to_alpha(png_ptr);
                }
                if (InBitDepth == 8) {
                    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
                } else if (InBitDepth == 16) {
                    png_set_add_alpha(png_ptr, 0xffff, PNG_FILLER_AFTER);
                }
            }

            // Calculate Pixel Depth
            const uint64_t PixelChannels = (InFormat == ERGBFormat::Gray) ? 1 : 4;
            const uint64_t BytesPerPixel = (InBitDepth * PixelChannels) / 8;
            const uint64_t BytesPerRow = BytesPerPixel * Width;
            RawData.resize(Height * BytesPerRow);

            png_set_read_fn(png_ptr, this, FPngImageWrapper::user_read_compressed);

            for (int64_t i = 0; i < Height; i++) {
                row_pointers[i] = &RawData[i * BytesPerRow];
            }
            png_set_rows(png_ptr, info_ptr, row_pointers);

            uint32_t Transform = (InFormat == ERGBFormat::BGRA) ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

            // PNG files store 16-bit pixels in network byte order (big-endian, ie. most significant bits first).
            if (IsLitleEndian()) {
                // We're little endian so we need to swap
                if (BitDepth == 16) {
                    Transform |= PNG_TRANSFORM_SWAP_ENDIAN;
                }
            }

            // Convert grayscale png to RGB if requested
            if ((ColorType & PNG_COLOR_MASK_COLOR) == 0 && (InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA)) {
                Transform |= PNG_TRANSFORM_GRAY_TO_RGB;
            }

            // Convert RGB png to grayscale if requested
            if ((ColorType & PNG_COLOR_MASK_COLOR) != 0 && InFormat == ERGBFormat::Gray) {
                png_set_rgb_to_gray_fixed(png_ptr, 2 /* warn if image is in color */, -1, -1);
            }

            // Strip alpha channel if requested output is grayscale
            if (InFormat == ERGBFormat::Gray) {
                // this is not necessarily the best option, instead perhaps:
                // png_color background = {0,0,0};
                // png_set_background(png_ptr, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
                Transform |= PNG_TRANSFORM_STRIP_ALPHA;
            }

            // Reduce 16-bit to 8-bit if requested
            if (BitDepth == 16 && InBitDepth == 8) {
#if PNG_LIBPNG_VER >= 10504
                assert(0);  // Needs testing
                Transform |= PNG_TRANSFORM_SCALE_16;
#else
                Transform |= PNG_TRANSFORM_STRIP_16;
#endif
            }

            // Increase 8-bit to 16-bit if requested
            if (BitDepth <= 8 && InBitDepth == 16) {
#if PNG_LIBPNG_VER >= 10504
                assert(0);  // Needs testing
                Transform |= PNG_TRANSFORM_EXPAND_16;
#else
                // Expanding 8-bit images to 16-bit via transform needs a libpng update
                assert(0);
#endif
            }

            png_read_png(png_ptr, info_ptr, Transform, NULL);
        }
    } catch (const FPNGImageCRCError& e) {
        /**
         *	libPNG has a known issue in version 1.5.2 causing
         *	an unhandled exception upon a CRC error. This code
         *	catches our custom exception thrown in user_error_fn.
         */
        std::cerr << e.ErrorText << std::endl;
    }

    RawFormat = InFormat;
    RawBitDepth = InBitDepth;
}

/* FPngImageWrapper implementation
 *****************************************************************************/

bool FPngImageWrapper::IsPNG() const {
    assert(CompressedData.size());

    const int PNGSigSize = sizeof(png_size_t);

    if (CompressedData.size() > PNGSigSize) {
        png_size_t PNGSignature = *reinterpret_cast<const png_size_t*>(CompressedData.data());
        return (0 == png_sig_cmp(reinterpret_cast<png_bytep>(&PNGSignature), 0, PNGSigSize));
    }

    return false;
}

bool FPngImageWrapper::LoadPNGHeader() {
    assert(CompressedData.size());

    // Test whether the data this PNGLoader is pointing at is a PNG or not.
    if (IsPNG()) {
        // thread safety
        std::lock_guard<std::mutex> PNGLock(GPNGSection);

        png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn, NULL, FPngImageWrapper::user_malloc, FPngImageWrapper::user_free);
        assert(png_ptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        assert(info_ptr);

        PNGReadGuard PNGGuard(&png_ptr, &info_ptr);
        {
            png_set_read_fn(png_ptr, this, FPngImageWrapper::user_read_compressed);

            png_read_info(png_ptr, info_ptr);

            Width = info_ptr->width;
            Height = info_ptr->height;
            ColorType = info_ptr->color_type;
            BitDepth = info_ptr->bit_depth;
            Channels = info_ptr->channels;
            Format = (ColorType & PNG_COLOR_MASK_COLOR) ? ERGBFormat::RGBA : ERGBFormat::Gray;
        }

        return true;
    }

    return false;
}

/* FPngImageWrapper static implementation
 *****************************************************************************/

void FPngImageWrapper::user_read_compressed(png_structp png_ptr, png_bytep data, png_size_t length) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_io_ptr(png_ptr);
    if (ctx->ReadOffset + (int64_t)length <= ctx->CompressedData.size()) {
        memcpy(data, &ctx->CompressedData[ctx->ReadOffset], length);
        ctx->ReadOffset += length;
    } else {
        ctx->SetError("Invalid read position for CompressedData.");
    }
}

void FPngImageWrapper::user_write_compressed(png_structp png_ptr, png_bytep data, png_size_t length) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_io_ptr(png_ptr);
    for (int i = 0; i < length; i++) {
        ctx->CompressedData.push_back(data[i]);
    }
}

void FPngImageWrapper::user_flush_data(png_structp png_ptr) {}

void FPngImageWrapper::user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_error_ptr(png_ptr);

    {
        std::string ErrorMsg = error_msg;
        ctx->SetError(ErrorMsg.c_str());

        std::cerr << "PNG Error: " << ErrorMsg << std::endl;

        /**
         *	libPNG has a known issue in version 1.5.2 causing
         *	an unhandled exception upon a CRC error. This code
         *	detects the error manually and throws our own
         *	exception to be handled.
         */
        if (ErrorMsg.find("CRC error") != std::string::npos) {
            throw FPNGImageCRCError(ErrorMsg);
        }
    }

    // Ensure that FString is destructed prior to executing the longjmp

#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
    // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
    // The other platforms use libPNG jump buffer solution to allow concurrent compression\decompression on concurrent threads. The jump is trigered in libPNG after this function returns.
    longjmp(ctx->SetjmpBuffer, 1);
#endif
}

void FPngImageWrapper::user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) { std::cerr << "PNG Warning: " << warning_msg << std::endl; }

void* FPngImageWrapper::user_malloc(png_structp /*png_ptr*/, png_size_t size) {
    assert(size > 0);
    return malloc(size);
}

void FPngImageWrapper::user_free(png_structp /*png_ptr*/, png_voidp struct_ptr) {
    assert(struct_ptr);
    free(struct_ptr);
}

// Renable warning "interaction between '_setjmp' and C++ object destruction is non-portable"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
