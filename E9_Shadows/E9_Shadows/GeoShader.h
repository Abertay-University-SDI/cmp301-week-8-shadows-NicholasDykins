#pragma once
#include "BaseShader.h"

// Geometry shader wrapper 
class GeoShader : public BaseShader
{
public:
    GeoShader(ID3D11Device* device, HWND hwnd);
    ~GeoShader() { if (matrixBuffer) matrixBuffer->Release(); }

    // updates MVP matrix
    void setShaderParameters(
        ID3D11DeviceContext* ctx,
        const XMMATRIX& worldMatrix,
        const XMMATRIX& viewMatrix,
        const XMMATRIX& projMatrix);

private:
    void initShader(
        const wchar_t* vsFilename,
        const wchar_t* gsFilename,
        const wchar_t* psFilename);

    void initShader(const wchar_t* vs, const wchar_t* ps) override {}

    ID3D11Buffer* matrixBuffer = nullptr; // MVP buffer
};
