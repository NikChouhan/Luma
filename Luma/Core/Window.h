#pragma once

#include <SDL2/SDL.h>
#include <string>

// Type aliases (assuming these are defined elsewhere in your codebase)
// If not, you may need to define them or use standard types
#ifndef u16
using u16 = uint16_t;
#endif

#ifndef u32
using u32 = uint32_t;
#endif

struct WindowDesc
{
    u16 width;
    u16 height;
    std::string title; // Changed from std::wstring to std::string for SDL compatibility
};

class Window
{
public:
    Window(const WindowDesc& desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool PollEvents();
    void SetTitle(const std::string& title);

    [[nodiscard]] SDL_Window* GetHandle() const { return window; }
    [[nodiscard]] u32 GetWidth() const { return width; }
    [[nodiscard]] u32 GetHeight() const { return height; }
    [[nodiscard]] bool IsClosed() const { return isClosed; }

private:
    void HandleEvent(const SDL_Event& event);

    SDL_Window* window = nullptr;
    u32 width = 0;
    u32 height = 0;
    std::string title;
    bool isClosed = false;
};