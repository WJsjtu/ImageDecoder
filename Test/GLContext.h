#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <stdint.h>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

struct GLFWwindow;
struct GLFWcursor;

namespace GLFWContext {

namespace Cursor {
/**
 * Some cursor modes.
 * They defines if the mouse pointer should be visible, locked or normal
 */
enum class ECursorMode { NORMAL = 0x00034001, DISABLED = 0x00034003, HIDDEN = 0x00034002 };

/**
 * Some cursor shapes.
 * They specify how the mouse pointer should look
 */
enum class ECursorShape { ARROW = 0x00036001, IBEAM = 0x00036002, CROSSHAIR = 0x00036003, HAND = 0x00036004, HRESIZE = 0x00036005, VRESIZE = 0x00036006 };
}  // namespace Cursor

namespace Eventing {
/**
 * The ID of a listener (Registered callback).
 * This value is needed to remove a listener from an event
 */
using ListenerID = uint64_t;

/**
 * A simple event that contains a set of function callbacks. These functions will be called on invoke
 */
template <class... ArgTypes>
class Event {
public:
    /**
     * Simple shortcut for a generic function without return value
     */
    using Callback = std::function<void(ArgTypes...)>;

    /**
     * Add a function callback to this event
     * Also return the ID of the new listener (You should store the returned ID if you want to remove the listener
     * later)
     * @param p_call
     */
    ListenerID AddListener(Callback p_callback);

    /**
     * Add a function callback to this event
     * Also return the ID of the new listener (You should store the returned ID if you want to remove the listener
     * later)
     * @param p_call
     */
    ListenerID operator+=(Callback p_callback);

    /**
     * Remove a function callback to this event using a Listener (Created when calling AddListener)
     * @param p_listener
     */
    bool RemoveListener(ListenerID p_listenerID);

    /**
     * Remove a function callback to this event using a Listener (Created when calling AddListener)
     * @param p_listener
     */
    bool operator-=(ListenerID p_listenerID);

    /**
     * Remove every listeners to this event
     */
    void RemoveAllListeners();

    /**
     * Return the number of callback registered
     */
    uint64_t GetListenerCount();

    /**
     * Call every callbacks attached to this event
     * @param p_args (Variadic)
     */
    void Invoke(ArgTypes... p_args);

private:
    std::unordered_map<ListenerID, Callback> m_callbacks;
    ListenerID m_availableListenerID = 0;
};

template <class... ArgTypes>
ListenerID Event<ArgTypes...>::AddListener(Callback p_callback) {
    ListenerID listenerID = m_availableListenerID++;
    m_callbacks.emplace(listenerID, p_callback);
    return listenerID;
}

template <class... ArgTypes>
ListenerID Event<ArgTypes...>::operator+=(Callback p_callback) {
    return AddListener(p_callback);
}

template <class... ArgTypes>
bool Event<ArgTypes...>::RemoveListener(ListenerID p_listenerID) {
    return m_callbacks.erase(p_listenerID) != 0;
}

template <class... ArgTypes>
bool Event<ArgTypes...>::operator-=(ListenerID p_listenerID) {
    return RemoveListener(p_listenerID);
}

template <class... ArgTypes>
void Event<ArgTypes...>::RemoveAllListeners() {
    m_callbacks.clear();
}

template <class... ArgTypes>
uint64_t Event<ArgTypes...>::GetListenerCount() {
    return m_callbacks.size();
}

template <class... ArgTypes>
void Event<ArgTypes...>::Invoke(ArgTypes... p_args) {
    for (auto const& pair : m_callbacks) pair.second(p_args...);
}

}  // namespace Eventing

/**
 * Contains device settings
 */
struct DeviceSettings {
    DeviceSettings() = default;

    DeviceSettings(const DeviceSettings&) = default;

    DeviceSettings& operator=(DeviceSettings&) = default;

    /**
     * specifies whether to create a debug OpenGL context, which may have additional error and
     * performance issue reporting functionality. If OpenGL ES is requested, this hint is ignored
     */
#ifdef _DEBUG
    bool debugProfile = true;
#else
    bool debugProfile = false;
#endif

