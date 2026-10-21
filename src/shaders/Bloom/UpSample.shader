#include "../Common.hlsl"

Texture2D _SourceTexLowMip;
float4 _BloomParams;
#define Scatter             _BloomParams.x
#define ClampMax            _BloomParams.y
#define Threshold           _BloomParams.z
#define ThresholdKnee       _BloomParams.w

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

half3 Upsample(float2 uv)
{
    half3 highMip = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, uv));
    half3 lowMip = DecodeHDR(SAMPLE_TEXTURE2D_X(_SourceTexLowMip, sampler_LinearClamp, uv));
    return lerp(highMip, lowMip, frac(Scatter));
}

float4 PS(PSInput input) : SV_TARGET
{
    input.uv.y = 1 - input.uv.y;
    float2 uv = input.uv.xy;
    half3 color = Upsample(uv);
    return float4(color, 1);
}
