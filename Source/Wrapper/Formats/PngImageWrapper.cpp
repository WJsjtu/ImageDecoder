#include "PngImageWrapper.h"
#include "Utils/Utils.h"
#include <mutex>

namespace ImageDecoder {

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
    FPNGImageCRCError(std::string inErrorText) : errorText(std::move(inErrorText)) {}

    std::string errorText;
};

/**
 * Guard that safely releases PNG reader resources.
 */
class PNGReadGuard {
public:
    PNGReadGuard(png_structp* inReadStruct, png_infop* inInfo) : png_ptr(inReadStruct), info_ptr(inInfo), pngRowPointers(NULL) {}

    ~PNGReadGuard() {
        if (pngRowPointers != NULL) {
            png_free(*png_ptr, pngRowPointers);
        }
        png_destroy_read_struct(png_ptr, info_ptr, NULL);
    }

    void SetRowPointers(png_bytep* inRowPointers) { pngRowPointers = inRowPointers; }

private:
    png_structp* png_ptr;
    png_infop* info_ptr;
    png_bytep* pngRowPointers;
};

/**
 * Guard that safely releases PNG Writer resources
 */
class PNGWriteGuard {
public:
    PNGWriteGuard(png_structp* inWriteStruct, png_infop* inInfo) : pngWriteStruct(inWriteStruct), info_ptr(inInfo), pngRowPointers(NULL) {}

    ~PNGWriteGuard() {
        if (pngRowPointers != NULL) {
            png_free(*pngWriteStruct, pngRowPointers);
        }
        png_destroy_write_struct(pngWriteStruct, info_ptr);
    }

    void SetRowPointers(png_bytep* inRowPointers) { pngRowPointers = inRowPointers; }

private:
    png_structp* pngWriteStruct;
    png_infop* info_ptr;
    png_bytep* pngRowPointers;
};

/* FPngImageWrapper structors
 *****************************************************************************/

FPngImageWrapper::FPngImageWrapper() : FImageWrapperBase(), readOffset(0), colorType(0), channels(0) {}

/* FImageWrapper interface
 *****************************************************************************/

void FPngImageWrapper::Compress(int quality) {
    if (!compressedData.size()) {
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // thread safety
        std::lock_guard<std::mutex> pngLock(GPNGSection);
#endif

        Assert(rawData.size());
        Assert(width > 0);
        Assert(height > 0);

        // Reset to the beginning of file so we can use png_read_png(), which expects to start at the beginning.
        readOffset = 0;

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn);
        Assert(png_ptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        Assert(info_ptr);

        png_bytep* row_pointers = (png_bytep*)png_malloc(png_ptr, height * sizeof(png_bytep));
        PNGWriteGuard pngGuard(&png_ptr, &info_ptr);
        pngGuard.SetRowPointers(row_pointers);

        // Store the current stack pointer in the jump buffer. setjmp will return non-zero in the case of a write error.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
        if (setjmp(setjmpBuffer) != 0)
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
            png_set_IHDR(png_ptr, info_ptr, width, height, rawBitDepth, (rawFormat == ERGBFormat::Gray) ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_set_write_fn(png_ptr, this, FPngImageWrapper::user_write_compressed, FPngImageWrapper::user_flush_data);

            const uint64_t pixelChannels = (rawFormat == ERGBFormat::Gray) ? 1 : 4;
            const uint64_t bytesPerPixel = (rawBitDepth * pixelChannels) / 8;
            const uint64_t bytesPerRow = bytesPerPixel * width;

            for (int64_t i = 0; i < height; i++) {
                row_pointers[i] = &rawData[i * bytesPerRow];
            }
            png_set_rows(png_ptr, info_ptr, row_pointers);

            uint32_t transform = (rawFormat == ERGBFormat::BGRA) ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

            // PNG files store 16-bit pixels in network byte order (big-endian, ie. most significant bits first).
            if (IsLitleEndian()) {
                // We're little endian so we need to swap
                if (rawBitDepth == 16) {
                    transform |= PNG_TRANSFORM_SWAP_ENDIAN;
                }
            }

            png_write_png(png_ptr, info_ptr, transform, NULL);
        }
    }
}

void FPngImageWrapper::Reset() {
    FImageWrapperBase::Reset();

    readOffset = 0;
    colorType = 0;
    channels = 0;
}

bool FPngImageWrapper::SetCompressed(const void* inCompressedData, int64_t inCompressedSize) {
    bool bResult = FImageWrapperBase::SetCompressed(inCompressedData, inCompressedSize);

    return bResult && LoadPNGHeader();  // Fetch the variables from the header info
}

void FPngImageWrapper::Uncompress(const ERGBFormat inFormat, const int inBitDepth) {
    if (!rawData.size() || inFormat != rawFormat || inBitDepth != rawBitDepth) {
        Assert(compressedData.size());
        UncompressPNGData(inFormat, inBitDepth);
    }
}

