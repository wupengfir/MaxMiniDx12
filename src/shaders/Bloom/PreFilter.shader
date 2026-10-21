#include "../Common.hlsl"



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

float4 PS(PSInput input) : SV_TARGET
{
    input.uv.y = 1 - input.uv.y;
    float4 color = SAMPLE_TEXTURE2D_X(_SourceTex, sampler_LinearClamp, input.uv.xy);
    
    // User controlled clamp to limit crazy high broken spec
    color = min(ClampMax, color);

            // Thresholding
    half brightness = Max3(color.r, color.g, color.b);
    half softness = clamp(brightness - Threshold + ThresholdKnee, 0.0, 2.0 * ThresholdKnee);
    softness = (softness * softness) / (4.0 * ThresholdKnee + 1e-4);
    half multiplier = max(brightness - Threshold, softness) / max(brightness, 1e-4);
    color *= multiplier;

            // Clamp colors to positive once in prefilter. Encode can have a sqrt, and sqrt(-x) == NaN. Up/Downsample passes would then spread the NaN.
    color = max(color, 0);  
    return float4(color.xyz * floor(_BloomParams.x), 1.0);
}