    /**
     * Specifies whether the OpenGL context should be forward-compatible, i.e. one where all functionality
     * deprecated in the requested version of OpenGL is removed. This must only be used if the requested OpenGL
     * version is 3.0 or above. If OpenGL ES is requested, this hint is ignored.
     */
    bool forwardCompatibility = false;

    /**
     * Specify the client API major version that the created context must be compatible with. The exact
     * behavior of these hints depend on the requested client API
     */
    uint8_t contextMajorVersion = 3;

    /**
     * Specify the client API minor version that the created context must be compatible with. The exact
     * behavior of these hints depend on the requested client API
     */
    uint8_t contextMinorVersion = 2;

    /**
     * Defines the amount of samples to use (Requiered for multi-sampling)
     */
    uint8_t samples = 4;
};

/**
 * Some errors that the driver can return
 */
enum class EDeviceError {
    NOT_INITIALIZED = 0x00010001,
    NO_CURRENT_CONTEXT = 0x00010002,
    INVALID_ENUM = 0x00010003,
    INVALID_VALUE = 0x00010004,
    OUT_OF_MEMORY = 0x00010005,
    API_UNAVAILABLE = 0x00010006,
    VERSION_UNAVAILABLE = 0x00010007,
    PLATFORM_ERROR = 0x00010008,
    FORMAT_UNAVAILABLE = 0x00010009,
    NO_WINDOW_CONTEXT = 0x0001000A
};

class Context;

/**
 * The Device represents the windowing context. It is necessary to create a device
 * to create a window
 */
class Device {
public:
    /**
     * The constructor of the device will take care about GLFW initialization
     */
    Device(const DeviceSettings& p_deviceSettings);

    Device(const Device&) = delete;

    Device& operator=(Device&) = delete;

    /**
     * The destructor of the device will take care about GLFW destruction
     */
    ~Device();

    /**
     * Return the size, in pixels, of the primary monity
     */
    std::pair<int16_t, int16_t> GetMonitorSize() const;

    /**
     * Return an instance of GLFWcursor corresponding to the given shape
     * @param p_cursorShape
     */
    GLFWcursor* GetCursorInstance(Cursor::ECursorShape p_cursorShape) const;

    /**
     * Return true if the vsync is currently enabled
     */
    bool HasVsync() const;

    /**
     * Enable or disable the vsync
     * @note You must call this method after creating and defining a window as the current context
     * @param p_value (True to enable vsync)
     */
    void SetVsync(bool p_value);

    /**
     * Enable the inputs and events managments with created windows
     * @note Should be called every frames
     */
    void PollEvents() const;

    /**
     * Returns the elapsed time since the device startup
     */
    float GetElapsedTime() const;

private:
    void BindErrorCallback();
    void CreateCursors();
    void DestroyCursors();

private:
    /**
     * Bind a listener to this event to receive device errors
     */
#ifdef WIN32
    static Eventing::Event<EDeviceError, std::wstring> ErrorEvent;
#else
    static Eventing::Event<EDeviceError, std::string> ErrorEvent;
#endif

    bool m_vsync = true;
    bool m_isAlive = false;
    std::unordered_map<Cursor::ECursorShape, GLFWcursor*> m_cursors;

    friend class Context;
};

struct DriverSettings {
#ifdef _DEBUG
    bool debugMode = true;
#else
    bool debugMode = false;
#endif  // DEBUG
};

/**
 * The Driver represents the OpenGL context
 */
class Driver {
public:
    /**
     * Creates the Driver (OpenGL context)
     * @param p_driverSettings
     */
    Driver(const DriverSettings& p_driverSettings);

    Driver(const Driver&) = delete;

    Driver& operator=(Driver&) = delete;

    /**
     * Destroy the driver
     */
    ~Driver() = default;

    /**
     * Returns true if the OpenGL context is active
     */
    bool IsActive() const;

private:
    void InitGlew();
#ifdef WIN32
    static void __stdcall GLDebugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userParam);
#else
    static void GLDebugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userParam);
#endif

private:
    bool m_isActive;
};

/**
 * Contains window settings
 */
