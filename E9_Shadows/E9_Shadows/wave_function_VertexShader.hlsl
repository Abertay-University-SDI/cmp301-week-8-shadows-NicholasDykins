
// Moves vertices up anddown with a sine wave

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix; 
};

cbuffer TimeBuffer : register(b1)
{
    float time;
    float amplitude;
    float frequency;
    float pad; // for alignment
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT outp;

    // sine offset makes it bounce on the Y axis
    float wave = sin(input.pos.x * frequency + time) * amplitude;

    // only modify Y position
    float3 moved = float3(input.pos.x, input.pos.y + wave, input.pos.z);

    // MVP (world * view * projection)
    float4 world = mul(float4(moved, 1.0f), worldMatrix);
    float4 view = mul(world, viewMatrix);
    outp.pos = mul(view, projMatrix);

    // pass through UVs as-is
    outp.uv = input.uv;

    return outp;
}
