#include "Common.hlsl"

cbuffer MyConstants 
{
    float myFloat;      
    float4 myFloat4;    
    float4x4 myMatrix;  
};

Texture2D tex01;
TextureCube cubemap;
SamplerState sampler_linear_clamp;
float4 tempData;
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
    o.pos = mul(float4(input.pos, 1.0),Matrix_MVP); 
    //o.pos = float4(input.pos, 1.0);
    o.wPos = mul(float4(input.pos, 1.0), _Matrix_M).xyz;
    o.color = input.color;
    o.uv = input.uv;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
   // float4 color = tex01.SampleLevel(sampler_linear_clamp,input.uv.xy,5);
    float4 color = cubemap.Sample(sampler_linear_clamp,normalize(input.wPos.xzy - _CameraPos.xzy));
    return float4(color.xyz*1, 1.0); 
}
