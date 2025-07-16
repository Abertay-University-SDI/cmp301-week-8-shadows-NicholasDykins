
// ambient + diffuse with shadows
// Structs 
struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float3 norm : NORMAL;
};

struct PS_IN
{
    float4 pos      : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 norm     : NORMAL;
    float3 worldPos : TEXCOORD1;
    float4 shadowPos : TEXCOORD2; // light-space for shadow map
};

// Buffers
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
    matrix lightViewProj; // combined light view * proj
};

cbuffer LightBuffer : register(b1)
{
    float4 amb;
    float4 diff;
    float3 lightDir;
    float pad; // alignment
};

// Textures
Texture2D tex0      : register(t0); // surface texture
SamplerState samp0  : register(s0);

Texture2D shadowTex : register(t1); // shadow depth map
SamplerComparisonState shadowSamp : register(s1);

// Vertex Shader 
PS_IN VS(VS_IN input)
{
    PS_IN o;

    // transform to world
    float4 world = mul(float4(input.pos, 1.0f), worldMatrix);

    // MVP
    float4 viewPos = mul(world, viewMatrix);
    o.pos = mul(viewPos, projMatrix);

    // store world pos for possible specular calc (not used now)
    o.worldPos = world.xyz;

    // transform normal (ignore translation)
    o.norm = mul(float4(input.norm, 0.0f), worldMatrix).xyz;

    // pass uv
    o.uv = input.uv;

    // shadow projection (NDC)
    o.shadowPos = mul(world, lightViewProj);

    return o;
}

// Pixel Shader 
float4 PS(PS_IN input) : SV_TARGET
{
    // normal + light direction
    float3 n = normalize(input.norm);
    float3 l = normalize(-lightDir); // directional light

    // Lambert diffuse
    float diffFactor = max(dot(n, l), 0.0f);
    float3 diffCol = diff.rgb * diffFactor;

    // Shadow calculation
    float2 shadowUV = input.shadowPos.xy / input.shadowPos.w * 0.5f + 0.5f;
    float shadowDepth = input.shadowPos.z / input.shadowPos.w;

    float shadowFactor = 1.0f; // default fully lit

    // bias to avoid acne
    float bias = 0.005f;

    // only sampling shadow if in light frustum
    if (shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
        shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
    {
        // PCF compare sample  returns 0 = shadow, 1 = lit
        shadowFactor = shadowTex.SampleCmpLevelZero(
            shadowSamp,
            shadowUV,
            shadowDepth - bias
        );
    }

    // combine ambient + diffuse * shadow
    float3 litColor = amb.rgb + shadowFactor * diffCol;

    // sample base texture
    float4 texColor = tex0.Sample(samp0, input.uv);

    // final color = lighting * texture
    return float4(litColor * texColor.rgb, texColor.a);
}



        //float3 viewDir = normalize(-input.worldPos);
        //float3 halfway = normalize(l + viewDir);
       // float spec = pow(max(dot(n, halfway), 0.0f), 32.0f);


