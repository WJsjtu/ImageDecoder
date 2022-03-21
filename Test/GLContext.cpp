#if defined(WIN32)
#include "windows.h"
#endif
#include "GLContext.h"
#include <GL/glew.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#if defined(WIN32)
#include <GLFW/glfw3native.h>
#endif
#include <codecvt>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <functional>
#include <sstream>

#ifdef WIN32
GLFWContext::Eventing::Event<GLFWContext::EDeviceError, std::wstring> GLFWContext::Device::ErrorEvent;
#else
GLFWContext::Eventing::Event<GLFWContext::EDeviceError, std::string> GLFWContext::Device::ErrorEvent;
#endif

GLFWContext::Device::Device(const DeviceSettings& p_deviceSettings) {
    BindErrorCallback();

    int initializationCode = glfwInit();

    if (initializationCode == GLFW_FALSE) {
        throw std::runtime_error("Failed to Init GLFW");
        glfwTerminate();
    } else {
        CreateCursors();
#ifdef WIN32
        if (p_deviceSettings.debugProfile) glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, p_deviceSettings.contextMajorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, p_deviceSettings.contextMinorVersion);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, p_deviceSettings.samples);

        m_isAlive = true;
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, p_deviceSettings.samples);
#endif
    }
}

GLFWContext::Device::~Device() {
    if (m_isAlive) {
        DestroyCursors();
        glfwTerminate();
    }
}

std::pair<int16_t, int16_t> GLFWContext::Device::GetMonitorSize() const {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    return std::pair<int16_t, int16_t>(static_cast<int16_t>(mode->width), static_cast<int16_t>(mode->height));
}

GLFWcursor* GLFWContext::Device::GetCursorInstance(Cursor::ECursorShape p_cursorShape) const { return m_cursors.at(p_cursorShape); }

bool GLFWContext::Device::HasVsync() const { return m_vsync; }

void GLFWContext::Device::SetVsync(bool p_value) {
    glfwSwapInterval(p_value ? 1 : 0);
    m_vsync = p_value;
}

void GLFWContext::Device::PollEvents() const { glfwPollEvents(); }

float GLFWContext::Device::GetElapsedTime() const { return static_cast<float>(glfwGetTime()); }

void GLFWContext::Device::BindErrorCallback() {
    auto errorCallback = [](int p_code, const char* p_description) {
#ifdef WIN32
        WCHAR buffer[1024] = L"";
        MultiByteToWideChar(CP_UTF8, 0, p_description, -1, buffer, sizeof(buffer) / 2);
        std::wstring message(buffer);
        ErrorEvent.Invoke(static_cast<EDeviceError>(p_code), message);
#else
        ErrorEvent.Invoke(static_cast<EDeviceError>(p_code), p_description);
#endif
    };

    glfwSetErrorCallback(errorCallback);
}

void GLFWContext::Device::CreateCursors() {
    m_cursors[Cursor::ECursorShape::ARROW] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::ARROW));
    m_cursors[Cursor::ECursorShape::IBEAM] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::IBEAM));
    m_cursors[Cursor::ECursorShape::CROSSHAIR] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::CROSSHAIR));
    m_cursors[Cursor::ECursorShape::HAND] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::HAND));
    m_cursors[Cursor::ECursorShape::HRESIZE] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::HRESIZE));
    m_cursors[Cursor::ECursorShape::VRESIZE] = glfwCreateStandardCursor(static_cast<int>(Cursor::ECursorShape::VRESIZE));
}

void GLFWContext::Device::DestroyCursors() {
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::ARROW]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::IBEAM]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::CROSSHAIR]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::HAND]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::HRESIZE]);
    glfwDestroyCursor(m_cursors[Cursor::ECursorShape::VRESIZE]);
}

