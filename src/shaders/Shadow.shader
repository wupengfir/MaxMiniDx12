#include "ShadowCommon.hlsl"


struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;

};
struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normalWS : NORMAL;
};
PSInput VS(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 1.0), Matrix_MVP);
    o.normalWS = mul(_Matrix_M_I, float4(input.normal, 0)).xyz;
    return o;
}
float4 PS(PSInput input) : SV_TARGET
{
    return 0;
}
