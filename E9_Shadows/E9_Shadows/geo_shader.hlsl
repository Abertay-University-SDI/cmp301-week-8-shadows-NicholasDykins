
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
};


struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
};

// Vertex Shader 
VS_OUT VS_main(float3 pos : POSITION, float3 norm : NORMAL)
{
    VS_OUT o;

    // world transform
    float4 wpos = mul(float4(pos, 1.0f), worldMatrix);

    // MVP multiply
    o.pos = mul(mul(wpos, viewMatrix), projMatrix);

    // transform normal (ignore w)
    o.norm = normalize(mul(norm, (float3x3)worldMatrix));

    return o;
}


struct GS_OUT
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
};

// Geometry Shader
// Takes in one triangle and spits out 2: original + extruded spike
[maxvertexcount(6)]
void GS_main(triangle VS_OUT inTri[3], inout TriangleStream<GS_OUT> outTri)
{
    float extrude = 0.2f; // how far to push along normal

    // emit original tri
    [unroll]
    for (int i = 0; i < 3; i++)
    {
        GS_OUT v;
        v.pos = inTri[i].pos;
        v.norm = inTri[i].norm;
        outTri.Append(v);
    }
    outTri.RestartStrip(); // new strip

    // emit the spiked tri
    [unroll]
    for (int i = 0; i < 3; i++)
    {
        GS_OUT v;

        // quick offset in clip space along normal
        float4 spikePos = inTri[i].pos + float4(inTri[i].norm * extrude, 0);

        v.pos = spikePos;
        v.norm = inTri[i].norm; // same normal, no fancy smoothing
        outTri.Append(v);
    }
    outTri.RestartStrip(); 
}

// Pixel Shader 
// colors the spikes
float4 PS_main(GS_OUT input) : SV_TARGET
{
    // flat orange
    return float4(1.0f, 0.5f, 0.2f, 1.0f);
}
