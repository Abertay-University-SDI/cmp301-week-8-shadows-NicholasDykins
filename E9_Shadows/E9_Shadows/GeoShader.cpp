#include "GeoShader.h"

// Load VS + GS + PS
GeoShader::GeoShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    initShader(
        L"geo_vs.cso",  // vertex pass-through
        L"geo_gs.cso",  // geometry stage adds stuff
        L"geo_ps.cso"   // pixel shader
    );
}

void GeoShader::initShader(
    const wchar_t* vsFilename,
    const wchar_t* gsFilename,
    const wchar_t* psFilename)
{
    // allocate MVP constant buffer
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.ByteWidth = sizeof(MatrixBufferType);
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(renderer->CreateBuffer(&bufDesc, nullptr, &matrixBuffer))) 
    {
        OutputDebugStringA("GeoShader: Failed to create matrix buffer\n");
    }

    loadVertexShader(vsFilename);
    loadGeometryShader(gsFilename);
    loadPixelShader(psFilename);
}

void GeoShader::setShaderParameters(
    ID3D11DeviceContext* ctx,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projMatrix)
{
    // DXMath is row-major, HLSL defaults to column-major. Transpose time!
    XMMATRIX w = XMMatrixTranspose(worldMatrix);
    XMMATRIX v = XMMatrixTranspose(viewMatrix);
    XMMATRIX p = XMMatrixTranspose(projMatrix);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        auto* data = reinterpret_cast<MatrixBufferType*>(mapped.pData);
        data->world = w;
        data->view = v;
        data->projection = p;
        ctx->Unmap(matrixBuffer, 0);
    }
    else 
    {
        OutputDebugStringA("GeoShader: Failed to map matrix buffer\n");
    }

    // Bind it to VS + GS
    ctx->VSSetConstantBuffers(0, 1, &matrixBuffer);
    ctx->GSSetConstantBuffers(0, 1, &matrixBuffer);
}
