#pragma once
#include "BaseShader.h"
#include "Light.h"

class MyLightShader : public BaseShader
{
public:
    MyLightShader(ID3D11Device* device, HWND hwnd);
    ~MyLightShader();


    void setShaderParameters(
        ID3D11DeviceContext* deviceContext,
        const XMMATRIX& worldMatrix,
        const XMMATRIX& viewMatrix,
        const XMMATRIX& projectionMatrix,
        ID3D11ShaderResourceView* texture,
        ID3D11ShaderResourceView* shadowMap,
        Light* light
    );

    void render(ID3D11DeviceContext* deviceContext, int indexCount) override;

private:
    void initShader(const wchar_t* vsFilename, const wchar_t* psFilename) override;

    ID3D11Device* myDevice;
    ID3D11Buffer* matrixBuffer;
    ID3D11Buffer* lightBuffer;
    ID3D11SamplerState* sampleState;
    ID3D11SamplerState* shadowSamplerState;


    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX lightView;
        XMMATRIX lightProjection;
    };

    struct LightBufferType
    {
        XMFLOAT4 ambient;
        XMFLOAT4 diffuse;
        XMFLOAT3 lightDirection;
        float padding; // 16-byte HLSL alignment
    };
};
