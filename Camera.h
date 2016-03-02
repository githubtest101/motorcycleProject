#pragma once
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>
#include <math.h>


class Camera
{
private:
	float m_x, m_y, m_z;
	float m_dx, m_dy, m_dz;
	float m_camera_horizontal, m_camera_vertical;
	XMVECTOR m_position, m_lookat, m_up;
public:
	Camera(float initial_x, float initial_y, float initial_z, float camera_angle);
	void RotateHorizontal(float degrees);
	void RotateVertical(float degrees);
	void Stright(float distance);
	void Sideways(float distance);
	void Up();
	XMMATRIX GetViewMatrix();
	float getX();
	float getY();
	float getZ();
};