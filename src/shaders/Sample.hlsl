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

// R2 序列（黄金比例序列）常量
static const float GOLDEN_RATIO = 1.32471795724474602596f;
static const float R2_A1 = 0.7548776662466927f; // 1/φ
static const float R2_A2 = 0.5698402909980532f; // 1/φ²

// 生成第 index 个 R2 采样点，返回 [0,1)² 区间
float2 R2Sequence(uint index)
{
    float2 v;
    v.x = frac(0.5f + R2_A1 * (float) (index + 1));
    v.y = frac(0.5f + R2_A2 * (float) (index + 1));
    return v;
}
float2 R2Sequence(uint index, float2 offset)
{
    float2 v;
    v.x = frac(0.5f + R2_A1 * (float) (index + 1) + offset.x);
    v.y = frac(0.5f + R2_A2 * (float) (index + 1) + offset.y);
    return v;
}
// 使用像素坐标和帧号作为种子，得到每像素每帧稳定的低差异采样
float2 R2SequencePerPixel(uint2 pixelCoord, uint frameIndex, float2 offset)
{
    // 把像素坐标和帧号组合成一个索引，避免像素间出现相关性
    uint index = pixelCoord.x + pixelCoord.y * 1920u + frameIndex * 1920u * 1080u;
    return R2Sequence(index, offset);
}

// Radical inverse base 2 (van der Corput)
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 1 / 2^32
}

// Hammersley: i = sample index, N = total sample count
float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

#endif