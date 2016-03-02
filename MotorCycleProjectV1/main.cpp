#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <dinput.h>
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>
#include "Camera.h"
#include "SceneNode.h"
#include "text2D.h"
#include "Model.h"
//////////////////////////////////////////////////////////////////////////////////////
//	Global Variables
//////////////////////////////////////////////////////////////////////////////////////
HINSTANCE	g_hInst = NULL;
HWND		g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pD3DDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pBackBufferRTView = NULL;
IDirectInput8*			g_direct_input;
IDirectInputDevice8*    g_keyboard_device;
IDirectInputDevice8*    g_mouse_device;

ID3D11Buffer*			g_pVertexBuffer;
ID3D11VertexShader*		g_pVertexShader;
ID3D11PixelShader*		g_pPixelShader;
ID3D11InputLayout*		g_pInputLayout;
ID3D11DepthStencilView* g_pZbuffer;

ID3D11ShaderResourceView*	g_pTexture0;
ID3D11SamplerState*		g_pSampler0;

D3D11_SAMPLER_DESC		sampler_desc;

ID3D11Buffer*			g_pConstantBuffer0;

Camera*					camera1;
Text2D*					g_2DText;

SceneNode*				g_root_node;
SceneNode*				g_crate1;
SceneNode*				g_ball1;
SceneNode*				g_cam_node;

Model*					model1;
Model*					model2;
Model*					model3;
Model*					M_Camera;
Model*					M_bullet;

XMVECTOR g_directional_light_shines_from;
XMVECTOR g_directional_light_colour;
XMVECTOR g_ambient_light_colour;

unsigned char			g_keyboard_keys_state[256];
DIMOUSESTATE			g_MouseState;

float Xangle = 5.0f;
float Yangle = 5.0f;
float Zangle = 5.0f;
float LightAngle = 1.0f;
float Rcolour = 0.1f;
float Gcolour = 0.1f;
float Bcolour = 0.1f;

int flag = 0;
char		g_TutorialName[100] = "AGP - Tutorial 15 Exercise 01 (Adam Elletson) \0"; // Rename for each tutorial
//define vertex structure
struct POS_COL_TEX_NORM_VERTEX
{
	XMFLOAT3	Pos;
	XMFLOAT4	Col;
	XMFLOAT2	Texture0;
	XMFLOAT3	Normal;
};

//Const buffer struct. Pack to 16 bytes. Don't let any single element cross a 16 byte boundry
struct CONSTANT_BUFFER0
{
	XMMATRIX WorldViewProjection;	   //64 bytes (4x4 = 16 floats x 4 bytes)
	XMVECTOR directional_light_vector; //16 bytes
	XMVECTOR directional_light_colour; //16 bytes
	XMVECTOR ambient_light_colour;	   //16 bytes
}; //TOTAL size = 112 bytes


//////////////////////////////////////////////////////////////////////////////////////
//	Forward declarations
//////////////////////////////////////////////////////////////////////////////////////
HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitialiseInput();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitialiseD3D();
void ShutdownD3D();
void RenderFrame(void);
HRESULT	InitialiseGraphics(void);


//////////////////////////////////////////////////////////////////////////////////////
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//////////////////////////////////////////////////////////////////////////////////////


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitialiseWindow(hInstance, nCmdShow)))
	{
		DXTRACE_MSG("Failed to create Window");
		return 0;
	}
	if (FAILED(InitialiseInput()))
	{
		DXTRACE_MSG("Failed to create Input");
		return 0;
	}
	// Main message loop
	MSG msg = { 0 };
	if (FAILED(InitialiseD3D()))
	{
		DXTRACE_MSG("Failed to create Device");
		return 0;
	}
	if (FAILED(InitialiseGraphics()))
	{
		DXTRACE_MSG("Failed to initialise graphics");
		return 0;
	}

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			RenderFrame();
		}
	}
	ShutdownD3D();
	return (int)msg.wParam;
}


//////////////////////////////////////////////////////////////////////////////////////
// Register class and create window
//////////////////////////////////////////////////////////////////////////////////////
HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Give your app window your own name
	char Name[100] = "Adam Elletson work\0";

	// Register class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//   wcex.hbrBackground = (HBRUSH )( COLOR_WINDOW + 1); // Needed for non-D3D apps
	wcex.lpszClassName = Name;

	if (!RegisterClassEx(&wcex)) return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(Name, g_TutorialName, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
		rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);


	return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////