void FPngImageWrapper::UncompressPNGData(const ERGBFormat inFormat, const int inBitDepth) {
    // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
    // thread safety
    std::lock_guard<std::mutex> pngLock(GPNGSection);
#endif

    Assert(compressedData.size());
    Assert(width > 0);
    Assert(height > 0);

    // Note that PNGs on PC tend to be BGR
    Assert(inFormat == ERGBFormat::BGRA || inFormat == ERGBFormat::RGBA || inFormat == ERGBFormat::Gray);  // Other formats unsupported at present
    Assert(inBitDepth == 8 || inBitDepth == 16);                                                           // Other formats unsupported at present

    // Reset to the beginning of file so we can use png_read_png(), which expects to start at the beginning.
    readOffset = 0;

    png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn, NULL, FPngImageWrapper::user_malloc, FPngImageWrapper::user_free);
    Assert(png_ptr);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    Assert(info_ptr);
    try {
        png_bytep* row_pointers = (png_bytep*)png_malloc(png_ptr, height * sizeof(png_bytep));
        PNGReadGuard pngGuard(&png_ptr, &info_ptr);
        pngGuard.SetRowPointers(row_pointers);

        // Store the current stack pointer in the jump buffer. setjmp will return non-zero in the case of a read error.
#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
        // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
        if (setjmp(setjmpBuffer) != 0)
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
            if (colorType == PNG_COLOR_TYPE_PALETTE) {
                png_set_palette_to_rgb(png_ptr);
            }

            if ((colorType & PNG_COLOR_MASK_COLOR) == 0 && bitDepth < 8) {
                png_set_expand_gray_1_2_4_to_8(png_ptr);
            }

            // Insert alpha channel with full opacity for RGB images without alpha
            if ((colorType & PNG_COLOR_MASK_ALPHA) == 0 && (inFormat == ERGBFormat::BGRA || inFormat == ERGBFormat::RGBA)) {
                // png images don't set PNG_COLOR_MASK_ALPHA if they have alpha from a tRNS chunk, but png_set_add_alpha seems to be safe regardless
                if ((colorType & PNG_COLOR_MASK_COLOR) == 0) {
                    png_set_tRNS_to_alpha(png_ptr);
                } else if (colorType == PNG_COLOR_TYPE_PALETTE) {
                    png_set_tRNS_to_alpha(png_ptr);
                }
                if (inBitDepth == 8) {
                    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
                } else if (inBitDepth == 16) {
                    png_set_add_alpha(png_ptr, 0xffff, PNG_FILLER_AFTER);
                }
            }

            // Calculate Pixel Depth
            const uint64_t pixelChannels = (inFormat == ERGBFormat::Gray) ? 1 : 4;
            const uint64_t bytesPerPixel = (inBitDepth * pixelChannels) / 8;
            const uint64_t bytesPerRow = bytesPerPixel * width;
            rawData.resize(height * bytesPerRow);

            png_set_read_fn(png_ptr, this, FPngImageWrapper::user_read_compressed);

            for (int64_t i = 0; i < height; i++) {
                row_pointers[i] = &rawData[i * bytesPerRow];
            }
            png_set_rows(png_ptr, info_ptr, row_pointers);

            uint32_t transform = (inFormat == ERGBFormat::BGRA) ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

            // PNG files store 16-bit pixels in network byte order (big-endian, ie. most significant bits first).
            if (IsLitleEndian()) {
                // We're little endian so we need to swap
                if (bitDepth == 16) {
                    transform |= PNG_TRANSFORM_SWAP_ENDIAN;
                }
            }

            // Convert grayscale png to RGB if requested
            if ((colorType & PNG_COLOR_MASK_COLOR) == 0 && (inFormat == ERGBFormat::RGBA || inFormat == ERGBFormat::BGRA)) {
                transform |= PNG_TRANSFORM_GRAY_TO_RGB;
            }

            // Convert RGB png to grayscale if requested
            if ((colorType & PNG_COLOR_MASK_COLOR) != 0 && inFormat == ERGBFormat::Gray) {
                png_set_rgb_to_gray_fixed(png_ptr, 2 /* warn if image is in color */, -1, -1);
            }

            // Strip alpha channel if requested output is grayscale
            if (inFormat == ERGBFormat::Gray) {
                // this is not necessarily the best option, instead perhaps:
                // png_color background = {0,0,0};
                // png_set_background(png_ptr, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
                transform |= PNG_TRANSFORM_STRIP_ALPHA;
            }

            // Reduce 16-bit to 8-bit if requested
            if (bitDepth == 16 && inBitDepth == 8) {
#if PNG_LIBPNG_VER >= 10504
                Assert(0);  // Needs testing
                transform |= PNG_TRANSFORM_SCALE_16;
#else
                Transform |= PNG_TRANSFORM_STRIP_16;
#endif
            }

            // Increase 8-bit to 16-bit if requested
            if (bitDepth <= 8 && inBitDepth == 16) {
#if PNG_LIBPNG_VER >= 10504
                Assert(0);  // Needs testing
                transform |= PNG_TRANSFORM_EXPAND_16;
#else
                // Expanding 8-bit images to 16-bit via transform needs a libpng update
                Assert(0);
#endif
            }

            png_read_png(png_ptr, info_ptr, transform, NULL);
        }
    } catch (const FPNGImageCRCError& e) {
        /**
         *	libPNG has a known issue in version 1.5.2 causing
         *	an unhandled exception upon a CRC error. This code
         *	catches our custom exception thrown in user_error_fn.
         */
        std::cerr << e.errorText << std::endl;
    }

    rawFormat = inFormat;
    rawBitDepth = inBitDepth;
}