GLFWContext::Driver::Driver(const DriverSettings& p_driverSettings) {
    InitGlew();
    m_isActive = true;

    if (p_driverSettings.debugMode) {
        GLint flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(GLDebugMessageCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
}

bool GLFWContext::Driver::IsActive() const { return m_isActive; }

void GLFWContext::Driver::GLDebugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userParam) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::string output;

    output += "OpenGL Debug Message:\n";
    output += "Debug message (" + std::to_string(id) + "): " + message + "\n";

    switch (source) {
        case GL_DEBUG_SOURCE_API: output += "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: output += "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: output += "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: output += "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: output += "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER: output += "Source: Other"; break;
    }

    output += "\n";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR: output += "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: output += "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: output += "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY: output += "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: output += "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER: output += "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: output += "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: output += "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER: output += "Type: Other"; break;
    }

    output += "\n";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: output += "Severity: High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: output += "Severity: Medium"; break;
        case GL_DEBUG_SEVERITY_LOW: output += "Severity: Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: output += "Severity: Notification"; break;
    }

    output += "\n";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: std::cerr << "Error: " << output << std::endl; break;
        case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "Warn: " << output << std::endl; break;
        case GL_DEBUG_SEVERITY_LOW: std::cout << "Info: " << output << std::endl; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cerr << "Debug: " << output << std::endl; break;
    }
}

void GLFWContext::Driver::InitGlew() {
    const GLenum error = glewInit();
    if (error != GLEW_OK) {
        std::string message = "Error Init GLEW: ";
        std::string glewError = reinterpret_cast<const char*>(glewGetErrorString(error));
        std::cerr << glewError << std::endl;
    }
}

std::unordered_map<GLFWwindow*, GLFWContext::Window*> GLFWContext::Window::__WINDOWS_MAP;

GLFWContext::Window::Window(const std::shared_ptr<Device>& p_device, const WindowSettings& p_windowSettings)
    : m_device(p_device),
      m_title(p_windowSettings.title),
      m_size{p_windowSettings.width, p_windowSettings.height},
      m_minimumSize{p_windowSettings.minimumWidth, p_windowSettings.minimumHeight},
      m_maximumSize{p_windowSettings.maximumWidth, p_windowSettings.maximumHeight},
      m_fullscreen(p_windowSettings.fullscreen),
      m_refreshRate(p_windowSettings.refreshRate),
      m_cursorMode(p_windowSettings.cursorMode),
      m_cursorShape(p_windowSettings.cursorShape) {
    /* Window creation */
    CreateGlfwWindow(p_windowSettings);

    /* Window settings */
    SetCursorMode(p_windowSettings.cursorMode);
    SetCursorShape(p_windowSettings.cursorShape);

    /* Callback binding */
    BindKeyCallback();
    BindMouseCallback();
    BindIconifyCallback();
    BindCloseCallback();
    BindResizeCallback();
    BindCursorMoveCallback();
    BindFramebufferResizeCallback();
    BindMoveCallback();
    BindFocusCallback();

    /* Event listening */
    std::function<void(uint16_t, uint16_t)> onResize = std::bind(&Window::OnResize, this, std::placeholders::_1, std::placeholders::_2);
    std::function<void(uint16_t, uint16_t)> onMove = std::bind(&Window::OnMove, this, std::placeholders::_1, std::placeholders::_2);
    ResizeEvent.AddListener(onResize);
    MoveEvent.AddListener(onMove);
}

GLFWContext::Window::~Window() { glfwDestroyWindow(m_glfwWindow); }

void GLFWContext::Window::SetIconFromMemory(uint8_t* p_data, uint32_t p_width, uint32_t p_height) {
    GLFWimage images[1];
    images[0].pixels = p_data;
    images[0].height = p_width;
    images[0].width = p_height;
    glfwSetWindowIcon(m_glfwWindow, 1, images);
}

GLFWContext::Window* GLFWContext::Window::FindInstance(GLFWwindow* p_glfwWindow) { return __WINDOWS_MAP.find(p_glfwWindow) != __WINDOWS_MAP.end() ? __WINDOWS_MAP[p_glfwWindow] : nullptr; }

void GLFWContext::Window::SetSize(uint16_t p_width, uint16_t p_height) { glfwSetWindowSize(m_glfwWindow, static_cast<int>(p_width), static_cast<int>(p_height)); }

void GLFWContext::Window::SetMinimumSize(int16_t p_minimumWidth, int16_t p_minimumHeight) {
    m_minimumSize.first = p_minimumWidth;
    m_minimumSize.second = p_minimumHeight;

    UpdateSizeLimit();
}

