#include "Window.h"

#include <imgui.h>
#include <windowsx.h>

#include "Graphics/FrameSync.h"
#include "Graphics/Globals.h"

static const wchar_t* CLASS_NAME = L"LumaEngineWindowClass";
extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



Window::Window(GfxDevice& gfxDevice, FrameSync& frameSync, const WindowDesc& desc)
	: gfxDevice_(gfxDevice), frameSync_(frameSync)
{
	hInstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassEx(&wc);

    hwnd_ = CreateWindowEx(
        0,
        CLASS_NAME,
        desc.title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance_,
        this
    );

    assert(hwnd_ && "Failed to create window");

    ShowWindow(hwnd_, SW_SHOW);
}

Window::~Window()
{
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        UnregisterClass(CLASS_NAME, hInstance_);
    }
}

bool Window::PollEvents()
{
    MSG msg = {};

    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            isClosed_ = true;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return !isClosed_;
}

void Window::SetTitle(const std::wstring& title)
{
    title_ = title;
    SetWindowText(hwnd_, title.c_str());
}

LRESULT Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* pThis = nullptr;

    if (msg == WM_NCCREATE) // before window creation, WM_NCCREATE is passed
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Window*>(pCreate->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMsg(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
    {
        return true;
    }

    switch (uMsg)
    {
    case WM_DESTROY:
        isClosed_ = true;
        WaitForGPU(gfxDevice_, frameSync_);
        DestroyDevice(gfxDevice_);
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam < 256) IGS::keys[wParam] = true;
        return 0;
    case WM_KEYUP:
        if (wParam < 256) IGS::keys[wParam] = false;

        if (wParam == VK_ESCAPE)
        {
            WaitForGPU(gfxDevice_, frameSync_);
            DestroyDevice(gfxDevice_);
            PostQuitMessage(0);
        }
        if (wParam == 'M')
        {
            IGS::isMouseCaptured = !IGS::isMouseCaptured;
            ShowCursor(!IGS::isMouseCaptured);

            RECT rect;
            GetClientRect(hwnd, &rect);
            ClientToScreen(hwnd, (POINT*)&rect.left);
            ClientToScreen(hwnd, (POINT*)&rect.right);

            if (IGS::isMouseCaptured)
            {
                ClipCursor(&rect);
            }
            else
            {
                ClipCursor(NULL);
            }
            POINT center = { rect.right / 2, rect.bottom / 2 };
            SetCursorPos(center.x, center.y);
            IGS::lastMouseX = IGS::currentMouseX = center.x;
            IGS::lastMouseY = IGS::currentMouseY = center.y;
        }
        return 0;
    case WM_MOUSEMOVE:
        IGS::lastMouseX = IGS::currentMouseX;
        IGS::lastMouseY = IGS::currentMouseY;
        IGS::currentMouseX = GET_X_LPARAM(lParam);
        IGS::currentMouseY = GET_Y_LPARAM(lParam);
        return 0;
        // Will resolve resizing later when the architecture is fixed

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}


