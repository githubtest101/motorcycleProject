#include "Camera.h"
#include <Windows.h>

Camera::Camera(float initial_x, float initial_y, float initial_z, float camera_angle)
{
	m_camera_horizontal = camera_angle;
	m_camera_vertical = camera_angle;
	m_x = initial_x;
	m_y = initial_y;
	m_z = initial_z;
	m_dx = sin(camera_angle * (XM_PI / XMConvertToRadians(180.0)));
	m_dy = tan(camera_angle * (XM_PI / XMConvertToRadians(180.0)));
	m_dz = cos(camera_angle * (XM_PI / XMConvertToRadians(180.0)));

}

void Camera::RotateHorizontal(float degrees)
{
	m_camera_horizontal += degrees;

	m_dx = sin(m_camera_horizontal * (XM_PI / 180));
	m_dz = cos(m_camera_horizontal * (XM_PI / 180));
}

void Camera::RotateVertical(float degrees)
{
	m_camera_vertical += degrees;

	m_dy = tan(m_camera_vertical * (XM_PI / 180));
}

void Camera::Stright(float distance)
{
	m_x += m_dx * distance;
	m_z += m_dz * distance;
}

void Camera::Sideways(float distance)
{
	m_x += distance * cos(XMConvertToRadians(m_camera_horizontal));
	m_z += distance * sin(XMConvertToRadians(m_camera_horizontal));
}

float Camera::getX()
{
	return m_x;
}

float Camera::getY()
{
	return m_y;
}

float Camera::getZ()
{
	return m_z;
}


XMMATRIX Camera::GetViewMatrix()
{
	XMVECTOR position = XMVectorSet(m_x, m_y, m_z, 0.0);
	XMVECTOR lookat = XMVectorSet(m_x + m_dx, m_y + m_dy, m_z + m_dz, 0.0);
	XMVECTOR up = XMVectorSet(0.0, 1.0, 0.0, 0.0);

	XMMATRIX view = XMMatrixLookAtLH(position, lookat, up);

	return view;
}