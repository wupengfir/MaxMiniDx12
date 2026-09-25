#ifndef SAMPLE
#define SAMPLE
#include "BRDF.hlsl"
float3 GetDirByIndex(float n, float N)
{
    float phi = 0.618;
    float z = (2 * n - 1) / N - 1;
    float a = sqrt(1 - z * z);
    float x = a * cos(2 * PI * n * phi);
    float y = a * sin(2 * PI * n * phi);
    return float3(x, y, z);
}
//pdf = dot(L, N) / PI
float3 SampleCosineHemisphere(float2 xi, float3 N)
{
    // 1. 在局部切线空间，生成余弦加权半球方向
    float r = sqrt(xi.x);
    float phi = 2.0f * 3.1415926535f * xi.y;

    float3 dirTS;
    dirTS.x = r * cos(phi);
    dirTS.y = r * sin(phi);
    dirTS.z = sqrt(max(1.0f - xi.x, 0.0f));

    // 2. 构建 TBN 矩阵：切线空间 → 世界空间
    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    // 3. 转到世界空间
    return normalize(mul(dirTS, TBN));
}
//PDF：` pdf = 1.0f / (2.0f * PI)`
float3 SampleUniformHemisphere(float2 xi, float3 N)
{
    float z = xi.x;
    float r = sqrt(max(1.0f - z * z, 0.0f));
    float phi = 2.0f * 3.1415926535f * xi.y;

    float3 dirTS;
    dirTS.x = r * cos(phi);
    dirTS.y = r * sin(phi);
    dirTS.z = z;

    // TBN 转到世界空间
    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(dirTS, TBN));
}

// hash21 生成 [0,1] 伪随机数
float hash21(float2 p)
{
    p = frac(p * float2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return frac(p.x * p.y * 456.23);
}

// 生成一对随机数
float2 hash22(float2 p)
{
    p = frac(p * float2(234.34, 435.345));
    p += dot(p, p.yx + 34.23);
    return frac(float2(p.x * p.y, p.x + p.y));
}
#endif