#pragma once

#ifdef IMAGE_STATIC
#define IMAGE_PORT
#else
#ifdef _WIN32
#ifdef IMAGE_DLL
#define IMAGE_PORT __declspec(dllexport)
#else
#define IMAGE_PORT __declspec(dllimport)
#endif
#else
#if __has_attribute(visibility)
#define IMAGE_PORT __attribute__((visibility("default")))
#else
#define IMAGE_PORT
#endif
#endif
#endif

#include <cstdint>
#include <cassert>

namespace ImageDecoder {

template <class T>
class IMAGE_PORT Vector {
public:
    typedef T* iterator;

    Vector() {
        _size = 0;
        _buf = new T[1];
        _capacity = 1;
    }
    Vector(size_t size, T const& a) {
        if (size > _max_capacity) {
            size = _max_capacity;
        }
        _size = size;
        _capacity = 1;
        while (_capacity < _size) {
            _capacity *= 2;
        }
        _buf = new T[_capacity];
        for (size_t i = 0; i < _size; i++) {
            _buf[i] = a;
        }
    }
    Vector(const Vector<T>& a) {
        _size = a._size;
        _capacity = a._capacity;
        _buf = new T[_capacity];
        for (size_t i = 0; i < _size; i++) {
            _buf[i] = a._buf[i];
        }
    }
    ~Vector() { delete[] _buf; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    size_t max_capacity() const { return _max_capacity; }
    void push_back(const T& val) {
        if (_size < _capacity) {
            _buf[_size] = val;
            _size++;
            return;
        } else if (_size == _max_capacity) {
            return;
        }
        _capacity *= 2;
        if (_capacity >= _max_capacity) {
            _capacity = _max_capacity;
        }
        T* tmp = new T[_capacity];
        for (size_t i = 0; i < _size; i++) {
            tmp[i] = _buf[i];
        }
        tmp[_size] = val;
        _size++;
        delete[] _buf;
        _buf = tmp;
    }
    void pop_back() {
        assert(_size > 0);
        _size--;
    }
    void clear() {
        delete[] _buf;
        _size = 0;
        _buf = new T[1];
        _capacity = 1;
    }
    void resize(size_t size) {
        if (size == _size) {
            return;
        } else if (size <= 0) {
            clear();
        }
        if (size < _size) {
            for (size_t s = size; s < _size; s++) {
                pop_back();
            }
        } else {
            for (size_t s = _size; s < size; s++) {
                T val;
                push_back(val);
            }
        }
    }
    T& operator[](size_t index) {
        assert(index >= 0 && index < _size);
        return _buf[index];
    }
    Vector<T>& operator=(const Vector<T>& a) {
        if (this == &a) {
            return *this;
        }
        delete[] _buf;
        _size = a._size;
        _capacity = a._capacity;
        _buf = new T[_capacity];
        for (size_t i = 0; i < _size; i++) {
            _buf[i] = a._buf[i];
        }
        return *this;
    }
    bool empty() const {
        if (_size == 0) {
            return true;
        }
        return false;
    }
    iterator begin() const { return _buf; }
    iterator end() const { return _buf + _size; }

    const T* data() const { return _buf; }

    T* data() { return _buf; }

private:
    size_t _size;
    size_t _capacity;
    T* _buf;
    const size_t _max_capacity = SIZE_MAX;
};

template class IMAGE_PORT Vector<uint8_t>;
template class IMAGE_PORT Vector<char>;

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
     * @param inCompressedData The memory address of the start of the compressed data.
     * @param inCompressedSize The size of the compressed data parsed.
     * @return true if data was the expected format.
     */
    virtual bool SetCompressed(const void* inCompressedData, int64_t inCompressedSize) = 0;

    /**
     * Sets the compressed data.
     *
     * @param inRawData The memory address of the start of the raw data.
     * @param inRawSize The size of the compressed data parsed.
     * @param inWidth The width of the image data.
     * @param inHeight the height of the image data.
     * @param inFormat the format the raw data is in, normally RGBA.
     * @param inBitDepth the bit-depth per channel, normally 8.
     * @return true if data was the expected format.
     */
    virtual bool SetRaw(const void* inRawData, int64_t inRawSize, const int inWidth, const int inHeight, const ERGBFormat inFormat, const int inBitDepth) = 0;

    /**
     * Set information for animated formats
     * @param inNumFrames The number of frames in the animation (the RawData from SetRaw will need to be a multiple of NumFrames)
     * @param inFramerate The playback rate of the animation
     * @return true if successful
     */
    virtual bool SetAnimationInfo(int inNumFrames, int inFramerate) = 0;

    /**
     * Gets the compressed data.
     *
     * @return Array of the compressed data.
     */
    virtual const Vector<uint8_t>& GetCompressed(int quality = 0) = 0;

    /**
     * Gets the raw data in a TArray. Only use this if you're certain that the image is less than 2 GB in size.
     * Prefer using the overload which takes a TArray64 in general.
     *
     * @param inFormat How we want to manipulate the RGB data.
     * @param inBitDepth The output bit-depth per channel, normally 8.
     * @param outRawData Will contain the uncompressed raw data.
     * @return true on success, false otherwise.
     */
    virtual bool GetRaw(const ERGBFormat inFormat, int inBitDepth, Vector<uint8_t>& outRawData);

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

/**
 * Interface for image wrapper modules.
 */
class IMAGE_PORT IImageWrapperModule {
public:
    /**
     * Create a helper of a specific type
     *
     * @param inFormat - The type of image we want to deal with
     * @return The helper base class to manage the data
     */
    virtual IImageWrapper* CreateImageWrapper(const EImageFormat inFormat) = 0;

    /**
     * Detect image format by looking at the first few bytes of the compressed image data.
     * You can call this method as soon as you have 8-16 bytes of compressed file content available.
     *
     * @param inCompressedData The raw image header.
     * @param inCompressedSize The size of inCompressedData.
     * @return the detected format or EImageFormat::Invalid if the method could not detect the image format.
     */
    virtual EImageFormat DetectImageFormat(const void* inCompressedData, int64_t inCompressedSize) = 0;

public:
    /** Virtual destructor. */
    virtual ~IImageWrapperModule() {}
};

IMAGE_PORT IImageWrapperModule& GetImageWrapperModule();

enum class IMAGE_PORT ETextureSourceFormat {
    Invalid,
    G8,
    G16,
    BGRA8,
    BGRE8,
    RGBA16,
    RGBA16F,
    RGBA8,
    RGBE8,
};

class IMAGE_PORT ImportImage {
public:
    struct IMAGE_PORT Source {
        EImageFormat type = EImageFormat::Invalid;
        ERGBFormat RGBFormat = ERGBFormat::Invalid;
        int bitDepth = 8;
    } source;
    Vector<uint8_t> data;
    ETextureSourceFormat textureformat = ETextureSourceFormat::Invalid;
    int bitDepth = 8;
    int numMips = 0;
    int width = 0;
    int height = 0;
};

bool IMAGE_PORT DecodeImage(EImageFormat imageFormat, const uint8_t* buffer, uint32_t length, bool bFillPNGZeroAlpha, Vector<char>& warn, ImportImage& outImage);
}  // namespace ImageDecoder
