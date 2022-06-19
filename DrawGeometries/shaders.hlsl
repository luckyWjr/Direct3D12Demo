struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float3 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    float4x4 mvp = float4x4(0.7795, 0.8776, 0.8269, 0.8261,
        0.0000, 2.2097, -0.4031, -0.4027,
        -1.6342, 0.4185, 0.3944, 0.3940,
        0.0000, 0.0000, 4.0040, 5.0000);

    result.position = mul(float4(position, 1.0f), mvp);
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