// Called every time the application receives a message
//////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////
// Create D3D device and swap chain
//////////////////////////////////////////////////////////////////////////////////////
HRESULT InitialiseD3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, // comment out this line if you need to test D3D 11.0 functionality on hardware that doesn't support it
		D3D_DRIVER_TYPE_WARP, // comment this out also to use reference device
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = true;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL,
			createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain,
			&g_pD3DDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
		return hr;

	// Get pointer to back buffer texture
	ID3D11Texture2D *pBackBufferTexture;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(LPVOID*)&pBackBufferTexture);

	if (FAILED(hr)) return hr;

	// Use the back buffer texture pointer to create the render target view
	hr = g_pD3DDevice->CreateRenderTargetView(pBackBufferTexture, NULL,	&g_pBackBufferRTView);
	pBackBufferTexture->Release();

	//Create a Z buffer texture
	D3D11_TEXTURE2D_DESC tex2dDesc;
	ZeroMemory(&tex2dDesc, sizeof(tex2dDesc));
	tex2dDesc.Width = width;
	tex2dDesc.Height = height;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tex2dDesc.SampleDesc.Count = sd.SampleDesc.Count;
	tex2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D *pZBufferTexture;
	hr = g_pD3DDevice->CreateTexture2D(&tex2dDesc, NULL, &pZBufferTexture);

	if (FAILED(hr)) return hr;

	//create the z buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));

	dsvDesc.Format = tex2dDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	g_pD3DDevice->CreateDepthStencilView(pZBufferTexture, &dsvDesc, &g_pZbuffer);
	pZBufferTexture->Release();

	// Set the render target view
	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRTView, g_pZbuffer);

	// Set the viewport
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	g_pImmediateContext->RSSetViewports(1, &viewport);

	g_2DText = new Text2D("assets/font1.bmp", g_pD3DDevice, g_pImmediateContext);

	return S_OK;
}
///////////////
//Init graphics
///////////////
HRESULT InitialiseGraphics()
{
	model1 = new Model(g_pD3DDevice, g_pImmediateContext);
	model1->LoadObjModel("assets/cube.obj");
	model1->LoadTextureForModel("assets/Box.PNG");

	model2 = new Model(g_pD3DDevice, g_pImmediateContext);
	model2->LoadObjModel("assets/Sphere.obj");
	model2->LoadTextureForModel("assets/FlatBall.PNG");

	model3 = new Model(g_pD3DDevice, g_pImmediateContext);
	model3->LoadObjModel("assets/Sphere.obj");
	model3->LoadTextureForModel("assets/FlatWorld.PNG");

	M_bullet = new Model(g_pD3DDevice, g_pImmediateContext);
	M_bullet->LoadObjModel("assets/cube.obj");
	M_bullet->LoadTextureForModel("assets/Bullet.PNG");

	M_Camera = new Model(g_pD3DDevice, g_pImmediateContext);
	M_Camera->LoadObjModel("assets/cube.obj");
	M_Camera->LoadTextureForModel("assets/Camera.PNG");

	g_root_node = new SceneNode();
	g_cam_node = new SceneNode();
	g_crate1 = new SceneNode();
	g_ball1 = new SceneNode();

	
	g_cam_node->SetModel(M_Camera);
	g_crate1->SetModel(model1);
	g_ball1->SetModel(model2);

	g_root_node->addChildNode(g_cam_node);

	g_root_node->addChildNode(g_crate1);
	g_root_node->addChildNode(g_ball1);

	g_cam_node->setNodeScale(0.3);
	g_root_node->setNodeScale(1.0);
	g_ball1->setNodeScale(0.5);

	g_root_node->setNodePosition(0.0, 0.0, 0.0);
	g_crate1->setNodePosition(5.0, 0.0, 0.0);
	g_ball1->setNodePosition(0.0, 0.0, 0.0);



	HRESULT hr = S_OK;

	//setup and create constant buffer
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;	//Can use updateSubresourse() to update
	constant_buffer_desc.ByteWidth = 112;		//Must be a multiple of 16, calculate from CB struct
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //Use as a constant buffer

	hr = g_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &g_pConstantBuffer0);

	if (FAILED(hr)) return hr;

	//setup and create the sample state
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pD3DDevice->CreateSamplerState(&sampler_desc, &g_pSampler0);

	if (FAILED(hr)) // return error code on failure
	{
		return hr;
	}

	camera1 = new Camera(0.0, 0.0, -10.0, 0);
	g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());


	g_pImmediateContext->IASetInputLayout(g_pInputLayout);

	return S_OK;
}

