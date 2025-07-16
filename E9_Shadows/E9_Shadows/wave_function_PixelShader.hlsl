
//samples the texture 
Texture2D tex0 : register(t0);       // main texture
SamplerState samp0 : register(s0);   // basic sampler

struct PS_INPUT
{
    float4 pos : SV_POSITION;  // screen position
    float2 uv  : TEXCOORD0;    // UV from vertex shader
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // just sample the texture at given UV
    float4 col = tex0.Sample(samp0, input.uv);

    // a wavy floor
    return col;
}