struct WindowSettings {
    WindowSettings() = default;

    WindowSettings(const WindowSettings&) = default;

    WindowSettings& operator=(WindowSettings&) = default;

    /**
     * A simple constant used to ignore a value setting (Let the program decide for you)
     * @note You can you WindowSettings::DontCare only where it is indicated
     */
    static const int32_t DontCare = -1;

    /**
     * Title of the window (Displayed in the title bar)
     */
    std::string title;

    /**
     * Width in pixels of the window
     */
    uint16_t width;

    /**
     * Height in pixels of the window
     */
    uint16_t height;

    /**
     * Minimum width of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t minimumWidth = DontCare;

    /**
     * Minimum height of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t minimumHeight = DontCare;

    /**
     * Maximum width of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t maximumWidth = DontCare;

    /**
     * Maximum height of the window.
     * Use WindowSettings::DontCare to disable limit
     */
    int16_t maximumHeight = DontCare;

    /**
     * Specifies if the window is by default in fullscreen or windowed mode
     */
    bool fullscreen = false;

    /**
     * Specifies whether the windowed mode window will have window decorations such as a border, a close widget, etc.
     * An undecorated window may still allow the user to generate close events on some platforms. This hint is ignored
     * for full screen windows.
     */
    bool decorated = true;

    /**
     * specifies whether the windowed mode window will be resizable by the user. The window will still be resizable
     * using the "SetSize(uint16_t, uint16_t)" method of the "Window" class. This hint is ignored for full screen
     * windows
     */
    bool resizable = true;

    /**
     * Specifies whether the windowed mode window will be given input focus when created. This hint is ignored for
     * full screen and initially hidden windows.
     */
    bool focused = true;

    /**
     * Specifies whether the windowed mode window will be maximized when created. This hint is ignored for full screen
     * windows.
     */
    bool maximized = false;

    /**
     * Specifies whether the windowed mode window will be floating above other regular windows, also called topmost or
     * always-on-top. This is intended primarily for debugging purposes and cannot be used to implement proper full
     * screen windows. This hint is ignored for full screen windows.
     */
    bool floating = false;

    /**
     * Specifies whether the windowed mode window will be initially visible. This hint is ignored for full screen
     * windows.
     */
    bool visible = true;

    /**
     * Specifies whether the full screen window will automatically iconify and restore
     * the previous video mode on input focus loss. This hint is ignored for windowed mode windows
     */
    bool autoIconify = true;

    /**
     * Specifies the desired refresh rate for full screen windows. If set to WindowSettings::DontCare, the highest
     * available refresh rate will be used. This hint is ignored for windowed mode windows.
     */
    int32_t refreshRate = WindowSettings::DontCare;

    /**
     * Specifies the default cursor mode of the window
     */
    Cursor::ECursorMode cursorMode = Cursor::ECursorMode::NORMAL;

    /**
     * Specifies the default cursor shape of the window
     */
    Cursor::ECursorShape cursorShape = Cursor::ECursorShape::ARROW;

    /**
     * Defines the number of samples to use (For anti-aliasing)
     */
    uint32_t samples = 4;
};

/**
 * A simple OS-based window.
 * It needs a Device (GLFW) to work
 */
class Window {
public:
    /* Inputs relatives */
    Eventing::Event<int> KeyPressedEvent;
    Eventing::Event<int> KeyReleasedEvent;
    Eventing::Event<int> MouseButtonPressedEvent;
    Eventing::Event<int> MouseButtonReleasedEvent;

    /* Window events */
    Eventing::Event<uint16_t, uint16_t> ResizeEvent;
    Eventing::Event<uint16_t, uint16_t> FramebufferResizeEvent;
    Eventing::Event<int16_t, int16_t> MoveEvent;
    Eventing::Event<int16_t, int16_t> CursorMoveEvent;
    Eventing::Event<> MinimizeEvent;
    Eventing::Event<> MaximizeEvent;
    Eventing::Event<> GainFocusEvent;
    Eventing::Event<> LostFocusEvent;
    Eventing::Event<> CloseEvent;

