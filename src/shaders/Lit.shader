#include "Common.hlsl"
#include "BRDF.hlsl"

#define MAX_REFLECTION_LOD 10.0

cbuffer MyConstants 
{
    float xxx;     
    float4 testcolor;    
    float4x4 myMatrix;  
};

Texture2D _BaseMap;
Texture2D _NormalMap;
Texture2D _MetallicMap;
Texture2D _RoughnessMap;
Texture2D _BrdfMap;
TextureCube _IrradianceMap;

TextureCube _ReflectionMap;

TextureCube _GeneratedIrradiancemap;
TextureCube _GeneratedReflectionmap;

SamplerState sampler_linear_clamp;
float4 tempData;
struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};
struct PSInput 
{
    float4 pos : SV_POSITION; 
    float3 color : COLOR;
    float4 uv : TEXCOORD0;
    float3 wPos : TEXCOOR1;
    float3 normalWS : NORMAL;
    float4 tangentWS : TANGENT;
};   
PSInput VS(VSInput input) 
{
    PSInput o;
    o.pos = mul(float4(input.pos, 1.0),Matrix_MVP); 
    o.color = input.color;
    o.uv = input.uv;
    o.normalWS = mul(_Matrix_M_I, float4(input.normal,0)).xyz;
    float sign = input.tangent.w;
    o.tangentWS = mul(float4(input.tangent.xyz,0),_Matrix_M);
    o.wPos = mul(float4(input.pos, 1.0), _Matrix_M).xyz;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    input.normalWS = normalize(input.normalWS);
    float sgn = input.tangentWS.w;      // should be either +1 or -1
    float3 bitangent = sgn * cross(input.normalWS.xyz, input.tangentWS.xyz);
    float3x3 tangentToWorld = half3x3(input.tangentWS.xyz, bitangent.xyz, input.normalWS.xyz);
    float3 normalTS = _NormalMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0))*2-1;
    float3 normalWS = mul(normalTS, tangentToWorld);
    
    float3 albedo = _BaseMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0));
    float metallic = _MetallicMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0)).r;
    float roughness = _RoughnessMap.Sample(sampler_linear_clamp,float4(input.uv.xy,0,0)).r;
    float3 viewDir = normalize(_CameraPos - input.wPos);

    //采样注意坐标系
    GIInput giInput;
    giInput.diffuseIrradiance = _GeneratedIrradiancemap.Sample(sampler_linear_clamp, normalWS.xzy).xyz;
    float3 reflectDir = reflect(-viewDir, normalWS);
    giInput.reflecIrradiance = _GeneratedReflectionmap.SampleLevel(sampler_linear_clamp, reflectDir.xzy, roughness * MAX_REFLECTION_LOD).xyz;
    giInput.iblBrdf = _BrdfMap.Sample(sampler_linear_clamp,float2(saturate(dot(normalWS, viewDir)), roughness)).rg;
    float3 color = UE_PBR_BRDF(albedo,metallic,roughness*roughness,normalWS,viewDir,_MainLightDirection,_MainLightColor.xyz,giInput);

    

    return float4(color.xyz, 1.0); 
}
