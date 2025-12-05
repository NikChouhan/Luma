#pragma once
#include <imgui_internal.h>

#include <DirectXMath.h>
#include "External/SimpleMath/SimpleMath.h"

#include <StandardTypes.h>

namespace SM = DirectX::SimpleMath;

struct HeapTextureIndex
{
    u32 accelerationStructureIndex{};
    u32 computeShaderBgIndex{};
    u32 depthSRV{};
    u32 normalUAV{};
    u32 rtaoUAV{};
};
struct PipelineIndex
{
    u8 BGComputePass{};
    u8 DepthPrePass{};
    u8 RTAOPass{};
    u8 ImguiPass{};
    u8 RenderPass{};
};
struct ShaderIndex
{
    u8 bgComputeShader{};
    u8 depthPPShader{};
    u8 rtaoShader{};

    u8 renderPassVSShader{};
    u8 renderPassPSShader{};
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

namespace GlobalStorage
{
    inline HeapTextureIndex index{};
    inline PipelineIndex pipelineIndex{};
    inline ShaderIndex shaderIndex{};

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