////////////////
//Create Input
////////////////
HRESULT InitialiseInput()
{
	HRESULT hr;

	ZeroMemory(g_keyboard_keys_state, sizeof(g_keyboard_keys_state));

	hr = DirectInput8Create(g_hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_direct_input, NULL);
	if (FAILED(hr)) return hr;
	
	//keyboard
	hr = g_direct_input->CreateDevice(GUID_SysKeyboard, &g_keyboard_device, NULL);
	if (FAILED(hr)) return hr;

	hr = g_keyboard_device->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr)) return hr;

	hr = g_keyboard_device->SetCooperativeLevel(g_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(hr)) return hr;

	hr = g_keyboard_device->Acquire();
	if (FAILED(hr)) return hr;


	//mouse
	hr = g_direct_input->CreateDevice(GUID_SysMouse, &g_mouse_device, NULL);
	if (FAILED(hr)) return hr;

	hr = g_mouse_device->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hr)) return hr;

	hr = g_mouse_device->SetCooperativeLevel(g_hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(hr)) return hr;

	hr = g_mouse_device->Acquire();
	if (FAILED(hr)) return hr;

	g_mouse_device->Poll();
	if (!SUCCEEDED(g_mouse_device->GetDeviceState(sizeof(DIMOUSESTATE), &g_MouseState)));
	{
		hr = g_mouse_device->Acquire();
		if (FAILED(hr)) return hr;
	}

	return S_OK;
}
////////////////
//Read Input
////////////////
void ReadInputStates()
{
	HRESULT hr;
	hr = g_keyboard_device->GetDeviceState(sizeof(g_keyboard_keys_state), (LPVOID)&g_keyboard_keys_state);
	if (FAILED(hr))
	{
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			g_keyboard_device->Acquire();
		}
	}

	hr = g_mouse_device->GetDeviceState(sizeof(g_MouseState), (LPVOID)&g_MouseState);
	if (FAILED(hr))
	{
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			g_mouse_device->Acquire();
		}
	}

	camera1->RotateHorizontal(g_MouseState.lX * 0.05);
	camera1->RotateVertical(-g_MouseState.lY * 0.01);

}

bool IsKeyPressed(unsigned char DI_keycode)
{
	return g_keyboard_keys_state[DI_keycode] & 0x80;
}

