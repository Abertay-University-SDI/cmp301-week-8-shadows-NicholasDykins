#pragma once
#include "BaseShader.h"

// renders a texture on a quad
class PostProcessShader : public BaseShader
{
public:
    PostProcessShader(ID3D11Device* device, HWND hwnd);
    ~PostProcessShader() 
    {
        if (sampleState)
        {
            sampleState->Release();
        }

        if (matrixBuffer)
        {
            matrixBuffer->Release();
        }
    }


    void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);

    void setShaderParameters(
        ID3D11DeviceContext* ctx,
        const XMMATRIX& world,
        const XMMATRIX& view,
        const XMMATRIX& proj,
        ID3D11ShaderResourceView* tex);

private:
    ID3D11Buffer* matrixBuffer = nullptr;    // ortho MVP
    ID3D11SamplerState* sampleState = nullptr;
};
