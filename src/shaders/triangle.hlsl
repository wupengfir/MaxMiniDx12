struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
};
struct PSInput 
{
    float4 pos : SV_POSITION; // 系统值：裁剪空间位置
    float3 color : COLOR;
};   
PSInput VS(VSInput input) 
{
    PSInput o;
    o.pos = float4(input.pos, 1.0); // 直接用裁剪空间坐标，暂不做矩阵变换
    o.color = input.color;
    return o;
}
float4 PS(PSInput input) : SV_TARGET 
{
    return float4(input.color, 1.0); // 顶点颜色会被光栅化插值到每个像素
}
