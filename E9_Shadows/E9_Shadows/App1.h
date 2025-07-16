// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "TextureShader.h"
//#include "ShadowShader.h"
#include "DepthShader.h"
#include "MyLightShader.h"
#include "WaveFunctionShader.h"
#include "PostProcessShader.h"
#include "GeoShader.h"
#include "TessellationShader.h"



class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

	float orbitAngle = 0.0f;
	bool orbiting = false;

	float accumulatedTime = 0.0f;





protected:
	bool render();
	void depthPass();
	void finalPass();
	void gui();
	void postProcessPass();
private:
	TextureShader* textureShader;
	PlaneMesh* mesh;

	Light* light;
	AModel* model;
	AModel* cottage;
	//ShadowShader* shadowShader;
	DepthShader* depthShader;




	ShadowMap* shadowMap;


	Light* pointLight;
	ShadowMap* shadowMap2;
	bool pointLightOn = false;
	bool pointToCottage = true;

	bool postProcessEnabled = false;
	bool togglePostProcessPressed = false;

	GeoShader* geoShader;   
	bool geoEnabled = false;

	WaveFunctionShader* waveShader;
	PostProcessShader* postProcessShader;

	TessellationShader* tessShader;
	bool tessEnabled = false;


	MyLightShader* customShader;

	RenderTexture* postProcessTexture;
	OrthoMesh* orthoMesh;
};

#endif