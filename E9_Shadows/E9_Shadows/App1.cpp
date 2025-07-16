// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"

App1::App1()
{

}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);
	waveShader = new WaveFunctionShader(renderer->getDevice(), hwnd);
	// Create Mesh object and shader object
	mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	model = new AModel(renderer->getDevice(), "res/teapot.obj");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");

	//Post Processing Texture
	postProcessTexture = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	orthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth, screenHeight);

	postProcessShader = new PostProcessShader(renderer->getDevice(), hwnd);
	postProcessShader->initShader(
		L"post_process_VertexShader.cso",
		L"post_process_PixelShader.cso");

	cottage = new AModel(renderer->getDevice(), "res/models/cottage_fbx.fbx");
	textureMgr->loadTexture(L"cottage_diffuse", L"res/models/cottage_textures/cottage_diffuse.png");
	textureMgr->loadTexture(L"cottage_normal", L"res/models/cottage_textures/cottage_normal.png");

	// initial shaders
	textureShader = new TextureShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	geoShader = new GeoShader(renderer->getDevice(), hwnd);
	tessShader = new TessellationShader(renderer->getDevice(), hwnd);
	//shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

	// Variables for defining shadow map
	int shadowmapWidth = 1024;
	int shadowmapHeight = 1024;
	int sceneWidth = 100;
	int sceneHeight = 100;

	// This is your shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Configure directional light
	light = new Light();
	light->setAmbientColour(0.2f, 0.2f, 0.2f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(0.0f, -1.0f, -0.1f);
	light->setPosition(0.f, 0.f, -10.f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);



	shadowMap2 = new ShadowMap(renderer->getDevice(), 1024, 1024);

	// Red glowing point light
	pointLight = new Light();
	pointLight->setAmbientColour(0.1f, 0.0f, 0.0f, 1.0f);
	pointLight->setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f);
	pointLight->setDirection(0.0f, -1.0f, 0.0f); 
	pointLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);


	customShader = new MyLightShader(renderer->getDevice(), hwnd);

}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();


}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}
	
	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	if (input->isKeyDown('T')) 
	{
		orbiting = true;
	}

	if (orbiting)
	{
		
		orbitAngle += XMConvertToRadians(45.0f) * timer->getTime(); // 45 deg/sec
		if (orbitAngle > XM_2PI) orbitAngle -= XM_2PI; // wrap around

		float radius = 10.f;
		float x = cosf(orbitAngle) * radius;
		float z = sinf(orbitAngle) * radius;
		float y = 5.f; // fixed height

		light->setPosition(x, y, z);

		// Aim back at centre 
		XMFLOAT3 toCentre(-x, -y, -z);
		auto dirVec = XMVector3Normalize(XMLoadFloat3(&toCentre));
		XMFLOAT3 newDir;
		XMStoreFloat3(&newDir, dirVec);
		light->setDirection(newDir.x, newDir.y, newDir.z);
	}




	// toggle for point light
	if (input->isKeyDown('Y'))
	{
		if (!pointLightOn) {
			pointLightOn = true;

			// swap between cottage/teapot positions
			if (pointToCottage)
				pointLight->setPosition(5.f, 10.f, 10.f); // above cottage
			else
				pointLight->setPosition(8.f, 10.f, 5.f);  // above teapot   I'm not sure why this is giving me trouble but out of time

			pointToCottage = !pointToCottage; // flip for next time
		}
	}
	else 
	{
		// turn off when not holding key
		pointLightOn = false;
	}


	if (input->isKeyDown('U'))
	{
		if (!togglePostProcessPressed) 
		{
			postProcessEnabled = !postProcessEnabled;
			togglePostProcessPressed = true;
		}
	}
	else 
	{
		togglePostProcessPressed = false; // reset once released
	}

	
	if (input->isKeyDown('G'))
	{
		geoEnabled = true;
	}
	else
	{
		geoEnabled = false;
	}
		
	if (input->isKeyDown('H'))
	{
		tessEnabled = true;
	}		
	else
	{
		tessEnabled = false;
	}
		


	accumulatedTime += timer->getTime();

	return true;
}

bool App1::render()
{

	// Perform depth pass
	depthPass();
	
	if (postProcessEnabled)
	{
		postProcessPass();
	}
		 // Render scene to texture, then draw full-screen quad
	else
	{
		finalPass(); // Regular 3D scene with shadows etc.
	}
	
	gui();
	renderer->endScene();

	return true;
}

