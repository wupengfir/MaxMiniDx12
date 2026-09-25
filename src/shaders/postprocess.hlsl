#include "Common.hlsl"

cbuffer PerMaterial 
{
    float _Exposure;
};

Texture2D _ColorAttachment;
SamplerState sampler_linear_clamp;
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
    float3 wPos : TEXCOOR1;
};   
PSInput VS(VSInput input) 
{
    PSInput o;
    o.pos = float4(input.pos, 1.0); 
    o.color = input.color;
    o.uv = input.uv;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    input.uv.y = 1 - input.uv.y;
    float4 color = _ColorAttachment.Sample(sampler_linear_clamp,input.uv.xy);


    //tonemap
    color.xyz = ACESFilmic(color.xyz * exp2(_Exposure));

    return float4(color.xyz*1, 1.0); 
}
