#ifndef COMMON
#define COMMON

#define Matrix_MVP mul(_Matrix_M , _Matrix_VP)

cbuffer PerCamera : register(b0)
{
	float4 _CameraPos;
	row_major  float4x4 _Matrix_VP;
}
cbuffer PerDraw : register(b1)
{
	row_major  float4x4 _Matrix_M;
	row_major  float4x4 _Matrix_M_I;
}
cbuffer PerFrame : register(b2)
{
	float4 _MainLightDirection;
	float4 _MainLightColor;
}
#endif


// ACES Filmic 拟合 Tonemap (Hill‑Hejl)
// input: linear HDR rgb (>=0，线性空间，ACES‑CG范围)
// return: [0,1] 线性RGB，之后需要做gamma编码
float3 ACESFilmic(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x + b)) / (x*(c*x + d) + e));
}

// 亮度分离版本：只对亮度做tonemap，保留色彩饱和度
float3 ACESFilmicLumaPreserve(float3 color)
{
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float3 mapped = ACESFilmic(color);
    float lumaMapped = dot(mapped, float3(0.2126, 0.7152, 0.0722));
    return mapped * (luma / lumaMapped);
}