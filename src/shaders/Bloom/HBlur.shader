#include "../Common.hlsl"


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

float Max3(float a, float b, float c)
{
    return max(max(a, b), c);
}
#define SAMPLE_TEXTURE2D_X(texture, sampler, uv) texture.Sample(sampler, uv)
half3 DecodeHDR(float4 color)
{
    return color.rgb;
}

float4 PS(PSInput input) : SV_TARGET
{
    input.uv.y = 1 - input.uv.y;
    float texelSize = _SourceTex_TexelSize.x * 2.0;
    float2 uv =input.uv.xy;

    half3 c0 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(texelSize * 4.0, 0.0)));
    half3 c1 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(texelSize * 3.0, 0.0)));
    half3 c2 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(texelSize * 2.0, 0.0)));
    half3 c3 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(texelSize * 1.0, 0.0)));
    half3 c4 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv));
    half3 c5 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(texelSize * 1.0, 0.0)));
    half3 c6 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(texelSize * 2.0, 0.0)));
    half3 c7 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(texelSize * 3.0, 0.0)));
    half3 c8 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(texelSize * 4.0, 0.0)));

    half3 color = c0 * 0.01621622 + c1 * 0.05405405 + c2 * 0.12162162 + c3 * 0.19459459
                        + c4 * 0.22702703
                        + c5 * 0.19459459 + c6 * 0.12162162 + c7 * 0.05405405 + c8 * 0.01621622;

    return float4(color,1);
}
