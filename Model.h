#pragma once

#include "objfilemodel.h"
#include <xnamath.h>
#include <windows.h>

class Model
{
private:
	ID3D11Device*		 m_pD3DDevice;
	ID3D11DeviceContext* m_pImmediateContext;

	ObjFileModel*		 m_pObject;
	ID3D11VertexShader*	 m_pVShader;
	ID3D11PixelShader*	 m_pPShader;
	ID3D11InputLayout*	 m_pInputLayout;
	ID3D11Buffer*		 m_pConstantBuffer;

	ID3D11ShaderResourceView*	m_pTexture0;
	ID3D11SamplerState* m_pSampler0;

	float m_x, m_y, m_z;
	float m_xangle, m_yangle, m_zangle;
	float m_scale;
	float m_y_angle;
	float m_dx, m_dz;

	float m_bounding_sphere_radius;
	float radiusX, radiusY, radiusZ;
	float Distance_Squared;
	void CalculateModelCentrePoint();
	void CalculateBoundingSphereRadius();

public:

	float m_bounding_sphere_centre_x, m_bounding_sphere_centre_y, m_bounding_sphere_centre_z;
	Model(ID3D11Device* device, ID3D11DeviceContext* contex);
	~Model();
	int LoadObjModel(char* filename);
	void LoadTextureForModel(char* filename);
	void Draw(XMMATRIX *world, XMMATRIX* view, XMMATRIX* projection);

	XMVECTOR GetBoundingSphereWorldSpacePosition();
	bool CheckCollision(Model*);
	float GetBoundingSphereRadius();
	float GetBoundingSphere_x();
	float GetBoundingSphere_y();
	float GetBoundingSphere_z();


	void LookAt_XZ(float x, float z);
	void MoveForward(float distance);
};