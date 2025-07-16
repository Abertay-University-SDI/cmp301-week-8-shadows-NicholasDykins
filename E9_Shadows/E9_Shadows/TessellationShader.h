#pragma once
#include "BaseShader.h"

class TessellationShader : public BaseShader
{
public:
    TessellationShader(ID3D11Device* device, HWND hwnd);
    ~TessellationShader();

    void setShaderParameters(
        ID3D11DeviceContext* deviceContext,
        const XMMATRIX& worldMatrix,
        const XMMATRIX& viewMatrix,
        const XMMATRIX& projectionMatrix);

private:
    void initShader(const wchar_t* vsFilename, const wchar_t* psFilename) override {}; //Needed because why would things be simple...
    void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

    ID3D11Buffer* matrixBuffer;
};
