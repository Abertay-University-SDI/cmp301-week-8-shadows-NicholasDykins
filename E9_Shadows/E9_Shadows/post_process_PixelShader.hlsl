
Texture2D sceneTex : register(t0);   // the render texture
SamplerState samp0 : register(s0);  // basic sampler

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_IN input) : SV_TARGET
{
    
    float2 texelSize = float2(1.0 / 1366.0, 1.0 / 768.0);

    float3 sample[9];
    int idx = 0;

  
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            sample[idx++] = sceneTex.Sample(samp0, input.uv + float2(x, y) * texelSize).rgb;
        }
    }

    // Sobel kernels
    float3 gx = sample[2] + 2.0 * sample[5] + sample[8]
              - sample[0] - 2.0 * sample[3] - sample[6];

    float3 gy = sample[0] + 2.0 * sample[1] + sample[2]
              - sample[6] - 2.0 * sample[7] - sample[8];

    // Edge intensity
    float edge = length(gx + gy);

    // Output edge map as grayscale
    return float4(edge, edge, edge, 1.0);
}
