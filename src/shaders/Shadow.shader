#include "ShadowCommon.hlsl"

cbuffer PerMaterial
{
    float4 _ShadowBias;
}

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;

};
struct PSInput
{
    float4 pos : SV_POSITION;
};
PSInput VS(VSInput input)
{
    PSInput o;
    float3 wPos = mul(float4(input.pos, 1.0),_Matrix_M);
    float3 normalWS = normalize(mul(_Matrix_M_I, float4(input.normal, 0)).xyz);
    wPos -= _MainLightDirection * _ShadowBias.x + normalWS * _ShadowBias.y;
    o.pos = mul(float4(wPos, 1.0), _Matrix_VP);
    
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    return 0;
}
