// Defaults for number of lights.
#ifndef NUM_DIRECT_LIGHTS
#define NUM_DIRECT_LIGHTS 1
#endif
#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif
#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "../CommonShader/LightingUtil.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float3 positionInWorld : POSITION;
    float3 normalInWorld : NORMAL;
};

cbuffer passData : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectMatrix;
    float3 cameraPositionInWorld;
    float padding0;
    float4 ambientLight;
    Light lights[MaxLights];
}

cbuffer objectData : register(b1)
{
    float4x4 modelMatrix;
}

cbuffer materialData : register(b2)
{
    float4 albedo;
    float3 fresnelR0;
    float roughness;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL)
{
    PSInput result;
    // 顶点在世界坐标下的位置
    float4 positionInWorld = mul(float4(position, 1.0f), modelMatrix);
    result.positionInWorld = positionInWorld.xyz;
    // 顶点的法线在世界坐标下的值
    result.normalInWorld = mul(normal, (float3x3)modelMatrix);
    // 顶点在齐次裁剪空间下的位置
    result.position = mul(positionInWorld, mul(viewMatrix, projectMatrix));

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // 归一化法线向量
    input.normalInWorld = normalize(input.normalInWorld);
// 表面到camera的单位向量
float3 toCamera = normalize(cameraPositionInWorld - input.positionInWorld);
// 环境光照，模拟间接光
float4 ambient = ambientLight * albedo;
Material material = { albedo, fresnelR0, roughness };
// 直接光，包含漫反射和镜面反射
float4 directLight = ComputeLighting(lights, material, input.positionInWorld, input.normalInWorld, toCamera);
float4 litColor = ambient + directLight;
litColor.a = albedo.a;
return litColor;
}