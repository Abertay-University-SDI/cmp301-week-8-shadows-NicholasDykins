#include "WaveFunctionShader.h"

// Shader that wiggles stuff over time via sine displacement
WaveFunctionShader::WaveFunctionShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    initShader(
        L"wave_function_VertexShader.cso",
        L"wave_function_PixelShader.cso"
    );
}

void WaveFunctionShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
    // MVP buffer 
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.ByteWidth = sizeof(MatrixBufferType);
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(renderer->CreateBuffer(&bufDesc, nullptr, &matrixBuffer))) 
    {
        OutputDebugStringA("WaveShader: Failed to create MVP buffer\n");
    }

    // time/amplitude/freq buffer
    D3D11_BUFFER_DESC timeBuf = {};
    timeBuf.Usage = D3D11_USAGE_DYNAMIC;
    timeBuf.ByteWidth = sizeof(TimeBufferType);
    timeBuf.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    timeBuf.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(renderer->CreateBuffer(&timeBuf, nullptr, &timeBuffer))) 
    {
        OutputDebugStringA("WaveShader: Failed to create time buffer\n");
    }

    // wrap sampler
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    renderer->CreateSamplerState(&samp, &sampleState);

    // load VS + PS
    loadVertexShader(vsFilename);
    loadPixelShader(psFilename);
}

void WaveFunctionShader::setShaderParameters(
    ID3D11DeviceContext* ctx,
    const XMMATRIX& world,
    const XMMATRIX& view,
    const XMMATRIX& proj,
    ID3D11ShaderResourceView* texture,
    float time,
    float amplitude,
    float frequency)
{
    // Transpose MVP
    XMMATRIX w = XMMatrixTranspose(world);
    XMMATRIX v = XMMatrixTranspose(view);
    XMMATRIX p = XMMatrixTranspose(proj);

    // Update MVP buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        auto* data = reinterpret_cast<MatrixBufferType*>(mapped.pData);
        data->world = w;
        data->view = v;
        data->projection = p;
        ctx->Unmap(matrixBuffer, 0);
    }

    ctx->VSSetConstantBuffers(0, 1, &matrixBuffer);

    // Update wave time params
    if (SUCCEEDED(ctx->Map(timeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        auto* t = reinterpret_cast<TimeBufferType*>(mapped.pData);
        t->time = time;
        t->amplitude = amplitude;
        t->frequency = frequency;
        t->padding = 0.0f; // HLSL alignment thing
        ctx->Unmap(timeBuffer, 0);
    }

    // Bind time buffer after MVP
    ctx->VSSetConstantBuffers(1, 1, &timeBuffer);

    // PS needs texture + sampler
    ctx->PSSetShaderResources(0, 1, &texture);
    ctx->PSSetSamplers(0, 1, &sampleState);







}
