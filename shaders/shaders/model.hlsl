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
    uint _accelerationStructureIndex;
    uint _padding;
    float3 _lightDir;
    uint _padding2;
    float3 _cameraPos;
    uint _padding3;
};

ConstantBuffer<PerDraw> constBuffer : register(b0);

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL)
{
    PSInput result;

    result._position = mul(float4(position, 1.0f), constBuffer._worldViewProjMatrix);
    result._normal = mul(constBuffer._worldMatrix, float4(normal, 0.0f)).xyz;
    result._uv = uv;
    result._worldPos = mul(float4(position, 1.f), constBuffer._worldMatrix);
    return result;
}

float3 CalculatePhong(float3 normal, float3 lightDir, float3 viewDir, float3 lightColor, float shininess)
{
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * lightColor;

    // Specular
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float3 specular = spec * lightColor;

    return (diffuse + specular);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // uv test (pass)
    //return float4(input.uv.x, input.uv.y, 0.0f, 1.0f);

    // normals test (pass)
    // float3 normal = normalize(input.normal);
    // float3 outputNormal = normal * 0.5f + 0.5f;
    // return float4(outputNormal, 1.0f);

    RaytracingAccelerationStructure accelerationStructure = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._accelerationStructureIndex)];
    Texture2D tex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._albedoIndex)];
    Texture2D normalTex = ResourceDescriptorHeap[NonUniformResourceIndex(constBuffer._normalIndex)];

    float4 albedoColor = tex.Sample(samplerDiffuse, input._uv);

    float3 normal = normalize(input._normal);

    float3 lightDirection = normalize(constBuffer._lightDir);
    float3 viewDir = normalize(constBuffer._cameraPos - input._worldPos);

    float3 lightColor = float3(1.0, 1.0, 1.0);
    float shininess = 32.0;
    float3 ambient = float3(0.1, 0.1, 0.1);

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    RayDesc ray;
    ray.Origin = input._worldPos + input._normal * 0.001;
    ray.Direction = -lightDirection;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    query.TraceRayInline(accelerationStructure, RAY_FLAG_NONE, 0xFF, ray);
    query.Proceed();

    float shadow = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.1 : 1.0;

    // Apply lighting
    float3 lighting = CalculatePhong(normal, lightDirection, viewDir, lightColor, shininess);

    float3 finalColor = (ambient + (lighting * shadow)) * albedoColor.rgb;

    return float4(finalColor, albedoColor.a);
}
