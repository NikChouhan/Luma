#pragma once
#include <imgui_internal.h>

#include <DirectXMath.h>
#include "External/SimpleMath/SimpleMath.h"

#include <StandardTypes.h>

namespace SM = DirectX::SimpleMath;

constexpr auto cluster_number = (16 * 9 * 24);

namespace SponzaAABB
{
    inline constexpr SM::Vector3 min = { 17, -1, -9 };
    inline constexpr SM::Vector3 max = { -15, 13, 9 };
};

struct BindlessHeapIndex
{
    u32 nextIndex;
};
struct LightSettings
{
    float pointIntensity = 70.0f;
    float pointColor[3] = { 0.8f, 0.6f, 0.5f };
    float pointRadius = 10.0f;
    float dirIntensity = 2.0f;
    float dirColor[3] = { 1.0f, 0.85f, 0.6f };
    SM::Vector3 direction = { -1.0f, -2.0f, 0.0f };
};

struct LumaConstants
{
    u32 width = 1920;
    u32 height = 1080;
};

namespace GlobalStorage
{
    inline BindlessHeapIndex bindlessHeapIndex{};
    inline LumaConstants g_LumaConstants{};

    inline u32 depthSRVIndex;

    inline DirectX::XMMATRIX projMatrixInv{};
    inline DirectX::XMMATRIX viewMatrixInv{};
    inline LightSettings g_lightSettings;
}

inline void SetupLightSettingsHandler()
{
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "LightSettings";
    ini_handler.TypeHash = ImHashStr("LightSettings");

    ini_handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) { return (void*)1; };
    ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
        float x, y, z;
        if (sscanf(line, "pointIntensity=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.pointIntensity = x;
        else if (sscanf(line, "pointColor=%f,%f,%f", &x, &y, &z) == 3) {
	        GlobalStorage::g_lightSettings.pointColor[0] = x;
	        GlobalStorage::g_lightSettings.pointColor[1] = y;
	        GlobalStorage::g_lightSettings.pointColor[2] = z;
        }
        else if (sscanf(line, "pointRadius=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.pointRadius = x;
        else if (sscanf(line, "dirIntensity=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.dirIntensity = x;
        else if (sscanf(line, "dirColor=%f,%f,%f", &x, &y, &z) == 3) {
	        GlobalStorage::g_lightSettings.dirColor[0] = x;
	        GlobalStorage::g_lightSettings.dirColor[1] = y;
	        GlobalStorage::g_lightSettings.dirColor[2] = z;
        }
        else if (sscanf(line, "direction=%f,%f,%f", &x, &y, &z) == 3)
	        GlobalStorage::g_lightSettings.direction = SM::Vector3(x, y, z);
        };

    ini_handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        buf->appendf("[%s][Settings]\n", handler->TypeName);
        buf->appendf("pointIntensity=%.3f\n", GlobalStorage::g_lightSettings.pointIntensity);
        buf->appendf("pointColor=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.pointColor[0], GlobalStorage::g_lightSettings.pointColor[1], GlobalStorage::g_lightSettings.pointColor[2]);
        buf->appendf("pointRadius=%.3f\n", GlobalStorage::g_lightSettings.pointRadius);
        buf->appendf("dirIntensity=%.3f\n", GlobalStorage::g_lightSettings.dirIntensity);
        buf->appendf("dirColor=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.dirColor[0], GlobalStorage::g_lightSettings.dirColor[1], GlobalStorage::g_lightSettings.dirColor[2]);
        buf->appendf("direction=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.direction.x, GlobalStorage::g_lightSettings.direction.y, GlobalStorage::g_lightSettings.direction.z);
        buf->append("\n");
        };

    ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
}

struct InputGlobalState
{
	inline static bool keys[256];
    inline static int lastMouseX;
    inline static int lastMouseY;
    inline static int currentMouseX;
    inline static int currentMouseY;

	inline static bool isMouseCaptured;
};

typedef InputGlobalState IGS;