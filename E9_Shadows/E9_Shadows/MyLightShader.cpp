#include "MyLightShader.h"

// Custom lighting shader w/ shadow map sampling
MyLightShader::MyLightShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    myDevice = device;
    initShader(
        L"my_light_shader_vs.cso",
        L"my_light_shader_ps.cso"
    );
}

MyLightShader::~MyLightShader()
{
    if (sampleState) sampleState->Release();
    if (shadowSamplerState) shadowSamplerState->Release();
    if (matrixBuffer) matrixBuffer->Release();
    if (lightBuffer) lightBuffer->Release();
    BaseShader::~BaseShader();
}

void MyLightShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
    // Load shaders
    loadVertexShader(vsFilename);
    loadPixelShader(psFilename);

    // MVP + light matrices
    D3D11_BUFFER_DESC mvpBuf = {};
    mvpBuf.Usage = D3D11_USAGE_DYNAMIC;
    mvpBuf.ByteWidth = sizeof(MatrixBufferType);
    mvpBuf.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    mvpBuf.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    myDevice->CreateBuffer(&mvpBuf, nullptr, &matrixBuffer);

    // Light params buffer
    D3D11_BUFFER_DESC lightBuf = {};
    lightBuf.Usage = D3D11_USAGE_DYNAMIC;
    lightBuf.ByteWidth = sizeof(LightBufferType);
    lightBuf.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBuf.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    myDevice->CreateBuffer(&lightBuf, nullptr, &lightBuffer);

    // Texture sampler for diffuse maps
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    myDevice->CreateSamplerState(&samp, &sampleState);

    // Shadow sampler: border color = white so outside shadow = lit
    D3D11_SAMPLER_DESC shadow = {};
    shadow.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    shadow.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    shadow.BorderColor[0] = 1.f;
    shadow.BorderColor[1] = 1.f;
    shadow.BorderColor[2] = 1.f;
    shadow.BorderColor[3] = 1.f;
    shadow.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    myDevice->CreateSamplerState(&shadow, &shadowSamplerState);
}

void MyLightShader::setShaderParameters(
    ID3D11DeviceContext* deviceContext,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix,
    ID3D11ShaderResourceView* texture,
    ID3D11ShaderResourceView* shadowMap,
    Light* light
)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;

    // === MATRIX BUFFER ===
    MatrixBufferType* dataPtr;
    XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
    XMMATRIX tview = XMMatrixTranspose(viewMatrix);
    XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);
    XMMATRIX tlightView = XMMatrixTranspose(light->getViewMatrix());
    XMMATRIX tlightProj = XMMatrixTranspose(light->getOrthoMatrix());

    if (SUCCEEDED(deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        dataPtr = (MatrixBufferType*)mappedResource.pData;
        dataPtr->world = tworld;
        dataPtr->view = tview;
        dataPtr->projection = tproj;
        dataPtr->lightView = tlightView;
        dataPtr->lightProjection = tlightProj;
        deviceContext->Unmap(matrixBuffer, 0);
    }

    // Bind matrix buffer to correct slot (b0)
    deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

    // === LIGHT BUFFER ===
    LightBufferType* lightPtr;
    if (SUCCEEDED(deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        lightPtr = (LightBufferType*)mappedResource.pData;
        lightPtr->ambient = light->getAmbientColour();
        lightPtr->diffuse = light->getDiffuseColour();
        lightPtr->lightDirection = light->getDirection();
        lightPtr->padding = 0.f; // maintain alignment
        deviceContext->Unmap(lightBuffer, 0);
    }

    // Bind light buffer to correct slot (b1)
    deviceContext->PSSetConstantBuffers(1, 1, &lightBuffer);

    // === TEXTURES ===
    deviceContext->PSSetShaderResources(0, 1, &texture);    // Diffuse texture
    deviceContext->PSSetShaderResources(1, 1, &shadowMap);  // Shadow map

    // === SAMPLERS ===
    deviceContext->PSSetSamplers(0, 1, &sampleState);
    deviceContext->PSSetSamplers(1, 1, &shadowSamplerState);
}

void MyLightShader::render(ID3D11DeviceContext* deviceContext, int indexCount)
{
    BaseShader::render(deviceContext, indexCount);
}
