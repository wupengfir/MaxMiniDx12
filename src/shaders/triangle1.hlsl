#include "Common.hlsl"
#include "BRDF.hlsl"
cbuffer MyConstants 
{
    float xxx;     
    float4 testcolor;    
    float4x4 myMatrix;  
};

Texture2D _BaseMap;
Texture2D _MetallicMap;
Texture2D _RoughnessMap;
Texture2D _Normap;
SamplerState sampler_linear_clamp;
float4 tempData;
struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
    float3 normal : NORMAL;
};
struct PSInput 
{
    float4 pos : SV_POSITION; 
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
    float3 wPos : TEXCOOR1;
    float3 normal : NORMAL;
};   
PSInput VS(VSInput input) 
{
    PSInput o;
    o.pos = mul(float4(input.pos, 1.0),Matrix_MVP); 
    o.color = input.color;
    o.uv = input.uv;
    o.normal = mul(_Matrix_M_I, float4(input.normal,0)).xyz;
    o.wPos = mul(float4(input.pos, 1.0), _Matrix_M).xyz;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    //float4 color = tex01.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0));

    float3 albedo = _BaseMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0));
    float metallic = _MetallicMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0)).r;
    float roughness = _RoughnessMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0)).r;
    float3 viewDir = normalize(_CameraPos - input.wPos);
    float3 color = UE_PBR_BRDF(albedo,metallic,roughness*roughness,input.normal,viewDir,_MainLightDirection);

    float3 irradiance = _MainLightColor;
    return float4(color.xyz*irradiance, 1.0); 
}