void GLFWContext::Window::SetMaximumSize(int16_t p_maximumWidth, int16_t p_maximumHeight) {
    m_maximumSize.first = p_maximumWidth;
    m_maximumSize.second = p_maximumHeight;

    UpdateSizeLimit();
}

void GLFWContext::Window::SetPosition(int16_t p_x, int16_t p_y) { glfwSetWindowPos(m_glfwWindow, static_cast<int>(p_x), static_cast<int>(p_y)); }

void GLFWContext::Window::Minimize() const { glfwIconifyWindow(m_glfwWindow); }

void GLFWContext::Window::Maximize() const { glfwMaximizeWindow(m_glfwWindow); }

void GLFWContext::Window::Restore() const { glfwRestoreWindow(m_glfwWindow); }

void GLFWContext::Window::Hide() const { glfwHideWindow(m_glfwWindow); }

void GLFWContext::Window::Show() const { glfwShowWindow(m_glfwWindow); }

void GLFWContext::Window::Focus() const { glfwFocusWindow(m_glfwWindow); }

void GLFWContext::Window::SetShouldClose(bool p_value) const { glfwSetWindowShouldClose(m_glfwWindow, p_value); }

bool GLFWContext::Window::ShouldClose() const { return glfwWindowShouldClose(m_glfwWindow); }

void GLFWContext::Window::SetFullscreen(bool p_value) {
    if (p_value) m_fullscreen = true;

    glfwSetWindowMonitor(m_glfwWindow, p_value ? glfwGetPrimaryMonitor() : nullptr, static_cast<int>(m_position.first), static_cast<int>(m_position.second), static_cast<int>(m_size.first), static_cast<int>(m_size.second), m_refreshRate);

    if (!p_value) m_fullscreen = false;
}

void GLFWContext::Window::ToggleFullscreen() { SetFullscreen(!m_fullscreen); }

bool GLFWContext::Window::IsFullscreen() const { return m_fullscreen; }

bool GLFWContext::Window::IsHidden() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_VISIBLE) == GLFW_FALSE; }

bool GLFWContext::Window::IsVisible() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_VISIBLE) == GLFW_TRUE; }

bool GLFWContext::Window::IsMaximized() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE; }

bool GLFWContext::Window::IsMinimized() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_MAXIMIZED) == GLFW_FALSE; }

bool GLFWContext::Window::IsFocused() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_FOCUSED) == GLFW_TRUE; }

bool GLFWContext::Window::IsResizable() const { return glfwGetWindowAttrib(m_glfwWindow, GLFW_RESIZABLE) == GLFW_TRUE; }

bool GLFWContext::Window::IsDecorated() const {
    return glfwGetWindowAttrib(m_glfwWindow, GLFW_DECORATED) == GLFW_TRUE;
    ;
}

void GLFWContext::Window::MakeCurrentContext(bool active) { glfwMakeContextCurrent(active ? m_glfwWindow : NULL); }

void GLFWContext::Window::SwapBuffers() const { glfwSwapBuffers(m_glfwWindow); }

void GLFWContext::Window::SetCursorMode(Cursor::ECursorMode p_cursorMode) {
    m_cursorMode = p_cursorMode;
    glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, static_cast<int>(p_cursorMode));
}

void GLFWContext::Window::SetCursorShape(Cursor::ECursorShape p_cursorShape) {
    m_cursorShape = p_cursorShape;
    glfwSetCursor(m_glfwWindow, m_device->GetCursorInstance(p_cursorShape));
}

void GLFWContext::Window::SetCursorPosition(int16_t p_x, int16_t p_y) { glfwSetCursorPos(m_glfwWindow, static_cast<double>(p_x), static_cast<double>(p_y)); }

void GLFWContext::Window::SetTitle(const std::string& p_title) {
    m_title = p_title;
    glfwSetWindowTitle(m_glfwWindow, p_title.c_str());
}

void GLFWContext::Window::SetRefreshRate(int32_t p_refreshRate) { m_refreshRate = p_refreshRate; }

std::string GLFWContext::Window::GetTitle() const { return m_title; }

