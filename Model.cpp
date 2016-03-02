#include "Model.h"

struct POS_COL_TEX_NORM_VERTEX
{
	XMFLOAT3	Pos;
	XMFLOAT4	Col;
	XMFLOAT2	Texture0;
	XMFLOAT3	Normal;
};

struct MODEL_CONSTANT_BUFFER
{
	XMMATRIX WorldViewProjection; // 64 bytes
}; //total 64

Model::Model(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 0.0f;
	m_xangle = 5.0f;
	m_yangle = 5.0f;
	m_zangle = 5.0f;
	m_scale = 1.0f;
	m_pD3DDevice = device;
	m_pImmediateContext = context;
}

int Model::LoadObjModel(char* filename)
{
	m_pObject = new ObjFileModel(filename, m_pD3DDevice, m_pImmediateContext);

	if (m_pObject->filename == "FILE NOT LOADED") return S_FALSE;

	HRESULT hr = S_OK;

	//setup and create constant buffer
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;	//Can use updateSubresourse() to update
	constant_buffer_desc.ByteWidth = 112;		//Must be a multiple of 16, calculate from CB struct
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //Use as a constant buffer

	hr = m_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &m_pConstantBuffer);

	if (FAILED(hr)) return hr;

	//setup and create the vertex buffer
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;				//used by CPU and GPU
	bufferDesc.ByteWidth = sizeof(POS_COL_TEX_NORM_VERTEX)* 36;	//Total size of buffer, 36 vertices
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//use as a vertex buffer
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //Allow CPU access

	if (FAILED(hr)) // return error code on failure
	{
		return hr;
	}

	//copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//Load and compile pixle and vertex shaders - use vs_5_0 to targtet DX11 hardware only
	ID3DBlob *VS, *PS, *error;
	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelVS", "vs_4_0", 0, 0, 0, &VS, &error, 0);

	if (error != 0) //check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //dont fail if error is just a warning
		{
			return hr;
		};
	}

	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelPS", "ps_4_0", 0, 0, 0, &PS, &error, 0);

	if (error != 0)//check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))//dont fail if error is just a warning
		{
			return hr;
		};
	}

	//create shader objects
	hr = m_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &m_pVShader);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &m_pPShader);

	if (FAILED(hr))
	{
		return hr;
	}

	//create and set the input layout object
	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = m_pD3DDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &m_pInputLayout);

	if (FAILED(hr))
	{
		return hr;
	}

	m_pImmediateContext->IASetInputLayout(m_pInputLayout);

	CalculateModelCentrePoint();
	CalculateBoundingSphereRadius();

	return S_OK;
}

void Model::LoadTextureForModel(char* filename)
{
	//Load the texture to the device
	D3DX11CreateShaderResourceViewFromFile(m_pD3DDevice, filename, NULL, NULL, &m_pTexture0, NULL);
	//setup and create the sample state
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));

	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	m_pD3DDevice->CreateSamplerState(&sampler_desc, &m_pSampler0);
}

void Model::Draw(XMMATRIX* world, XMMATRIX* view, XMMATRIX* projection)
{

	MODEL_CONSTANT_BUFFER model_cb_values;
	model_cb_values.WorldViewProjection = (*world)*(*view)*(*projection);
	m_pImmediateContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	m_pImmediateContext->UpdateSubresource(m_pConstantBuffer, 0, 0, &model_cb_values, 0, 0);

	m_pImmediateContext->VSSetShader(m_pVShader, 0, 0);
	m_pImmediateContext->PSSetShader(m_pPShader, 0, 0);
	m_pImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTexture0);
	m_pImmediateContext->PSSetSamplers(0, 1, &m_pSampler0);

	m_pObject->Draw();

}

void Model::LookAt_XZ(float x, float z)
{
	m_dx = x - m_x;
	m_dz = z - m_z;
	m_y_angle = atan2(m_dx, m_dz) * (180.0 / XM_PI);
}

void Model::MoveForward(float distance)
{
	m_x += sin(m_y_angle * (XM_PI / 180.0)) * distance;
	m_z += cos(m_y_angle * (XM_PI / 180.0)) * distance;

	float step = 0.5;
	m_x += step * m_dx;
	m_z += step * m_dz;

	XMVECTOR position = XMVectorSet(m_x, m_y, m_z, 0.0);
}

