#include "TessellationShader.h"

// Constructor just forwards to init
TessellationShader::TessellationShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    // Possible config readable
    initShader(
        L"tess_vs.cso",   // vertex
        L"tess_hs.cso",   // hull (tess control)
        L"tess_ds.cso",   // domain (tess eval)
        L"tess_ps.cso"    // pixel shader
    );
}

void TessellationShader::initShader(
    const wchar_t* vsFilename,
    const wchar_t* hsFilename,
    const wchar_t* dsFilename,
    const wchar_t* psFilename)
{
    // Allocate a buffer for MVP matrices
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;          
    bufDesc.ByteWidth = sizeof(MatrixBufferType);  // MVP only
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(renderer->CreateBuffer(&bufDesc, nullptr, &matrixBuffer))) 
    {
        OutputDebugStringA("Failed to create tessellation matrix buffer\n");
    }

    // Load shaders 
    loadVertexShader(vsFilename);
    loadHullShader(hsFilename);
    loadDomainShader(dsFilename);
    loadPixelShader(psFilename);
}

void TessellationShader::setShaderParameters(
    ID3D11DeviceContext* deviceContext,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix)
{
    // Needed to make a work around as DXMath is row-major, HLSL wants column-major...
    XMMATRIX w = XMMatrixTranspose(worldMatrix);
    XMMATRIX v = XMMatrixTranspose(viewMatrix);
    XMMATRIX p = XMMatrixTranspose(projectionMatrix);

    // Map constant buffer so GPU can read it
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        auto* data = reinterpret_cast<MatrixBufferType*>(mapped.pData);
        data->world = w;
        data->view = v;
        data->projection = p;
        deviceContext->Unmap(matrixBuffer, 0);
    }
    else
    {
        OutputDebugStringA("Failed to map tessellation matrix buffer\n");
    }

    // Bind buffer to both VS + DS
    deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
    deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);
}