std::pair<uint16_t, uint16_t> GLFWContext::Window::GetSize() const {
    int width, height;
    glfwGetWindowSize(m_glfwWindow, &width, &height);
    return std::make_pair(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
}

std::pair<int16_t, int16_t> GLFWContext::Window::GetMinimumSize() const { return m_minimumSize; }

std::pair<int16_t, int16_t> GLFWContext::Window::GetMaximumSize() const { return m_maximumSize; }

std::pair<int16_t, int16_t> GLFWContext::Window::GetPosition() const {
    int x, y;
    glfwGetWindowPos(m_glfwWindow, &x, &y);
    return std::make_pair(static_cast<int16_t>(x), static_cast<int16_t>(y));
}

std::pair<uint16_t, uint16_t> GLFWContext::Window::GetFramebufferSize() const {
    int width, height;
    glfwGetFramebufferSize(m_glfwWindow, &width, &height);
    return std::make_pair(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
}

GLFWContext::Cursor::ECursorMode GLFWContext::Window::GetCursorMode() const { return m_cursorMode; }

GLFWContext::Cursor::ECursorShape GLFWContext::Window::GetCursorShape() const { return m_cursorShape; }

int32_t GLFWContext::Window::GetRefreshRate() const { return m_refreshRate; }

GLFWwindow* GLFWContext::Window::GetGlfwWindow() const { return m_glfwWindow; }

void GLFWContext::Window::CreateGlfwWindow(const WindowSettings& p_windowSettings) {
    GLFWmonitor* selectedMonitor = nullptr;

    if (m_fullscreen) selectedMonitor = glfwGetPrimaryMonitor();

    glfwWindowHint(GLFW_RESIZABLE, p_windowSettings.resizable);
    glfwWindowHint(GLFW_DECORATED, p_windowSettings.decorated);
    glfwWindowHint(GLFW_FOCUSED, p_windowSettings.focused);
    glfwWindowHint(GLFW_MAXIMIZED, p_windowSettings.maximized);
    glfwWindowHint(GLFW_FLOATING, p_windowSettings.floating);
    glfwWindowHint(GLFW_VISIBLE, p_windowSettings.visible);
    glfwWindowHint(GLFW_AUTO_ICONIFY, p_windowSettings.autoIconify);
    glfwWindowHint(GLFW_REFRESH_RATE, p_windowSettings.refreshRate);
    glfwWindowHint(GLFW_SAMPLES, p_windowSettings.samples);

    m_glfwWindow = glfwCreateWindow(static_cast<int>(m_size.first), static_cast<int>(m_size.second), m_title.c_str(), selectedMonitor, nullptr);

    if (!m_glfwWindow) {
        throw std::runtime_error("Failed to create GLFW window");
    } else {
        UpdateSizeLimit();

        std::pair<int16_t, int16_t> pair = GetPosition();
        m_position.first = pair.first;
        m_position.second = pair.second;

        __WINDOWS_MAP[m_glfwWindow] = this;
    }
}

void GLFWContext::Window::BindKeyCallback() const {
    auto keyCallback = [](GLFWwindow* p_window, int p_key, int p_scancode, int p_action, int p_mods) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            if (p_action == GLFW_PRESS) windowInstance->KeyPressedEvent.Invoke(p_key);

            if (p_action == GLFW_RELEASE) windowInstance->KeyReleasedEvent.Invoke(p_key);
        }
    };

    glfwSetKeyCallback(m_glfwWindow, keyCallback);
}

void GLFWContext::Window::BindMouseCallback() const {
    auto mouseCallback = [](GLFWwindow* p_window, int p_button, int p_action, int p_mods) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            if (p_action == GLFW_PRESS) windowInstance->MouseButtonPressedEvent.Invoke(p_button);

            if (p_action == GLFW_RELEASE) windowInstance->MouseButtonReleasedEvent.Invoke(p_button);
        }
    };

    glfwSetMouseButtonCallback(m_glfwWindow, mouseCallback);
}

void GLFWContext::Window::BindResizeCallback() const {
    auto resizeCallback = [](GLFWwindow* p_window, int p_width, int p_height) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            windowInstance->ResizeEvent.Invoke(static_cast<uint16_t>(p_width), static_cast<uint16_t>(p_height));
        }
    };

    glfwSetWindowSizeCallback(m_glfwWindow, resizeCallback);
}