    /**
     * Create the window
     * @param p_device
     * @param p_windowSettings
     */
    Window(const std::shared_ptr<Device>& p_device, const WindowSettings& p_windowSettings);

    Window(const Window&) = delete;

    Window& operator=(Window&) = delete;

    /**
     * Destructor of the window, responsible of the GLFW window memory free
     */
    ~Window();

    /**
     * Set Icon from memory
     * @param p_data
     * @param p_width
     * @param p_height
     */
    void SetIconFromMemory(uint8_t* p_data, uint32_t p_width, uint32_t p_height);

    /**
     * Find an instance of window with a given GLFWwindow
     * @param p_glfwWindow
     */
    static Window* FindInstance(GLFWwindow* p_glfwWindow);

    /**
     * Resize the window
     * @param p_width
     * @param p_height
     */
    void SetSize(uint16_t p_width, uint16_t p_height);

    /**
     * Defines a minimum size for the window
     * @param p_minimumWidth
     * @param p_minimumHeight
     * @note -1 (WindowSettings::DontCare) value means no limitation
     */
    void SetMinimumSize(int16_t p_minimumWidth, int16_t p_minimumHeight);

    /**
     * Defines a maximum size for the window
     * @param p_maximumWidth
     * @param p_maximumHeight
     * @note -1 (WindowSettings::DontCare) value means no limitation
     */
    void SetMaximumSize(int16_t p_maximumWidth, int16_t p_maximumHeight);

    /**
     * Define a position for the window
     * @param p_x
     * @param p_y
     */
    void SetPosition(int16_t p_x, int16_t p_y);

    /**
     * Minimize the window
     */
    void Minimize() const;

    /**
     * Maximize the window
     */
    void Maximize() const;

    /**
     * Restore the window
     */
    void Restore() const;

    /**
     * Hides the specified window if it was previously visible
     */
    void Hide() const;

    /**
     * Show the specified window if it was previously hidden
     */
    void Show() const;

    /**
     * Focus the window
     */
    void Focus() const;

    /**
     * Set the should close flag of the window to true
     * @param p_value
     */
    void SetShouldClose(bool p_value) const;

    /**
     * Return true if the window should close
     */
    bool ShouldClose() const;

    /**
     * Set the window in fullscreen or windowed mode
     * @param p_value (True for fullscreen mode, false for windowed)
     */
    void SetFullscreen(bool p_value);

    /**
     * Switch the window to fullscreen or windowed mode depending
     * on the current state
     */
    void ToggleFullscreen();

    /**
     * Return true if the window is fullscreen
     */
    bool IsFullscreen() const;

    /**
     * Return true if the window is hidden
     */
    bool IsHidden() const;

    /**
     * Return true if the window is visible
     */
    bool IsVisible() const;

    /**
     * Return true if the windows is maximized
     */
    bool IsMaximized() const;

    /**
     * Return true if the windows is minimized
     */
    bool IsMinimized() const;

    /**
     * Return true if the windows is focused
     */
    bool IsFocused() const;

    /**
     * Return true if the windows is resizable
     */
    bool IsResizable() const;

    /**
     * Return true if the windows is decorated
     */
    bool IsDecorated() const;

    /**
     * Define the window as the current context
     */
    void MakeCurrentContext(bool active);

    /**
     * Handle the buffer swapping with the current window
     */
    void SwapBuffers() const;

    /**
     * Define a mode for the mouse cursor
     * @param p_cursorMode
     */
    void SetCursorMode(Cursor::ECursorMode p_cursorMode);

    /**
     * Define a shape to apply to the current cursor
     * @param p_cursorShape
     */
    void SetCursorShape(Cursor::ECursorShape p_cursorShape);

    /**
     * Move the cursor to the given position
     */
    void SetCursorPosition(int16_t p_x, int16_t p_y);

    /**
     * Define a title for the window
     * @param p_title
     */
    void SetTitle(const std::string& p_title);

    /**
     * Defines a refresh rate (Use WindowSettings::DontCare to use the highest available refresh rate)
     * @param p_refreshRate
     * @note You need to switch to fullscreen mode to apply this effect (Or leave fullscreen and re-apply)
     */
    void SetRefreshRate(int32_t p_refreshRate);

