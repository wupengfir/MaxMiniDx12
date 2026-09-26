#include "Common.hlsl"
#include "Sample.hlsl"


Texture2D tex01;
TextureCube cubemap;
SamplerState sampler_linear_clamp;
float4x4 _FaceRotateMatrix;
float _Roughness;
struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
};
struct PSInput 
{
    float4 pos : SV_POSITION; 
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
    float3 wDir : TEXCOORD1;
};   
PSInput VS(VSInput input) 
{
    PSInput o;
    o.pos = float4(input.pos, 1.0);
    o.color = input.color;
    o.uv = input.uv;
    o.wDir = mul(_FaceRotateMatrix,float4(input.pos.x,input.pos.y,1,0)).xyz;
    return o;
}

float3 convoloveCubeMap(float3 outputDir, float roughness, uint2 pixelCoord)
{
    roughness = max(1e-4, roughness);
    float Alpha = roughness * roughness;

// 1. 当前cubemap像素的宏观法线n = wo (预过滤cubemap经典设定)
    float3 wo = normalize(outputDir); // outputDir就是当前cubemap texel方向
    float3 n = wo;

    float3 color = 0;
    float weightSum = 0;

    int sampleCount = 16;
    for (int i = 0; i < sampleCount; i++)
    {
    // 低差异序列： Hammersley
        float2 offset = Hammersley(i, sampleCount);
        float2 u = R2SequencePerPixel(pixelCoord, _Time.w, offset);
        float u0 = u.x;
        float u1 = u.y;

    // GGX重要性采样生成半矢量h（局部切空间，z=n）
        float theta = atan(Alpha * sqrt(u0) / sqrt(1 - u0));
        float phi = 2 * PI * u1;

        float3 hLocal;
        hLocal.x = sin(theta) * cos(phi);
        hLocal.y = sin(theta) * sin(phi);
        hLocal.z = cos(theta);
        
        float3 tangent = normalize(abs(n.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0));
        tangent = normalize(cross(tangent, n));
        float3 bitangent = cross(n, tangent);
        float3x3 TBN = float3x3(tangent, bitangent, n);
    // 转到世界空间
        float3 h = normalize(mul(hLocal, TBN));

    // 计算入射方向 wi
        float3 wi = 2 * dot(h, wo) * h - wo;

    // 剔除下半球
        float NdotWi = dot(n, wi);
        if (NdotWi <= 0)
            continue;

    // GGX NDF
        float NdotH = saturate(dot(n, h));
        float D = D_GGX(NdotH, Alpha);
        float WoDotH = saturate(dot(wo, h));
        float pdf = D * NdotH / (4 * WoDotH);

    // 采样原始cubemap
        float3 Li = cubemap.SampleLevel(sampler_linear_clamp, wi, 0).rgb;

    // 积分累加
        color += Li * NdotWi / pdf;
        weightSum += NdotWi / pdf;
    }
    color /= weightSum; // 归一化
    return color;
}

float4 PS(PSInput input) : SV_TARGET
{
    float2 seed = input.uv.xy + float2(_Time.w * 0.131, _Time.w * 0.279);
    float2 xi = hash22(seed);
    input.wDir = normalize(input.wDir);
    float3 color = convoloveCubeMap(input.wDir, _Roughness,input.pos.xy);
    return float4(color.xyz, 0.01); 
}