void App1::depthPass()
{
	// Set the render target to be the render to texture.
	shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// get the world, view, and projection matrices from the camera and d3d objects.
	light->generateViewMatrix();
	XMMATRIX lightViewMatrix = light->getViewMatrix();
	XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();
	XMMATRIX worldMatrix = renderer->getWorldMatrix();

	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	// Render floor
	mesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	worldMatrix = renderer->getWorldMatrix();
	worldMatrix = XMMatrixTranslation(0.f, 3.f, 5.f);
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	worldMatrix = XMMatrixMultiply(worldMatrix, scaleMatrix);
	
	// Render model
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());


	XMMATRIX rotateX = XMMatrixRotationX(XMConvertToRadians(-90)); // FBX often comes in rotated
	XMMATRIX rotateY = XMMatrixRotationY(XMConvertToRadians(180)); // Facing backward?
	XMMATRIX scale = XMMatrixScaling(3.1f, 3.1f, 3.1f); // Make it much smaller
	XMMATRIX translate = XMMatrixTranslation(0.f, 0.f, 10.f); // Move to front-right of scene

	worldMatrix = rotateX * rotateY * scale * translate;

	// Render cottage
	cottage->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), cottage->getIndexCount());


	if (pointLightOn)
	{
		pointLight->generateViewMatrix();
		XMMATRIX view = pointLight->getViewMatrix();
		XMMATRIX proj = pointLight->getOrthoMatrix();

		shadowMap2->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

		XMMATRIX world = XMMatrixTranslation(-50.f, 0.f, -10.f);
		mesh->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, proj);
		depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

		world = XMMatrixTranslation(8.f, 3.f, 5.f) * XMMatrixScaling(0.5f, 0.5f, 0.5f);
		model->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, proj);
		depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

		XMMATRIX rotateX = XMMatrixRotationX(XMConvertToRadians(90));
		XMMATRIX rotateY = XMMatrixRotationY(XMConvertToRadians(180));
		XMMATRIX scale = XMMatrixScaling(3.1f, 3.1f, 3.1f);
		XMMATRIX translate = XMMatrixTranslation(-5.f, 3.f, 10.f);
		world = scale * rotateX * rotateY * translate;

		cottage->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), world, view, proj);
		depthShader->render(renderer->getDeviceContext(), cottage->getIndexCount());

		renderer->setBackBufferRenderTarget();
		renderer->resetViewport();
	}


	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::finalPass()
{
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	if (wireframeToggle)
	{
		renderer->setWireframeMode(true);
	}
	else
	{
		renderer->setWireframeMode(false);
	}
		

	Light* activeLight = pointLightOn ? pointLight : light;
	ID3D11ShaderResourceView* activeShadow = pointLightOn ? shadowMap2->getDepthMapSRV() : shadowMap->getDepthMapSRV();
	XMMATRIX activeLightMatrix = pointLightOn
		? pointLight->getViewMatrix() * pointLight->getOrthoMatrix()
		: light->getViewMatrix() * light->getOrthoMatrix();


	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();


	//Wave
	float time = timer->getTime();
	float amplitude = 0.3f;
	float frequency = 2.0f;

	XMMATRIX worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	waveShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix,
		viewMatrix,
		projectionMatrix,
		textureMgr->getTexture(L"brick"),
		accumulatedTime,
		amplitude,
		frequency
	);
	waveShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	//

	////  FLOOR 
	//XMMATRIX worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	//mesh->sendData(renderer->getDeviceContext());
	//customShader->setShaderParameters(
	//	renderer->getDeviceContext(),
	//	worldMatrix,
	//	viewMatrix,
	//	projectionMatrix,
	//	textureMgr->getTexture(L"brick"),
	//	light->getViewMatrix() * light->getOrthoMatrix(), 
	//	shadowMap->getDepthMapSRV(),
	//	light
	//);
	//customShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// MODEL 
	worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f);
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	
	if (tessEnabled)
	{
		XMMATRIX worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f) * XMMatrixScaling(0.5f, 0.5f, 0.5f);
		model->sendData(renderer->getDeviceContext());
		// tessellation needs patchlist topology or it explodes
		renderer->getDeviceContext()->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

		tessShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix);
		tessShader->render(renderer->getDeviceContext(), model->getIndexCount());

		// probably should reset topology after tess assume TRIANGLELIST is default
		renderer->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	else if (geoEnabled)
	{
		// GS teapot not super optimized but it works
		XMMATRIX worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f) * XMMatrixScaling(0.5f, 0.5f, 0.5f);
		model->sendData(renderer->getDeviceContext());
		geoShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix);
		geoShader->render(renderer->getDeviceContext(), model->getIndexCount());
	}
	else
	{
		// fallback normal lighting

		XMMATRIX worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f) * XMMatrixScaling(0.5f, 0.5f, 0.5f);
		model->sendData(renderer->getDeviceContext());
		customShader->setShaderParameters(
			renderer->getDeviceContext(),
			worldMatrix,
			viewMatrix,
			projectionMatrix,
			textureMgr->getTexture(L"brick"),
			activeShadow,
			activeLight
		);

		customShader->render(renderer->getDeviceContext(), model->getIndexCount());
	}



	// COTTAGE 
	worldMatrix = XMMatrixTranslation(10.f, 0.f, 5.f);  // Adjust position as needed
	scaleMatrix = XMMatrixScaling(3.1f, 3.1f, 3.1f);  // Adjust size as needed
	XMMATRIX rotateX = XMMatrixRotationX(XMConvertToRadians(90)); // FBX often comes in rotated
	XMMATRIX rotateY = XMMatrixRotationY(XMConvertToRadians(180)); // Facing backward?
	XMMATRIX translate = XMMatrixTranslation(-5.f, 3.f, 10.f); // Move to front-right of scene

	worldMatrix = scaleMatrix * rotateX * rotateY * translate;

	cottage->sendData(renderer->getDeviceContext());
	customShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix,
		viewMatrix,
		projectionMatrix,
		textureMgr->getTexture(L"brick"),
		activeShadow,
		activeLight
	);

	customShader->render(renderer->getDeviceContext(), cottage->getIndexCount());

	//gui();
	//renderer->endScene();
}




