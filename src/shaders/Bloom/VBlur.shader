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
    float texelSize = _SourceTex_TexelSize.y * 1.0;
    float2 uv = input.uv.xy;

    half3 c0 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(0.0, texelSize * 3.23076923)));
    half3 c1 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv - float2(0.0, texelSize * 1.38461538)));
    half3 c2 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv));
    half3 c3 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(0.0, texelSize * 1.38461538)));
    half3 c4 = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv + float2(0.0, texelSize * 3.23076923)));

    half3 color = c0 * 0.07027027 + c1 * 0.31621622
                        + c2 * 0.22702703
                        + c3 * 0.31621622 + c4 * 0.07027027;

    return float4(color, 1);
}
