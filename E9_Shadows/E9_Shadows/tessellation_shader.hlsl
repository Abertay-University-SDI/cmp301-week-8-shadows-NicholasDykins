

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
};

// Tessellation factor buffer – needs 16-byte align or HLSL complains
cbuffer TessFactorBuffer : register(b1)
{
    float tessFactor;
    float pad[3];
};

// Vertex Shader 
// forwards position + normal

struct VS_INPUT
{
    float3 pos   : POSITION;
    float3 norm  : NORMAL;
};

struct VS_OUTPUT
{
    float3 pos   : POSITION;
    float3 norm  : NORMAL;
};

VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT o;
    o.pos = input.pos;
    o.norm = input.norm;
    return o;
}

// Hull Shader 
// Outputs control points unchanged, sets tessellation factors

struct HS_CP_OUT // control point
{
    float3 pos  : POSITION;
    float3 norm : NORMAL;
};

struct HS_CONST_OUT // tess factors
{
    float edge[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

// HS main
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConst")]
HS_CP_OUT HS_main(InputPatch<VS_OUTPUT, 3> patch, uint i : SV_OutputControlPointID)
{
    HS_CP_OUT o;
    o.pos = patch[i].pos;
    o.norm = patch[i].norm;
    return o;
}


HS_CONST_OUT HSConst(InputPatch<VS_OUTPUT, 3> patch)
{
    HS_CONST_OUT o;

    // same tess factor for all edges
    [unroll]
    for (int e = 0; e < 3; e++)
        o.edge[e] = tessFactor;

    o.inside = tessFactor;
    return o;
}

// Domain Shader
// Interpolates barycentric coords, transforms to world/clip space

struct DS_OUT
{
    float4 pos  : SV_POSITION;
    float3 norm : NORMAL;
};

[domain("tri")]
DS_OUT DS_main(
    HS_CONST_OUT patchConst,
    const OutputPatch<HS_CP_OUT, 3> patch,
    float3 bary : SV_DomainLocation)
{
    DS_OUT o;

    // barycentric interp for positions
    float3 pos =
        patch[0].pos * bary.x +
        patch[1].pos * bary.y +
        patch[2].pos * bary.z;

    // barycentric interp for normals
    float3 norm =
        patch[0].norm * bary.x +
        patch[1].norm * bary.y +
        patch[2].norm * bary.z;


    float4 worldPos = mul(float4(pos, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    o.pos = mul(viewPos, projMatrix);

    // transform normal 
    o.norm = normalize(mul(norm, (float3x3)worldMatrix));

    return o;
}

// Pixel Shader 
// Debug shading 
float4 PS_main(DS_OUT input) : SV_TARGET
{
    float3 up = float3(0, 1, 0); 
    float lighting = saturate(dot(normalize(input.norm), normalize(up)));
    return float4(1.0f, 0.6f, 0.3f, 1.0f) * lighting;
}
