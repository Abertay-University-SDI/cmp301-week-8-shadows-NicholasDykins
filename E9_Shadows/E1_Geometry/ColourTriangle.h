#pragma once

#include "../DXFramework/DXF.h"

using namespace DirectX;


class ColourTriangle : public BaseMesh
{
public:
	ColourTriangle(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	~ColourTriangle();

	virtual void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
protected:
	void initBuffers(ID3D11Device* device);
};

