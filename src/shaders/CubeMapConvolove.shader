#include "Common.hlsl"
#include "Sample.hlsl"


Texture2D tex01;
TextureCube cubemap;
SamplerState sampler_linear_clamp;
float4x4 _FaceRotateMatrix;
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
float4 PS(PSInput input) : SV_TARGET
{
    float2 seed = input.uv.xy + float2(_Time.w * 0.131, _Time.w * 0.279);
    float2 xi = hash22(seed);
    input.wDir = normalize(input.wDir);
    float3 L = SampleCosineHemisphere(xi, input.wDir);
    float4 origin = cubemap.Sample(sampler_linear_clamp,input.wDir);
    float4 color = cubemap.Sample(sampler_linear_clamp,L);
    color = lerp(origin,color,saturate(_Time.z*0.05));
    return float4(color.xyz, 0.01); 
}
