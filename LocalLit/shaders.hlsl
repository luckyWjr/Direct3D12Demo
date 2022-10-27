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
    // ���������������µ�λ��
    float4 positionInWorld = mul(float4(position, 1.0f), modelMatrix);
    result.positionInWorld = positionInWorld.xyz;
    // ����ķ��������������µ�ֵ
    result.normalInWorld = mul(normal, (float3x3)modelMatrix);
    // ��������βü��ռ��µ�λ��
    result.position = mul(positionInWorld, mul(viewMatrix, projectMatrix));

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // ��һ����������
    input.normalInWorld = normalize(input.normalInWorld);
// ���浽camera�ĵ�λ����
float3 toCamera = normalize(cameraPositionInWorld - input.positionInWorld);
// �������գ�ģ���ӹ�
float4 ambient = ambientLight * albedo;
Material material = { albedo, fresnelR0, roughness };
// ֱ�ӹ⣬����������;��淴��
float4 directLight = ComputeLighting(lights, material, input.positionInWorld, input.normalInWorld, toCamera);
float4 litColor = ambient + directLight;
litColor.a = albedo.a;
return litColor;
}