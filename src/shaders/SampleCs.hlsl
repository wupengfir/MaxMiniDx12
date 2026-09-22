// 可读写结构化缓冲区，UAV，register u0
RWStructuredBuffer<float3> g_OutputBuffer : register(u0);

cbuffer CB : register(b0)
{
    float g_DeltaTime;
}

// 线程组大小 [numthreads(GroupX,GroupY,GroupZ)]
// 常用64/128，适配GPU warp/wavefront
[numthreads(64,1,1)]
void CSMain(uint3 groupThreadID : SV_GroupThreadID, uint3 dispatchID : SV_DispatchThreadID)
{
    uint idx = dispatchID.x;
    g_OutputBuffer[idx] = float3(idx * 0.1f, g_DeltaTime+1.2, 0);
}