void App1::postProcessPass()
{
	// LIGHT SELECTION 
	Light* activeLight = pointLightOn ? pointLight : light;
	ID3D11ShaderResourceView* activeShadow = pointLightOn ? shadowMap2->getDepthMapSRV() : shadowMap->getDepthMapSRV();
	XMMATRIX activeLightMatrix = pointLightOn
		? pointLight->getViewMatrix() * pointLight->getOrthoMatrix()
		: light->getViewMatrix() * light->getOrthoMatrix();

	// RENDER TO TEXTURE 
	postProcessTexture->setRenderTarget(renderer->getDeviceContext());
	postProcessTexture->clearRenderTarget(renderer->getDeviceContext(), 0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

	// FLOOR
	XMMATRIX worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	waveShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix,
		viewMatrix,
		projectionMatrix,
		textureMgr->getTexture(L"brick"),
		accumulatedTime,
		0.3f, 2.0f
	);
	waveShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// TEAPOT 
	worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f) * XMMatrixScaling(0.5f, 0.5f, 0.5f);
		model->sendData(renderer->getDeviceContext());
		customShader->setShaderParameters(
			renderer->getDeviceContext(),
			worldMatrix,
			viewMatrix,
			projectionMatrix,
			textureMgr->getTexture(L"brick"),
			activeShadow,
			activeLight
		);

		customShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// COTTAGE 
	worldMatrix = XMMatrixScaling(3.1f, 3.1f, 3.1f) *
		XMMatrixRotationX(XMConvertToRadians(90)) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixTranslation(-5.f, 3.f, 10.f);
	cottage->sendData(renderer->getDeviceContext());
	customShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix,
		viewMatrix,
		projectionMatrix,
		textureMgr->getTexture(L"brick"),
		activeShadow,
		activeLight
	);
	customShader->render(renderer->getDeviceContext(), cottage->getIndexCount());

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	renderer->getDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

	// switch to bac buffr
	renderer->setBackBufferRenderTarget();
	renderer->beginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// fullscreen quad
	XMMATRIX orthoWorld = renderer->getWorldMatrix();
	XMMATRIX orthoView = camera->getViewMatrix();
	XMMATRIX orthoProj = renderer->getOrthoMatrix();

	orthoMesh->sendData(renderer->getDeviceContext());


	orthoMesh->sendData(renderer->getDeviceContext());
	postProcessShader->setShaderParameters(
		renderer->getDeviceContext(),
		orthoWorld,
		orthoView,
		orthoProj,
		postProcessTexture->getShaderResourceView() 
	);
	postProcessShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());
}



