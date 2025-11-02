//// barebones setup
//cbuffer SceneData : register(b0)
//{
//    float2 resolution;
//    float time;
//    float cameraYaw;
//
//    float cameraPitch;
//    float3 cameraPos;
//
//    uint uavIndex;
//    float3 padding;
//};
//
//
//[numthreads(8, 8, 1)]
//void CSMain(uint3 DTid : SV_DispatchThreadID)
//{
//    RWTexture2D<float4> OutputTexture = ResourceDescriptorHeap[NonUniformResourceIndex(uavIndex)];
//
//    float2 fragCoord = float2(DTid.xy);
//
//    // get coords and direction
//    float2 uv = fragCoord.xy / resolution.xy - 0.5;
//    uv.y *= resolution.y / resolution.x;
//    
//
//    OutputTexture[DTid.xy] = float4(uv * 0.01, uv.x, 1.0);
//}

// Galaxy shader
// Created by Frank Hugenroth  /frankenburgh/   07/2015
// Released at nordlicht/bremen 2015

cbuffer SceneData : register(b0)
{
    // 16 bytes
    float2 resolution;
    float time;
    float cameraYaw;

    // 16 bytes
    float cameraPitch;
    uint uavIndex;
    float padding2;
    float padding3;

    // 16 bytes
    float3 cameraPos;
    float padding4;
};

#define SCREEN_EFFECT 0
SamplerState LinearSampler : register(s0);

// random/hash function              
float hash(float n)
{
    return frac(cos(n) * 41415.92653);
}

// 2d noise function
float noise(float2 x)
{
    float2 p = floor(x);
    float2 f = smoothstep(0.0, 1.0, frac(x));
    float n = p.x + p.y * 57.0;

    return lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
        lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y);
}