void KeyPresses()
{
	//KEYDOWN EVENTS
	//Rcolour += 0.1f;      colour change
	//LightAngle -= 0.1;
	//g_nodeWheel1->LookAt_XZ(g_nodeBody->getXpos(), g_nodeBody->getYpos());
	if (IsKeyPressed(DIK_ESCAPE)) DestroyWindow(g_hWnd);
/*	if (IsKeyPressed(DIK_UP))
	{
		g_nodeBody->setXNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(1.0));
	}
	if (IsKeyPressed(DIK_DOWN))
	{
		g_nodeBody->setXNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setXNodeAngle(XMConvertToRadians(-1.0));
	}
	if (IsKeyPressed(DIK_RIGHT))
	{
		g_nodeBody->setYNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(-1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(-1.0));
	}
	if (IsKeyPressed(DIK_LEFT))
	{
		g_nodeBody->setYNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(1.0));
		g_nodeBody->setYNodeAngle(XMConvertToRadians(1.0));
	}
	*/
	if (IsKeyPressed(DIK_F1)) g_root_node->setNodePosition(0.0,0.0,0.0);
//	if (IsKeyPressed(DIK_G)) g_nodeBody->AlterX(0.01, g_root_node);
//	if (IsKeyPressed(DIK_H)) g_nodeBody->AlterX(-0.01, g_root_node);
	if (IsKeyPressed(DIK_W)){ 
		camera1->Stright(0.01);
		g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		XMMATRIX identity = XMMatrixIdentity();
		// update tree to reflect new camera position
		g_root_node->UpdateCollisionTree(&identity, 1.0);
		if (g_cam_node->CheckCollision(g_root_node) == true)
		{
			// if there is a collision, restore camera and camera node positions
			OutputDebugString("W Collision ");
			camera1->Stright(-1.0);
			g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		}
	}
	if (IsKeyPressed(DIK_S)){
		camera1->Stright(-0.01);
		g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		XMMATRIX identity = XMMatrixIdentity();
		g_root_node->UpdateCollisionTree(&identity, 1.0);
		if (g_cam_node->CheckCollision(g_root_node) == true)
		{
			OutputDebugString("S Collision ");
			camera1->Stright(1.0);
			g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		}
	}
	if (IsKeyPressed(DIK_A)){
		camera1->Sideways(-0.009);
		g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		XMMATRIX identity = XMMatrixIdentity();
		g_root_node->UpdateCollisionTree(&identity, 1.0);
		if (g_cam_node->CheckCollision(g_root_node) == true)
		{
			OutputDebugString("A Collision ");
			camera1->Sideways(1.0);
			g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		}
	}
	if (IsKeyPressed(DIK_D)){
		camera1->Sideways(0.009);
		g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		XMMATRIX identity = XMMatrixIdentity();
		g_root_node->UpdateCollisionTree(&identity, 1.0);
		if (g_cam_node->CheckCollision(g_root_node) == true)
		{
			OutputDebugString("D Collision ");
			camera1->Sideways(-1.0);
			g_cam_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		}
	}
/*	if (g_MouseState.rgbButtons[0])
	{
		g_bulllet_node->setNodePosition(camera1->getX(), camera1->getY(), camera1->getZ());
		g_bulllet_node->Shoot(camera1->getZ(), g_root_node);
		Sleep(30);
	}
	if (IsKeyPressed(DIK_SPACE)){
		XMVECTOR bullettest = g_bulllet_node->GetWorldCentrePosition();
		XMVECTOR cartest = g_nodeBody->GetWorldCentrePosition();
		XMVECTOR camtest = g_cam_node->GetWorldCentrePosition();
		XMVECTOR W1test = g_nodeWheel1->GetWorldCentrePosition();
		XMVECTOR W2test = g_nodeWheel2->GetWorldCentrePosition();
		XMVECTOR W3test = g_nodeWheel3->GetWorldCentrePosition();
		XMVECTOR W4test = g_nodeWheel4->GetWorldCentrePosition();
		OutputDebugString(" ");
	}
	*/
}

///////////////
// Render frame
///////////////
void RenderFrame(void)
{
	ReadInputStates();
	KeyPresses();
	// Clear the back buffer - choose a colour you like
	float rgba_clear_colour[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRTView, rgba_clear_colour);
	g_pImmediateContext->ClearDepthStencilView
		(g_pZbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//calculate the world view projection matrix
	XMMATRIX world, projection, view;
	world = XMMatrixIdentity();
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0), 640.0 / 480.0, 1.0, 100.0);
	view = camera1->GetViewMatrix();			//XMMatrixIdentity();

	//add text
	g_2DText->AddText("Test", -1.0, +1.0, 0.05);
	
	//Select which primitive type to use
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//set Texture and sample states (index, how many, address)
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSampler0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTexture0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, 0, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, 0, 0);
	g_pImmediateContext->IASetInputLayout(g_pInputLayout);

	//all transformations must be done before this is called
	g_root_node->execute(&world, &view, &projection);

	//Draw the text
	g_2DText->RenderText();

	// Display what has just been rendered
	g_pSwapChain->Present(0, 0);
}

#include "DXGIDebug.h"
//////////////////////////////////////////////////////////////////////////////////////
// Clean up D3D objects
//////////////////////////////////////////////////////////////////////////////////////
void ShutdownD3D()
{
	if (model3)			delete model3;
	if (model2)			delete model2;
	if (model1)			delete model1;
	if (g_2DText)		delete g_2DText;
	if (camera1)		delete camera1;
	if (g_pConstantBuffer0) g_pConstantBuffer0->Release();
	if (g_pSampler0) g_pSampler0->Release();
	if (g_pTexture0) g_pTexture0->Release();
	if (g_pZbuffer) g_pZbuffer->Release();
	if (g_pInputLayout) g_pInputLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_mouse_device)
	{
		g_mouse_device->Unacquire();
		g_mouse_device->Release();
	}
	if (g_keyboard_device)
	{
		g_keyboard_device->Unacquire();
		g_keyboard_device->Release();
	}
	if (g_pBackBufferRTView) g_pBackBufferRTView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();

//	if (g_pD3DDevice) g_pD3DDevice->Release(); strange error
}
