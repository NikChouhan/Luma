#include "Window.h"

#if defined(RHI_BACKEND_VULKAN)
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#elif RHI_BACKEND_D3D12
#include <imgui.h>
#include <imgui_impl_win32.h>
#endif
#include "Log.h"
#include "Graphics/Globals.h"
#include "Graphics/RHI/RHI.h"

// TODO: verify with the docs
//extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplSDL2_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Window::Window(const WindowDesc& desc)
    : width(desc.width)
    , height(desc.height)
    , title(desc.title)
{
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        assert(false && "Failed to initialize SDL");
        return;
    }
    u32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if defined(RHI_BACKEND_VULKAN)
    flags |= SDL_WINDOW_VULKAN;
#endif
    window = SDL_CreateWindow(
        desc.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        desc.width,
        desc.height,
        flags);

    if (!window)
        printl(Log::LogLevel::Error, "[Core] Failed to create window");
}

Window::~Window()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

bool Window::PollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        //ImGui_ImplSDL2_ProcessEvent(&event);

        HandleEvent(event);

        if (event.type == SDL_QUIT)
        {
            isClosed = true;
            return false;
        }
    }

    return !isClosed;
}

void Window::SetTitle(const std::string& title)
{
    this->title = title;
    SDL_SetWindowTitle(window, title.c_str());
}

void Window::HandleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        isClosed = true;
        RHI::WaitIdle();
        break;

    case SDL_KEYDOWN:
    {
        SDL_Scancode scancode = event.key.keysym.scancode;
        if (scancode < 256)
        {
            IGS::keys[scancode] = true;
        }

        if (event.key.keysym.sym == SDLK_ESCAPE)
        {
            RHI::WaitIdle();
            SDL_Event quitEvent;
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }

        if (event.key.keysym.sym == SDLK_m)
        {
            IGS::isMouseCaptured = !IGS::isMouseCaptured;

            if (IGS::isMouseCaptured)
            {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                SDL_ShowCursor(SDL_DISABLE);
            }
            else
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_ShowCursor(SDL_ENABLE);
            }

            int windowWidth, windowHeight;
            SDL_GetWindowSize(window, &windowWidth, &windowHeight);

            IGS::lastMouseX = IGS::currentMouseX = windowWidth / 2;
            IGS::lastMouseY = IGS::currentMouseY = windowHeight / 2;

            SDL_WarpMouseInWindow(window, windowWidth / 2, windowHeight / 2);
        }
        break;
    }

    case SDL_KEYUP:
    {
        SDL_Scancode scancode = event.key.keysym.scancode;
        if (scancode < 256)
        {
            IGS::keys[scancode] = false;
        }
        break;
    }

    case SDL_MOUSEMOTION:
        IGS::lastMouseX = IGS::currentMouseX;
        IGS::lastMouseY = IGS::currentMouseY;
        IGS::currentMouseX = event.motion.x;
        IGS::currentMouseY = event.motion.y;
        break;

    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            width = event.window.data1;
            height = event.window.data2;
            // TODO: Handle window resize for your RHI
        }
        break;

    default:
        break;
    }
}