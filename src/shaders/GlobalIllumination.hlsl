#ifndef GLOBAL_ILLUMINATION
#define GLOBAL_ILLUMINATION

struct GIInput
{
    float3 diffuseIrradiance;
    float3 reflecIrradiance;
    float2 iblBrdf;
};

float3 PBR_GlobalIllumination(float3 diffuse, float3 specular , GIInput giInput)
{  
    // IBL环境光照贡献
    float3 diffuseIBL = giInput.diffuseIrradiance * diffuse;
    float3 specularIBL = giInput.reflecIrradiance * (specular * giInput.iblBrdf.x + giInput.iblBrdf.y);
    return diffuseIBL + specularIBL;
}

#endif