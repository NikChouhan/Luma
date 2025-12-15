#pragma once
#include "Graphics/GfxDevice.h"

struct WindowDesc
{
	u16 width;
	u16 height;
	const std::wstring title;
};

struct Window
{
	Window(GfxDevice& gfxDevice, FrameSync& frameSync, const WindowDesc& desc);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool PollEvents();
	void SetTitle(const std::wstring& title);

	[[nodiscard]] HWND GetHandle() const { return hwnd_; }
	[[nodiscard]] u32 GetWidth() const { return width_; }
	[[nodiscard]] u32 GetHeight() const { return height_; }
	[[nodiscard]] bool IsClosed() const { return isClosed_; }

private:
	GfxDevice& gfxDevice_;
	FrameSync& frameSync_;

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND hwnd_ = nullptr;
	HINSTANCE hInstance_ = nullptr;
	u32 width_ = 0;
	u32 height_ = 0;
	std::wstring title_;
	bool isClosed_ = false;
};