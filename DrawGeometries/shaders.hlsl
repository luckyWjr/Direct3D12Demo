struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer passData : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectMatrix;
}

cbuffer objectData : register(b1)
{
    float4x4 modelMatrix;
}

PSInput VSMain(float3 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    float4x4 mvpMatrix = mul(modelMatrix, mul(viewMatrix, projectMatrix));
    result.position = mul(float4(position, 1.0f), mvpMatrix);
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
