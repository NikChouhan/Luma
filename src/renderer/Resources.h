#pragma once
#include "Pipeline.h"

struct Resources
{
	std::vector<Shader> shaders;
	std::vector<Pipeline> pipelines;

	std::vector<ShaderDesc> shaderParams;
	std::vector<PipelineDesc> pipelineParams;
};