//void App1::finalPass()
//{
//	Light* activeLight = pointLightOn ? pointLight : light;
//	ID3D11ShaderResourceView* activeShadow = pointLightOn ? shadowMap2->getDepthMapSRV() : shadowMap->getDepthMapSRV();
//	XMMATRIX activeLightMatrix = pointLightOn
//		? pointLight->getViewMatrix() * pointLight->getOrthoMatrix()
//		: light->getViewMatrix() * light->getOrthoMatrix();
//
//	postProcessTexture->setRenderTarget(renderer->getDeviceContext());
//	postProcessTexture->clearRenderTarget(renderer->getDeviceContext(), 0.39f, 0.58f, 0.92f, 1.0f);
//	camera->update();
//
//	
//
//
//	XMMATRIX viewMatrix = camera->getViewMatrix();
//	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
//
//
//	//Wave
//	float time = timer->getTime();
//	float amplitude = 0.3f;
//	float frequency = 2.0f;
//
//	XMMATRIX worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
//	mesh->sendData(renderer->getDeviceContext());
//	waveShader->setShaderParameters(
//		renderer->getDeviceContext(),
//		worldMatrix,
//		viewMatrix,
//		projectionMatrix,
//		textureMgr->getTexture(L"brick"),
//		accumulatedTime,
//		amplitude,
//		frequency
//	);
//	waveShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
//
//	//
//
//	//// === FLOOR ===
//	//XMMATRIX worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
//	//mesh->sendData(renderer->getDeviceContext());
//	//customShader->setShaderParameters(
//	//	renderer->getDeviceContext(),
//	//	worldMatrix,
//	//	viewMatrix,
//	//	projectionMatrix,
//	//	textureMgr->getTexture(L"brick"),
//	//	light->getViewMatrix() * light->getOrthoMatrix(), 
//	//	shadowMap->getDepthMapSRV(),
//	//	light
//	//);
//	//customShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
//
//	// === MODEL ===
//	worldMatrix = XMMatrixTranslation(8.f, 3.f, 5.f);
//	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
//	worldMatrix = XMMatrixMultiply(scaleMatrix, worldMatrix); // scale THEN translate
//	model->sendData(renderer->getDeviceContext());
//	customShader->setShaderParameters(
//		renderer->getDeviceContext(),
//		worldMatrix,
//		viewMatrix,
//		projectionMatrix,
//		textureMgr->getTexture(L"brick"),  // or other texture
//		activeLightMatrix,
//		activeShadow,
//		activeLight
//	);
//	customShader->render(renderer->getDeviceContext(), model->getIndexCount());
//
//	// == COTTAGE ==
//	worldMatrix = XMMatrixTranslation(10.f, 0.f, 5.f);  // Adjust position as needed
//	scaleMatrix = XMMatrixScaling(3.1f, 3.1f, 3.1f);  // Adjust size as needed
//	XMMATRIX rotateX = XMMatrixRotationX(XMConvertToRadians(90)); // FBX often comes in rotated
//	XMMATRIX rotateY = XMMatrixRotationY(XMConvertToRadians(180)); // Facing backward?
//	XMMATRIX translate = XMMatrixTranslation(-5.f, 3.f, 10.f); // Move to front-right of scene
//
//	worldMatrix = scaleMatrix * rotateX * rotateY *  translate;
//
//	cottage->sendData(renderer->getDeviceContext());
//	customShader->setShaderParameters(
//		renderer->getDeviceContext(),
//		worldMatrix,
//		viewMatrix,
//		projectionMatrix,
//		textureMgr->getTexture(L"brick"),  // or other texture
//		activeLightMatrix,
//		activeShadow,
//		activeLight
//	);
//	customShader->render(renderer->getDeviceContext(), cottage->getIndexCount());
//
//	if (postProcessEnabled)
//	{
//		// Post-processing: draw quad with postProcessShader
//		XMMATRIX world = renderer->getWorldMatrix();
//		XMMATRIX view = camera->getViewMatrix();
//		XMMATRIX proj = renderer->getOrthoMatrix();
//
//		orthoMesh->sendData(renderer->getDeviceContext());
//		postProcessShader->setShaderParameters(
//			renderer->getDeviceContext(),
//			world,
//			view,
//			proj,
//			postProcessTexture->getShaderResourceView()
//		);
//		postProcessShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());
//	}
//	else
//	{
//		renderer->setBackBufferRenderTarget();  // Switch back to back buffer
//		renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);  // Clear screen
//
//		XMMATRIX world = renderer->getWorldMatrix();
//		XMMATRIX view = camera->getViewMatrix();
//		XMMATRIX ortho = renderer->getOrthoMatrix();
//
//		orthoMesh->sendData(renderer->getDeviceContext());
//		textureShader->setShaderParameters(
//			renderer->getDeviceContext(),
//			world,
//			view,
//			ortho,
//			postProcessTexture->getShaderResourceView()
//		);
//		textureShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());
//	}
//	//// Switch back to back buffer
//	//renderer->setBackBufferRenderTarget();
//	//renderer->beginScene(0.f, 0.f, 0.f, 1.f);  // Clear screen again
//
//	//// Prepare matrices
//	//XMMATRIX world = renderer->getWorldMatrix();
//	//XMMATRIX view = camera->getViewMatrix();
//	//XMMATRIX proj = renderer->getOrthoMatrix(); // Ortho for 2D quad
//
//	//// Draw fullscreen quad with post-process shader
//	//orthoMesh->sendData(renderer->getDeviceContext());
//	//postProcessShader->setShaderParameters(
//	//	renderer->getDeviceContext(),
//	//	world,
//	//	view,
//	//	proj,
//	//	postProcessTexture->getShaderResourceView()
//	//);
//	//postProcessShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());
//
//
//
//
//	gui();
//	renderer->endScene();
//}




void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	ImGui::Text("Post-processing: %s", postProcessEnabled ? "ON" : "OFF");
	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

