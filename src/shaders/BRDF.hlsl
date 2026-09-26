#ifndef BRDF
#define BRDF
#define PI 3.14159265359
#include "GlobalIllumination.hlsl"
//====================================================================
// UE PBR 核心函数
//====================================================================

// GGX NDF 法线分布函数 D (Trowbridge-Reitz)
float D_GGX(float NoH, float a)
{
    float a2 = a * a;
    float denom = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / max(1e-6, (PI * denom * denom));
}

// Smith-Schlick 几何项 G，UE 对GGX的优化版本
float G_SchlickGGX(float NoV, float k)
{
    return NoV / (NoV * (1.0 - k) + k);
}

// Smith 联合几何 G = G1(NoV)*G1(NoL)
float G_Smith(float NoV, float NoL, float roughness)
{
    // UE 直接光：k = roughness/2
    float k = roughness * 0.5;
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

// Schlick Fresnel F
float3 F_Schlick(float VoH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

// Burley Disney Diffuse (UE DefaultLit 漫反射，不是Lambert)
float3 Diffuse_Burley(float3 baseColor, float roughness, float NoV, float NoL, float VoH)
{
    float fd90 = 0.5 + 2.0 * VoH * VoH * roughness;
    float f0 = 1.0;
    float lightScatter = f0 + (fd90 - f0) * pow(1.0 - NoL, 5.0);
    float viewScatter = f0 + (fd90 - f0) * pow(1.0 - NoV, 5.0);
    return baseColor / PI * lightScatter * viewScatter;
}

// Cook-Torrance Specular 镜面BRDF
float3 Specular_CookTorrance(float3 F0, float roughness, float NoV, float NoL, float NoH, float VoH)
{
    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    float3 F = F_Schlick(VoH, F0);

    float spec = D * G / (4.0 * NoV * NoL + 0.001); // +eps防除0
    return spec * F;
}

// UE 主BRDF入口：金属粗糙度
float3 UE_PBR_BRDF(float3 BaseColor, float Metallic, float Roughness,
                   float3 N, float3 V, float3 L,float3 irradiance,GIInput gi)
{
    Roughness = max(Roughness, 0.025); // UE最小粗糙度，防止高光奇点

    float3 H = normalize(V + L);

    float NoV = saturate(dot(N, V));
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    // 金属度工作流：F0 lerp(0.04, BaseColor, Metallic)
    float3 F0 = lerp(0.04, BaseColor, Metallic);
    float3 diffColor = BaseColor * (1.0 - Metallic); // 金属漫反射=0

    float3 diffuse = Diffuse_Burley(diffColor, Roughness, NoV, NoL, VoH);
    float3 specular = Specular_CookTorrance(F0, Roughness, NoV, NoL, NoH, VoH);
    float3 ibl = PBR_GlobalIllumination(diffColor, F0, gi);
    return (diffuse + specular) * NoL * irradiance + ibl;
}
#endif