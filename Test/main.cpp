#include "GLContext.h"
#include "Decoder.h"
#include "GL/glew.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

std::string GetFileExtension(const char* file_name) {
    int ext = '.';
    const char* extension = NULL;
    extension = strrchr(file_name, ext);

    if (extension == NULL) {
        return "";
    }
    return extension;
}

int main(int argc, char* argv[]) {
    auto DllHandle = LoadLibrary("image.dll");
    if (!DllHandle) {
        return -1;
    }
    // declaration of function pointer
    bool(__cdecl * DecodeFunc)(ImageDecoder::EImageFormat, const uint8_t*, uint32_t, ImageDecoder::ImportImage*);
    auto DecodeFuncPtr = (bool(__cdecl*)(ImageDecoder::EImageFormat, const uint8_t*, uint32_t, ImageDecoder::ImportImage*))GetProcAddress(DllHandle, (LPCSTR) "Decode");
    if (!DecodeFuncPtr) {
        return -1;
    }
    if (argc < 2) {
        return -1;
    }
    std::string filePath = argv[1];
    std::vector<uint8_t> dataBinary;
    std::ifstream fileStream(filePath, std::ios_base::in | std::ios::binary);
    if (fileStream) {
        fileStream.unsetf(std::ios::skipws);
        fileStream.seekg(0, std::ios::end);
        dataBinary.resize(static_cast<size_t>(fileStream.tellg()));
        fileStream.seekg(0, std::ios::beg);
        fileStream.read(reinterpret_cast<char*>(dataBinary.data()), dataBinary.size());
        dataBinary.resize(static_cast<size_t>(fileStream.gcount()));
    } else {
        return -1;
    }
    std::string fileType = GetFileExtension(filePath.data());
    ImageDecoder::EImageFormat format = ImageDecoder::EImageFormat::Invalid;
    if (fileType == ".png") {
        format = ImageDecoder::EImageFormat::PNG;
    } else if (fileType == ".jpeg" || fileType == ".jpg") {
        format = ImageDecoder::EImageFormat::JPEG;
    } else if (fileType == ".bmp") {
        format = ImageDecoder::EImageFormat::BMP;
    } else if (fileType == ".ico") {
        format = ImageDecoder::EImageFormat::ICO;
    } else if (fileType == ".exr") {
        format = ImageDecoder::EImageFormat::EXR;
    } else if (fileType == ".pcx") {
        format = ImageDecoder::EImageFormat::PCX;
    } else if (fileType == ".tga") {
        format = ImageDecoder::EImageFormat::TGA;
    }
    if (format == ImageDecoder::EImageFormat::Invalid) {
        return -1;
    }
    ImageDecoder::ImportImage image;
    bool success = DecodeFuncPtr(format, dataBinary.data(), dataBinary.size(), &image);
    if (!success) {
        return -1;
    }

    static int ChannelMap[] = {
        0,  // Invalid,
        1,  // G8,
        1,  // G16,
        4,  // BGRA8,
        4,  // BGRE8,
        4,  // RGBA16,
        4,  // RGBA16F,
        4,  // RGBA8,
        4,  // RGBE8,
    };

    class Texture {
    public:
        enum class EWrapMode { Repeat, Clamp };
        enum class EPixelFormat { RGBA8, RGBA16F };

    public:
        uint32_t width = 0;
        uint32_t height = 0;
        EWrapMode wrapU = EWrapMode::Repeat;
        EWrapMode wrapV = EWrapMode::Repeat;
        EPixelFormat pixelFormat = EPixelFormat::RGBA8;
        std::vector<char> raw;
        std::vector<uint8_t> decoded;
        bool bNeedPremultiplyAlpha = false;
        bool bUseMipMap = false;
        bool bEnableCompress = true;
        uint32_t numMips = 0;
        std::string fileType;
        std::string originalPath;
        bool bIsCommonPNGorJPGFile = false;
    };
    std::shared_ptr<Texture> exportTexture = std::make_shared<Texture>();
    std::vector<uint8_t> data;
    data.resize(image.decoded_size);
    memcpy(data.data(), image.decoded, data.size());
    exportTexture->decoded = data;
    exportTexture->width = image.width;
    exportTexture->height = image.height;
    exportTexture->numMips = image.num_mips;
    exportTexture->bEnableCompress = false;
    if ((image.source.type == ImageDecoder::EImageFormat::PNG || image.source.type == ImageDecoder::EImageFormat::JPEG) && image.source.rgb_format == ImageDecoder::ERGBFormat::RGBA && image.source.bit_depth == 8) {
        exportTexture->bIsCommonPNGorJPGFile = true;
        exportTexture->bEnableCompress = true;
        exportTexture->pixelFormat = Texture::EPixelFormat::RGBA8;
    } else {
        if (image.texture_format == ImageDecoder::ETextureSourceFormat::RGBA8 || image.texture_format == ImageDecoder::ETextureSourceFormat::RGBA16F) {
            if (image.texture_format == ImageDecoder::ETextureSourceFormat::RGBA8) {
                exportTexture->pixelFormat = Texture::EPixelFormat::RGBA8;
            } else {
                exportTexture->pixelFormat = Texture::EPixelFormat::RGBA16F;
            }
        } else if (image.texture_format == ImageDecoder::ETextureSourceFormat::RGBA16) {
            exportTexture = nullptr;
        } else if (image.texture_format == ImageDecoder::ETextureSourceFormat::G8) {
            std::vector<uint8_t> data;
            data.resize(image.decoded_size * 4);
            for (int i = 0; i < data.size() / 4; i++) {
                char val = image.decoded[i];
                data[4 * i] = data[4 * i + 1] = data[4 * i + 2] = val;
                data[4 * i + 3] = 255;
            }
            exportTexture->decoded = data;
            exportTexture->pixelFormat = Texture::EPixelFormat::RGBA8;
        } else if (image.texture_format == ImageDecoder::ETextureSourceFormat::BGRA8) {
            std::vector<uint8_t> data;
            data.resize(image.decoded_size);
            memcpy(data.data(), image.decoded, data.size());
            for (int i = 0; i < data.size() / 4; i++) {
                char tmp = data[4 * i];
                data[4 * i] = data[4 * i + 2];
                data[4 * i + 2] = tmp;
            }
            exportTexture->decoded = data;
            exportTexture->pixelFormat = Texture::EPixelFormat::RGBA8;
        } else {
            exportTexture = nullptr;
        }
    }
    if (!exportTexture || exportTexture->pixelFormat != Texture::EPixelFormat::RGBA8) {
        return -1;
    }

    GLFWContext::DeviceSettings deviceSettings;
#ifdef _WIN32
    deviceSettings.contextMajorVersion = 3;
    deviceSettings.contextMinorVersion = 3;
#elif __APPLE__
    deviceSettings.contextMajorVersion = 3;
    deviceSettings.contextMinorVersion = 2;
#endif

    GLFWContext::WindowSettings windowSettings;
    windowSettings.title = "Image App";
    windowSettings.width = exportTexture->width ? exportTexture->width : 1920;
    windowSettings.height = exportTexture->height ? exportTexture->height : 1080;
    windowSettings.focused = false;
    windowSettings.floating = false;
    windowSettings.decorated = true;
    windowSettings.cursorMode = GLFWContext::Cursor::ECursorMode::NORMAL;
    windowSettings.cursorShape = GLFWContext::Cursor::ECursorShape::ARROW;
    auto glfwContext = std::make_shared<GLFWContext::Context>(windowSettings, deviceSettings);
    glfwContext->window->MakeCurrentContext(true);
    glfwContext->window->FramebufferResizeEvent += [](uint16_t width, uint16_t height) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        glViewport(0, 0, width, height);
    };
    glfwContext->device->SetVsync(true);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top right
        1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
        -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3   // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);  // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, exportTexture->width, exportTexture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, exportTexture->decoded.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    const std::string vs =
        "#version 330 core\n\
layout (location = 0) in vec3 aPos;\n\
layout (location = 1) in vec3 aColor;\n\
layout (location = 2) in vec2 aTexCoord;\n\
\n\
out vec3 ourColor;\n\
out vec2 TexCoord;\n\
\n\
void main()\n\
{\n\
	gl_Position = vec4(aPos, 1.0);\n\
	ourColor = aColor;\n\
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n\
}";

    const std::string fs =
        "#version 330 core\n\
out vec4 FragColor;\n\
\n\
in vec3 ourColor;\n\
in vec2 TexCoord;\n\
\n\
// texture sampler\n\
uniform sampler2D texture1;\n\
\n\
void main()\n\
{\n\
	vec4 color = texture(texture1, TexCoord);\n\
	float grey = (color.x + color.y + color.z) / 3.0;\n\
	FragColor = vec4(grey, grey, grey, 1.0);\n\
	FragColor = color;\n\
	// FragColor = vec4(vec3(1.0 - color.x, 1.0 - color.y, 1.0 - color.z) * (0.7 + 0.3 * sin(TexCoord.x * 5 * 3.1415926)), 1.0);\n\
}";
    const char* vShaderCode = vs.c_str();
    const char* fShaderCode = fs.c_str();
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    auto program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    const auto frameStartTime = glfwContext->device->GetElapsedTime();
    auto prevFrameTime = frameStartTime;
    int frames = 0;

    while (!glfwContext->window->ShouldClose()) {
        auto currentFrameTime = glfwContext->device->GetElapsedTime();
        if ((currentFrameTime - prevFrameTime) > 1.0 || frames == 0) {
            float fps = (float)(int)((double)frames / (currentFrameTime - prevFrameTime) * 10) / 10.f;
            glfwContext->window->SetTitle("Image App (" + std::to_string(fps) + " FPS)");
            prevFrameTime = currentFrameTime;
            frames = 0;
        }
        frames++;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // bind Texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // render container
        glUseProgram(program);
        auto location = glGetUniformLocation(program, "texture1");
        glUniform1i(location, 0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwContext->window->SwapBuffers();
        glfwContext->device->PollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    return 0;
}
