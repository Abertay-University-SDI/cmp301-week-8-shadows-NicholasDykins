#include "PostProcessShader.h"

// Post-process shader, renders fullscreen quad with a texture
PostProcessShader::PostProcessShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    //initShader(L"post_process_VertexShader.cso", L"post_process_PixelShader.cso");
}

void PostProcessShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
    // MVP buffer
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.ByteWidth = sizeof(MatrixBufferType);
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(renderer->CreateBuffer(&bufDesc, nullptr, &matrixBuffer))) 
    {
        OutputDebugStringA("PostProcessShader: Failed to create MVP buffer\n");
    }

    // clamped because I don't want wrapping on screen quads
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samp.MinLOD = 0;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    renderer->CreateSamplerState(&samp, &sampleState);

    // load basic VS+PS
    loadVertexShader(vsFilename);
    loadPixelShader(psFilename);
}

void PostProcessShader::setShaderParameters(
    ID3D11DeviceContext* deviceContext,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix,
    ID3D11ShaderResourceView* sceneTexture
)
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mappedResource;

    // === Update matrix buffer ===
    MatrixBufferType* dataPtr;
    XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
    XMMATRIX tview = XMMatrixTranspose(viewMatrix);
    XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

    result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result)) return;

    dataPtr = (MatrixBufferType*)mappedResource.pData;
    dataPtr->world = tworld;
    dataPtr->view = tview;
    dataPtr->projection = tproj;

    deviceContext->Unmap(matrixBuffer, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

    // === Bind scene texture ===
    deviceContext->PSSetShaderResources(0, 1, &sceneTexture);

    // === Bind sampler ===
    deviceContext->PSSetSamplers(0, 1, &sampleState);
}