void Model::CalculateModelCentrePoint()
{
	float minX = 1;
	float maxX = -1;
	float minY = 1;
	float maxY = -1;
	float minZ = 1;
	float maxZ = -1;

	for (int i = 0; i < m_pObject->numverts; i++)
	{
		if (m_pObject->vertices[i].Pos.x < minX)
		{
			minX = m_pObject->vertices[i].Pos.x;
		}
		if (m_pObject->vertices[i].Pos.x > maxX)
		{
			maxX = m_pObject->vertices[i].Pos.x;
		}
		if (m_pObject->vertices[i].Pos.y < minY)
		{
			minY = m_pObject->vertices[i].Pos.y;
		}
		if (m_pObject->vertices[i].Pos.y > maxY)
		{
			maxY = m_pObject->vertices[i].Pos.y;
		}
		if (m_pObject->vertices[i].Pos.z < minZ)
		{
			minZ = m_pObject->vertices[i].Pos.z;
		}
		if (m_pObject->vertices[i].Pos.z > maxZ)
		{
			maxZ = m_pObject->vertices[i].Pos.z;
		}
	}

	m_bounding_sphere_centre_x = ((minX + maxX) / 2);
	m_bounding_sphere_centre_y = ((minY + maxY) / 2);
	m_bounding_sphere_centre_z = ((minZ + maxZ) / 2);

	if (maxX < abs(minX))
	{
		radiusX = maxX;
	}
	else
	{
		radiusX = abs(minX);
	}

	if (maxY < abs(maxY))
	{
		radiusY = maxY;
	}
	else
	{
		radiusY = abs(maxY);
	}

	if (maxZ < abs(minZ))
	{
		radiusZ = maxZ;
	}
	else
	{
		radiusZ = abs(minZ);
	}
}

void Model::CalculateBoundingSphereRadius()
{
	if ((radiusX >= radiusY) && (radiusX >= radiusZ))
	{
		m_bounding_sphere_radius = radiusX;
	}
	else if ((radiusY >= radiusX) && (radiusY >= radiusZ))
	{
		m_bounding_sphere_radius = radiusY;
	}
	else
	{
		m_bounding_sphere_radius = radiusZ;
	}
}

XMVECTOR Model::GetBoundingSphereWorldSpacePosition()
{
	XMMATRIX world;
	world = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_xangle), XMConvertToRadians(m_yangle), XMConvertToRadians(m_zangle));
	world *= XMMatrixScaling(m_scale, m_scale, m_scale);
	world *= XMMatrixTranslation(m_x, m_y, m_z);
	XMVECTOR offset;
	offset = XMVectorSet(m_bounding_sphere_centre_x, m_bounding_sphere_centre_y, m_bounding_sphere_centre_z, 0.0);
	offset = XMVector3Transform(offset, world);
	return offset;
}

float Model::GetBoundingSphereRadius()
{
	return (m_bounding_sphere_radius * m_scale);
}

float Model::GetBoundingSphere_x()
{
	return m_bounding_sphere_centre_x;
}

float Model::GetBoundingSphere_y()
{
	return m_bounding_sphere_centre_y;
}

float Model::GetBoundingSphere_z()
{
	return m_bounding_sphere_centre_z;
}

bool Model::CheckCollision(Model* model)
{
	XMVECTOR ThisModel = this->GetBoundingSphereWorldSpacePosition();
	XMVECTOR OtherModel = model->GetBoundingSphereWorldSpacePosition();
	//M stands for Model
	float M1x = XMVectorGetX(ThisModel);
	float M1y = XMVectorGetY(ThisModel);
	float M1z = XMVectorGetZ(ThisModel);
	float M2x = XMVectorGetX(OtherModel);
	float M2y = XMVectorGetY(OtherModel);
	float M2z = XMVectorGetZ(OtherModel);

	float distance_squared = pow(M1x - M2x, 2) + pow(M1y - M2y, 2) + pow(M1z - M2z, 2);
	
	if (this == model)
	{
		return false;
	}
	if (distance_squared < pow((this->GetBoundingSphereRadius() + model->GetBoundingSphereRadius()), 2))
	{
		return true;
	}
}

Model::~Model()
{
	if (m_pObject)		delete m_pObject;
	if (m_pInputLayout) m_pInputLayout->Release();
	if (m_pPShader)	m_pPShader->Release();
	if (m_pVShader) m_pVShader->Release();
	if (m_pConstantBuffer) m_pConstantBuffer->Release();
}