void GLFWContext::Window::BindFramebufferResizeCallback() const {
    auto framebufferResizeCallback = [](GLFWwindow* p_window, int p_width, int p_height) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            windowInstance->FramebufferResizeEvent.Invoke(static_cast<uint16_t>(p_width), static_cast<uint16_t>(p_height));
        }
    };

    glfwSetFramebufferSizeCallback(m_glfwWindow, framebufferResizeCallback);
}

void GLFWContext::Window::BindCursorMoveCallback() const {
    auto cursorMoveCallback = [](GLFWwindow* p_window, double p_x, double p_y) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            windowInstance->CursorMoveEvent.Invoke(static_cast<int16_t>(p_x), static_cast<int16_t>(p_y));
        }
    };

    glfwSetCursorPosCallback(m_glfwWindow, cursorMoveCallback);
}

void GLFWContext::Window::BindMoveCallback() const {
    auto moveCallback = [](GLFWwindow* p_window, int p_x, int p_y) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            windowInstance->MoveEvent.Invoke(static_cast<int16_t>(p_x), static_cast<int16_t>(p_y));
        }
    };

    glfwSetWindowPosCallback(m_glfwWindow, moveCallback);
}

void GLFWContext::Window::BindIconifyCallback() const {
    auto iconifyCallback = [](GLFWwindow* p_window, int p_iconified) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            if (p_iconified == GLFW_TRUE) windowInstance->MinimizeEvent.Invoke();

            if (p_iconified == GLFW_FALSE) windowInstance->MaximizeEvent.Invoke();
        }
    };

    glfwSetWindowIconifyCallback(m_glfwWindow, iconifyCallback);
}

void GLFWContext::Window::BindFocusCallback() const {
    auto focusCallback = [](GLFWwindow* p_window, int p_focused) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            if (p_focused == GLFW_TRUE) windowInstance->GainFocusEvent.Invoke();

            if (p_focused == GLFW_FALSE) windowInstance->LostFocusEvent.Invoke();
        }
    };

    glfwSetWindowFocusCallback(m_glfwWindow, focusCallback);
}

void GLFWContext::Window::BindCloseCallback() const {
    auto closeCallback = [](GLFWwindow* p_window) {
        Window* windowInstance = FindInstance(p_window);

        if (windowInstance) {
            windowInstance->CloseEvent.Invoke();
        }
    };

    glfwSetWindowCloseCallback(m_glfwWindow, closeCallback);
}

void GLFWContext::Window::OnResize(uint16_t p_width, uint16_t p_height) {
    m_size.first = p_width;
    m_size.second = p_height;
}

void GLFWContext::Window::OnMove(int16_t p_x, int16_t p_y) {
    if (!m_fullscreen) {
        m_position.first = p_x;
        m_position.second = p_y;
    }
}

void GLFWContext::Window::UpdateSizeLimit() const { glfwSetWindowSizeLimits(m_glfwWindow, static_cast<int>(m_minimumSize.first), static_cast<int>(m_minimumSize.second), static_cast<int>(m_maximumSize.first), static_cast<int>(m_maximumSize.second)); }

