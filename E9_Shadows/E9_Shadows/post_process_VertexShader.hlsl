// Post-process VS
// passes through position + UV for a fullscreen quad

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

VS_OUT main(VS_IN input)
{
    VS_OUT o;
    float4 worldPos = mul(float4(input.pos, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    o.pos = mul(viewPos, projectionMatrix);
    o.uv = input.uv;
    return o;
}