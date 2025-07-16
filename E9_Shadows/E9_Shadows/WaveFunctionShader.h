#pragma once
#include "BaseShader.h"

// Wobbles vertices based on time
class WaveFunctionShader : public BaseShader
{
public:
    WaveFunctionShader(ID3D11Device* device, HWND hwnd);
    ~WaveFunctionShader()
    {
        if (sampleState)
        {
            sampleState->Release();
        }
        if (matrixBuffer)
        {
            matrixBuffer->Release();
        }
        if (timeBuffer)
        {
            timeBuffer->Release();
        }
    }

    void setShaderParameters(
        ID3D11DeviceContext* ctx,
        const XMMATRIX& world,
        const XMMATRIX& view,
        const XMMATRIX& proj,
        ID3D11ShaderResourceView* tex,
        float time,
        float amplitude,
        float frequency);

private:
    void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);

    ID3D11Buffer* matrixBuffer = nullptr;  // MVP
    ID3D11Buffer* timeBuffer = nullptr;    // time/amplitude/frequency
    ID3D11SamplerState* sampleState = nullptr;

    struct TimeBufferType {
        float time;
        float amplitude;
        float frequency;
        float padding; // for 16-byte HLSL alignment
    };
};
