struct VSInput
{
    float3 _position : POSITION;
    float2 _uv : TEXCOORD;
    float3 _normal : NORMAL;
};

struct PSInput
{
    float4 _position : SV_POSITION;
    float2 _uv : TEXCOORD;
    float3 _normal : NORMAL;
    float3 _worldPos : TEXCOORD1;
};

SamplerState samplerDiffuse : register(s0);
SamplerState samplerNormal : register(s1);

struct PerDraw
{
    row_major float4x4 _worldViewProjMatrix;

    row_major float4x4 _worldMatrix;

    uint _albedoIndex;
    uint _normalIndex;
    uint _metallicRoughNessIndex;
    uint _emissiveIndex;

    uint _accelerationStructureIndex;
    float3 _dirLightDir;

    float _dirLightIntensity;
    float3 _dirLightColor;

    float _pointLightIntensity;
    float3 _cameraPos;

    float _pointLightRadius;
    float3 _pointLightColor;
};

ConstantBuffer<PerDraw> constBuffer : register(b0);

static const float PI = 3.14159265359;

PSInput VSMain(VSInput input)
{
    PSInput result;
    result._position = mul(float4(input._position, 1.0f), constBuffer._worldViewProjMatrix);
    result._normal =  mul(input._normal, (float3x3)constBuffer._worldMatrix);
    result._uv = input._uv;
    result._worldPos = (mul(float4(input._position, 1.), constBuffer._worldMatrix));
    return result;
}

// PBR Functions
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float3 CalculatePBR(float3 N, float3 V, float3 L, float3 lightColor, float3 albedo, float metallic, float roughness, float attenuation)
{
    float3 H = normalize(V + L);

    // base reflectivity (F0)
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = numerator / max(denominator, 0.001);

    // Energy conservation
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * lightColor * NdotL * attenuation;
}

float TraceShadowRay(RaytracingAccelerationStructure accelStruct, float3 origin, float3 direction, float maxDist, RAY_FLAG flags)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    flags |= RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    query.TraceRayInline(accelStruct, flags, 0xFF, ray);
    query.Proceed();

    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    RaytracingAccelerationStructure accelStruct = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._accelerationStructureIndex)];
    Texture2D albedoTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._albedoIndex)];
    Texture2D metallicRoughnessTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._metallicRoughNessIndex)];
    Texture2D emissiveTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._emissiveIndex)];

    float2 uv = input._uv;
	//uv.x = 1.0 - uv.x;
	//uv.y = 1.0 - uv.y;
	//uv = uv.yx;


    float4 albedoColor = albedoTex.Sample(samplerDiffuse, uv);
    float2 metallicRoughness = metallicRoughnessTex.Sample(samplerDiffuse, uv).rg;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float3 emissive = emissiveTex.Sample(samplerDiffuse, uv).rgb;

    float3 N = normalize(input._normal);
    float3 V = normalize(constBuffer._cameraPos - input._worldPos);
    float3 surfaceOffset = input._worldPos + N * 0.1;

    float3 Lo = float3(0.0, 0.0, 0.0);

    // Directional Light
    // the looks of directional light are still messed up
    {
        float3 L = -normalize(constBuffer._dirLightDir);
        float NdotL = max(dot(N, L), 0.0);
        float bias = max(0.05 * (1.0 - NdotL), 0.01);
        float3 shadowOrigin = input._worldPos + N * bias;

        float shadow = TraceShadowRay(accelStruct, shadowOrigin, L, 10000.0, RAY_FLAG_NONE);

        if (shadow > 0.0)
        {
            float3 radiance = constBuffer._dirLightColor * constBuffer._dirLightIntensity;
            Lo += CalculatePBR(N, V, L, radiance, albedoColor.rgb, metallic, roughness, 1.0) * shadow;
        }
    }

     // point light attached to cam
    {
        float3 lightPos = constBuffer._cameraPos;
        float3 L = lightPos -  (input._worldPos);
        float distance = length(L); 
        L = normalize(L);

        // Attenuation with smooth falloff
        float attenuation = 1.0 / (distance * distance);
        attenuation *= pow(max(1.0 - (distance / constBuffer._pointLightRadius), 0.0), 2.0);

        if (attenuation > 0.001)
        {
            float shadow = TraceShadowRay(accelStruct, surfaceOffset, L, distance - 0.01, RAY_FLAG_NONE);

            if (shadow > 0.5)
            {
                float3 radiance = constBuffer._pointLightColor * constBuffer._pointLightIntensity * attenuation;
                Lo += CalculatePBR(N, V, L, radiance, albedoColor.rgb, metallic, roughness, 1.0) * shadow;
            }
        }
    }
    float amb = .1f;
    float3 ambient = float3(amb, amb, amb) * albedoColor.rgb * (1.0 - metallic * 0.5);

    float3 finalColor = ambient + Lo + emissive;

    // Tone mapping (simple Reinhard)
    finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    return float4(finalColor, albedoColor.a);
}