std::string ws2s(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

GLFWContext::Context::Context(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings) {
    /* Window creation */
    device = std::make_shared<Device>(deviceSettings);
    window = std::make_shared<Window>(device, windowSettings);
#ifdef WIN32
    std::function<void(EDeviceError, std::wstring message)> onError = [](EDeviceError, std::wstring message) {};
#else
    std::function<void(EDeviceError, std::string message)> onError = [](EDeviceError, std::string message) {};
#endif
    errorListenerID = Device::ErrorEvent.AddListener(onError);

    window->MakeCurrentContext(true);
    device->SetVsync(true);
    driver = std::make_shared<Driver>(DriverSettings());
    glEnable(GL_MULTISAMPLE);
    window->MakeCurrentContext(false);
}

GLFWContext::Context::~Context() { Device::ErrorEvent.RemoveListener(errorListenerID); }

#if defined(WIN32)
template <class T>
class GLWFWinSignal {
public:
    void Signal(T t) {
        std::unique_lock<std::mutex> lock(_mutex);
        m_initlaized = true;
        m_val = t;
        _cv.notify_one();
    }

    const T Wait() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [=] { return m_initlaized; });
        return m_val;
    }

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    bool m_initlaized = false;
    T m_val;
};
std::mutex GLVersionMutex;
std::string GLVersion = "";
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                         1,
                                         PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,  // Flags
                                         PFD_TYPE_RGBA,                                               // The kind of framebuffer. RGBA or palette.
                                         32,                                                          // Colordepth of the framebuffer.
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         24,  // Number of bits for the depthbuffer
                                         8,   // Number of bits for the stencilbuffer
                                         0,   // Number of Aux buffers in the framebuffer.
                                         PFD_MAIN_PLANE,
                                         0,
                                         0,
                                         0,
                                         0};

            HDC ourWindowHandleToDeviceContext = GetDC(hWnd);

            int letWindowsChooseThisPixelFormat;
            letWindowsChooseThisPixelFormat = ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);
            SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

            HGLRC ourOpenGLRenderingContext = wglCreateContext(ourWindowHandleToDeviceContext);
            wglMakeCurrent(ourWindowHandleToDeviceContext, ourOpenGLRenderingContext);

            GLVersion = (char*)glGetString(GL_VERSION);

            wglDeleteContext(ourOpenGLRenderingContext);
            PostQuitMessage(0);
        } break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

std::string GLFWContext::GetGLVersion() {
    if (GLVersion != "") {
        return GLVersion;
    }
    std::lock_guard<std::mutex> lk(GLVersionMutex);
    GLWFWinSignal<bool> signal;
    std::thread thread = std::thread([&]() {
        MSG msg = {0};
        WNDCLASSA wc = {0};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = "openglversioncheck";
        wc.style = CS_OWNDC;
        if (!RegisterClassA(&wc)) {
            signal.Signal(false);
        } else {
            auto winhdw = CreateWindowA(wc.lpszClassName, "openglversioncheck", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, NULL, 0);
            if (!winhdw) {
                signal.Signal(false);
            } else {
                while (GetMessage(&msg, NULL, 0, 0) > 0) {
                    DispatchMessage(&msg);
                }
                DestroyWindow(winhdw);
                signal.Signal(true);
            }
        }
    });
    bool success = signal.Wait();
    thread.join();
    return success ? GLVersion : "";
}

GLFWContext::ThreadedContext::ThreadedContext(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings) {
    GLWFWinSignal<std::shared_ptr<GLFWContext::Context>> glfwContextSignal;

    std::string error = "";
    thread = std::thread([&]() {
        std::shared_ptr<GLFWContext::Context> context = nullptr;
        try {
            context = std::make_shared<GLFWContext::Context>(windowSettings, deviceSettings);
            glfwContextSignal.Signal(context);
        } catch (const std::runtime_error& err) {
            error = err.what();
            glfwContextSignal.Signal(nullptr);
        } catch (...) {
            error = "Failed to create threaded GLFW window";
            glfwContextSignal.Signal(nullptr);
        }

        if (context) {
            while (true) {
                bool shouldCLoseLocal = false;
                {
                    std::lock_guard<std::mutex> lk(mutex);
                    shouldCLoseLocal = shouldCLose;
                }
                if (shouldCLoseLocal) {
                    break;
                }
                context->device->PollEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    });

    context = glfwContextSignal.Wait();

    if (context) {
        window = context->window;
        device = context->device;
        driver = context->driver;
    } else {
        {
            std::lock_guard<std::mutex> lk(mutex);
            shouldCLose = true;
        }
        thread.join();
        throw std::runtime_error(error);
    }
}

int GLFWContext::ThreadedContext::GetGLFWThreadID() {
    std::ostringstream ss;
    ss << thread.get_id();
    return std::stoi(ss.str());
}

intptr_t GLFWContext::ThreadedContext::GetNativeWindowHandler() {
    HWND handle = glfwGetWin32Window(context->window->GetGlfwWindow());
    return reinterpret_cast<intptr_t>(handle);
}

GLFWContext::ThreadedContext::~ThreadedContext() {
    {
        std::lock_guard<std::mutex> lk(mutex);
        shouldCLose = true;
    }
    thread.join();
    context = nullptr;
}

#endif