/* FPngImageWrapper implementation
 *****************************************************************************/

bool FPngImageWrapper::IsPNG() const {
    Assert(compressedData.size());

    const int pngSigSize = sizeof(png_size_t);

    if (compressedData.size() > pngSigSize) {
        png_size_t pngSignature = *reinterpret_cast<const png_size_t*>(compressedData.data());
        return (0 == png_sig_cmp(reinterpret_cast<png_bytep>(&pngSignature), 0, pngSigSize));
    }

    return false;
}

bool FPngImageWrapper::LoadPNGHeader() {
    Assert(compressedData.size());

    // Test whether the data this PNGLoader is pointing at is a PNG or not.
    if (IsPNG()) {
        // thread safety
        std::lock_guard<std::mutex> pngLock(GPNGSection);

        png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, this, FPngImageWrapper::user_error_fn, FPngImageWrapper::user_warning_fn, NULL, FPngImageWrapper::user_malloc, FPngImageWrapper::user_free);
        Assert(png_ptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        Assert(info_ptr);

        PNGReadGuard pngGuard(&png_ptr, &info_ptr);
        {
            png_set_read_fn(png_ptr, this, FPngImageWrapper::user_read_compressed);

            png_read_info(png_ptr, info_ptr);

            width = info_ptr->width;
            height = info_ptr->height;
            colorType = info_ptr->color_type;
            bitDepth = info_ptr->bit_depth;
            channels = info_ptr->channels;
            format = (colorType & PNG_COLOR_MASK_COLOR) ? ERGBFormat::RGBA : ERGBFormat::Gray;
        }

        return true;
    }

    return false;
}

/* FPngImageWrapper static implementation
 *****************************************************************************/

void FPngImageWrapper::user_read_compressed(png_structp png_ptr, png_bytep data, png_size_t length) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_io_ptr(png_ptr);
    if (static_cast<uint64_t>(ctx->readOffset) + static_cast<uint64_t>(length) <= ctx->compressedData.size()) {
        memcpy(data, &ctx->compressedData[ctx->readOffset], length);
        ctx->readOffset += length;
    } else {
        ctx->SetError("Invalid read position for CompressedData.");
    }
}

void FPngImageWrapper::user_write_compressed(png_structp png_ptr, png_bytep data, png_size_t length) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_io_ptr(png_ptr);
    for (int i = 0; i < length; i++) {
        ctx->compressedData.push_back(data[i]);
    }
}

void FPngImageWrapper::user_flush_data(png_structp png_ptr) {}

void FPngImageWrapper::user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    FPngImageWrapper* ctx = (FPngImageWrapper*)png_get_error_ptr(png_ptr);

    {
        std::string errorMsg = error_msg;
        ctx->SetError(errorMsg.c_str());

        std::cerr << "PNG Error: " << errorMsg << std::endl;

        /**
         *	libPNG has a known issue in version 1.5.2 causing
         *	an unhandled exception upon a CRC error. This code
         *	detects the error manually and throws our own
         *	exception to be handled.
         */
        if (errorMsg.find("CRC error") != std::string::npos) {
            throw FPNGImageCRCError(errorMsg);
        }
    }

    // Ensure that FString is destructed prior to executing the longjmp

#if PLATFORM_ANDROID || PLATFORM_LUMIN || PLATFORM_LUMINGL4
    // Preserve old single thread code on some platform in relation to a type incompatibility at compile time.
    // The other platforms use libPNG jump buffer solution to allow concurrent compression\decompression on concurrent threads. The jump is trigered in libPNG after this function returns.
    longjmp(ctx->setjmpBuffer, 1);
#endif
}

void FPngImageWrapper::user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) { std::cerr << "PNG Warning: " << warning_msg << std::endl; }

void* FPngImageWrapper::user_malloc(png_structp /*png_ptr*/, png_size_t size) {
    Assert(size > 0);
    return malloc(size);
}

void FPngImageWrapper::user_free(png_structp /*png_ptr*/, png_voidp struct_ptr) {
    Assert(struct_ptr);
    free(struct_ptr);
}

// Renable warning "interaction between '_setjmp' and C++ object destruction is non-portable"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}  // namespace ImageDecoder