float noise(float3 x)
{
    float3 p = floor(x);
    float3 f = smoothstep(0.0, 1.0, frac(x));
    float n = p.x + p.y * 57.0 + 113.0 * p.z;

    return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
        lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
        lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
            lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

static float3x3 m = float3x3(0.00, 1.60, 1.20, -1.60, 0.72, -0.96, -1.20, -0.96, 1.28);

// Fractional Brownian motion
float fbmslow(float3 p)
{
    float f = 0.5000 * noise(p); p = mul(m, p * 1.2);
    f += 0.2500 * noise(p); p = mul(m, p * 1.3);
    f += 0.1666 * noise(p); p = mul(m, p * 1.4);
    f += 0.0834 * noise(p); p = mul(m, p * 1.84);
    return f;
}

float fbm(float3 p)
{
    float f = 0., a = 1., s = 0.;
    f += a * noise(p); p = mul(m, p * 1.149); s += a; a *= .75;
    f += a * noise(p); p = mul(m, p * 1.41); s += a; a *= .75;
    f += a * noise(p); p = mul(m, p * 1.51); s += a; a *= .65;
    f += a * noise(p); p = mul(m, p * 1.21); s += a; a *= .35;
    f += a * noise(p); p = mul(m, p * 1.41); s += a; a *= .75;
    f += a * noise(p);
    return f / s;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    RWTexture2D<float4> OutputTexture = ResourceDescriptorHeap[NonUniformResourceIndex(uavIndex)];

    float2 fragCoord = float2(DTid.xy);
    float localTime = time;

    float2 xy = -1.0 + 2.0 * fragCoord.xy / resolution.xy;

    // fade in (1=10sec), out after 8=80sec;
    float fade = min(1., localTime * 1.) * min(1., max(0., 15. - localTime));
    // start glow after 5=50sec
    float fade2 = max(0., localTime - 10.) * 0.37;
    float glow = max(-.25, 1. + pow(fade2, 10.) - 0.001 * pow(fade2, 25.));

    // get camera position and view direction (static)
    float3 campos = float3(500.0, 850., -0.0 - cos((localTime - 1.4) / 2.) * 2000.);
    float3 camtar = float3(0., 0., 0.);

    float roll = 0.34;
    float3 cw = normalize(camtar - campos);
    float3 cp = float3(sin(roll), cos(roll), 0.0);
    float3 cu = normalize(cross(cw, cp));
    float3 cv = normalize(cross(cu, cw));
    float3 rd = normalize(xy.x * cu + xy.y * cv + 1.6 * cw);

    float3 light = normalize(float3(0., 0., 0.) - campos);
    float sundot = clamp(dot(light, rd), 0.0, 1.0);

    // render sky
    // galaxy center glow
    float3 col = glow * 1.2 * min(float3(1.0, 1.0, 1.0), float3(2.0, 1.0, 0.5) * pow(sundot, 100.0));
    // moon haze
    col += 0.3 * float3(0.8, 0.9, 1.2) * pow(sundot, 8.0);

    // stars
    float3 stars = 85.5 * float3(pow(fbmslow(rd.xyz * 312.0), 7.0), pow(fbmslow(rd.xyz * 312.0), 7.0), pow(fbmslow(rd.xyz * 312.0), 7.0)) *
        float3(pow(fbmslow(rd.zxy * 440.3), 8.0), pow(fbmslow(rd.zxy * 440.3), 8.0), pow(fbmslow(rd.zxy * 440.3), 8.0));

    // moving background fog
    float3 cpos = 1500. * rd + float3(831.0 - localTime * 30., 321.0, 1000.0);
    col += float3(0.4, 0.5, 1.0) * ((fbmslow(cpos * 0.0035) - .5));

    cpos += float3(831.0 - localTime * 33., 321.0, 999.);
    col += float3(0.6, 0.3, 0.6) * 10.0 * pow((fbmslow(cpos * 0.0045)), 10.0);

    cpos += float3(3831.0 - localTime * 39., 221.0, 999.0);
    col += 0.03 * float3(0.6, 0.0, 0.0) * 10.0 * pow((fbmslow(cpos * 0.0145)), 2.0);

    // stars
    cpos = 1500. * rd + float3(831.0, 321.0, 999.);
    col += stars * fbm(cpos * 0.0021);

    // Clouds
    float2 shift = float2(localTime * 100.0, localTime * 180.0);
    float4 sum = float4(0, 0, 0, 0);
    float c = campos.y / rd.y; // cloud height
    float3 cpos2 = campos - c * rd;
    float radius = length(cpos2.xz) / 1000.0;

    if (radius < 1.8)
    {
        for (int q = 10; q > -10; q--) // layers
        {
            if (sum.w > 0.999) break;
            float c = (float(q) * 8. - campos.y) / rd.y; // cloud height
            float3 cpos = campos + c * rd;

            float see = dot(normalize(cpos), normalize(campos));
            float3 lightUnvis = float3(.0, .0, .0);
            float3 lightVis = float3(1.3, 1.2, 1.2);
            float3 shine = lerp(lightVis, lightUnvis, smoothstep(0.0, 1.0, see));

            // border
            float radius = length(cpos.xz) / 999.;
            if (radius > 1.0)
                continue;

            float rot = 3.00 * (radius)-localTime;
            float2x2 rotMat = float2x2(cos(rot), -sin(rot), sin(rot), cos(rot));
            cpos.xz = mul(rotMat, cpos.xz);

            cpos += float3(831.0 + shift.x, 321.0 + float(q) * lerp(250.0, 50.0, radius) - shift.x * 0.2, 1330.0 + shift.y);
            cpos *= lerp(0.0025, 0.0028, radius);
            float alpha = smoothstep(0.50, 1.0, fbm(cpos));
            alpha *= 1.3 * pow(smoothstep(1.0, 0.0, radius), 0.3);
            float3 dustcolor = lerp(float3(2.0, 1.3, 1.0), float3(0.1, 0.2, 0.3), pow(radius, .5));
            float3 localcolor = lerp(dustcolor, shine, alpha);

            float gstar = 2. * pow(noise(cpos * 21.40), 22.0);
            float gstar2 = 3. * pow(noise(cpos * 26.55), 34.0);
            float gholes = 1. * pow(noise(cpos * 11.55), 14.0);
            localcolor += float3(1.0, 0.6, 0.3) * gstar;
            localcolor += float3(1.0, 1.0, 0.7) * gstar2;
            localcolor -= gholes;

            alpha = (1.0 - sum.w) * alpha;
            sum += float4(localcolor * alpha, alpha);
        }

        for (int q = 0; q < 20; q++)
        {
            if (sum.w > 0.999) continue;
            float c = (float(q) * 4. - campos.y) / rd.y;
            float3 cpos = campos + c * rd;

            float see = dot(normalize(cpos), normalize(campos));
            float3 lightUnvis = float3(.0, .0, .0);
            float3 lightVis = float3(1.3, 1.2, 1.2);
            float3 shine = lerp(lightVis, lightUnvis, smoothstep(0.0, 1.0, see));

            float radius = length(cpos.xz) / 200.0;
            if (radius > 1.0)
                continue;

            float rot = 3.2 * (radius)-localTime * 1.1;
            float2x2 rotMat = float2x2(cos(rot), -sin(rot), sin(rot), cos(rot));
            cpos.xz = mul(rotMat, cpos.xz);

            cpos += float3(831.0 + shift.x, 321.0 + float(q) * lerp(250.0, 50.0, radius) - shift.x * 0.2, 1330.0 + shift.y);
            float alpha = 0.1 + smoothstep(0.6, 1.0, fbm(cpos));
            alpha *= 1.2 * (pow(smoothstep(1.0, 0.0, radius), 0.72) - pow(smoothstep(1.0, 0.0, radius * 1.875), 0.2));
            float3 localcolor = float3(0.0, 0.0, 0.0);

            alpha = (1.0 - sum.w) * alpha;
            sum += float4(localcolor * alpha, alpha);
        }
    }

    float alpha = smoothstep(1. - radius * .5, 1.0, sum.w);
    sum.rgb /= sum.w + 0.0001;
    sum.rgb -= 0.2 * float3(0.8, 0.75, 0.7) * pow(sundot, 10.0) * alpha;
    sum.rgb += min(glow, 10.0) * 0.2 * float3(1.2, 1.2, 1.2) * pow(sundot, 5.0) * (1.0 - alpha);

    col = lerp(col, sum.rgb, sum.w);

    // haze
    col = fade * lerp(col, float3(0.3, 0.5, .9), 29.0 * (pow(sundot, 50.0) - pow(sundot, 60.0)) / (2. + 9. * abs(rd.y)));

    // Vignetting
    float2 xy2 = fragCoord.xy / resolution.xy;
    col *= float3(.5, .5, .5) + 0.25 * pow(100.0 * xy2.x * xy2.y * (1.0 - xy2.x) * (1.0 - xy2.y), .5);

    OutputTexture[DTid.xy] = float4(col * 10.f, 1.0);
    //OutputTexture[DTid.xy] = float4(1., 1., 0., 1.);
}