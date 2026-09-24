#include "Common.hlsl"



Texture2D tex01;
TextureCube cubemap;
SamplerState sampler_linear_clamp;
float4 tempData;
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
    //input.pos.y*=-1;
    o.wDir = mul(_FaceRotateMatrix,float4(1,-input.pos.x,input.pos.y,0)).xyz;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    //input.wDir = mul(_FaceRotateMatrix,float4(input.wDir,0)).xyz;
   // float4 color = tex01.SampleLevel(sampler_linear_clamp,input.uv.xy,5);
    float4 color = cubemap.Sample(sampler_linear_clamp,input.wDir.xzy);
    return float4(color.xyz*1, 1.0); 
}