    /**
     * Return the title of the window
     */
    std::string GetTitle() const;

    /**
     * Return the current size of the window
     */
    std::pair<uint16_t, uint16_t> GetSize() const;

    /**
     * Return the current minimum size of the window
     * @note -1 (WindowSettings::DontCare) values means no limitation
     */
    std::pair<int16_t, int16_t> GetMinimumSize() const;

    /**
     * Return the current maximum size of the window
     * @note -1 (WindowSettings::DontCare) values means no limitation
     */
    std::pair<int16_t, int16_t> GetMaximumSize() const;

    /**
     * Return the current position of the window
     */
    std::pair<int16_t, int16_t> GetPosition() const;

    /**
     * Return the framebuffer size (Viewport size)
     */
    std::pair<uint16_t, uint16_t> GetFramebufferSize() const;

    /**
     * Return the current cursor mode
     */
    Cursor::ECursorMode GetCursorMode() const;

    /**
     * Return the current cursor shape
     */
    Cursor::ECursorShape GetCursorShape() const;

    /**
     * Return the current refresh rate (Only applied to the fullscreen mode).
     * If the value is -1 (WindowSettings::DontCare) the highest refresh rate will be used
     */
    int32_t GetRefreshRate() const;

    /**
     * Return GLFW window
     */
    GLFWwindow* GetGlfwWindow() const;

private:
    void CreateGlfwWindow(const WindowSettings& p_windowSettings);

    /* Callbacks binding */
    void BindKeyCallback() const;
    void BindMouseCallback() const;
    void BindResizeCallback() const;
    void BindFramebufferResizeCallback() const;
    void BindCursorMoveCallback() const;
    void BindMoveCallback() const;
    void BindIconifyCallback() const;
    void BindFocusCallback() const;
    void BindCloseCallback() const;

    /* Event listeners */
    void OnResize(uint16_t p_width, uint16_t p_height);
    void OnMove(int16_t p_x, int16_t p_y);

    /* Internal helpers */
    void UpdateSizeLimit() const;

private:
    /* This map is used by callbacks to find a "Window" instance out of a "GLFWwindow" instance*/
    static std::unordered_map<GLFWwindow*, Window*> __WINDOWS_MAP;

    std::shared_ptr<Device> m_device;
    GLFWwindow* m_glfwWindow;

    /* Window settings */
    std::string m_title;
    std::pair<uint16_t, uint16_t> m_size;
    std::pair<int16_t, int16_t> m_minimumSize;
    std::pair<int16_t, int16_t> m_maximumSize;
    std::pair<int16_t, int16_t> m_position;
    bool m_fullscreen;
    int32_t m_refreshRate;
    Cursor::ECursorMode m_cursorMode;
    Cursor::ECursorShape m_cursorShape;
};

/**
 * The Context handle the engine features setup
 */
class Context {
public:
    /**
     * Constructor
     */
    Context(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings);

    Context() = delete;

    Context(const Context&) = delete;

    Context& operator=(Context&) = delete;

    /**
     * Destructor
     */
    ~Context();

public:
    std::shared_ptr<Device> device;
    std::shared_ptr<Window> window;
    std::shared_ptr<Driver> driver;

private:
    Eventing::ListenerID errorListenerID;
};

#if defined(WIN32)
std::string GetGLVersion();

class ThreadedContext {
public:
    /**
     * Constructor
     */
    ThreadedContext(const WindowSettings& windowSettings, const DeviceSettings& deviceSettings);

    ThreadedContext() = delete;

    ThreadedContext(const ThreadedContext&) = delete;

    ThreadedContext& operator=(ThreadedContext&) = delete;

    /**
     * Destructor
     */
    ~ThreadedContext();

    int GetGLFWThreadID();

    intptr_t GetNativeWindowHandler();

public:
    std::shared_ptr<Device> device;
    std::shared_ptr<Window> window;
    std::shared_ptr<Driver> driver;

private:
    std::shared_ptr<Context> context;
    std::thread thread;
    bool shouldCLose = false;
    std::mutex mutex;
};

#endif

}  // namespace GLFWContext