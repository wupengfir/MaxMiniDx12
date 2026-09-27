#pragma once
#include"Common.h"
#include "BasicComponents.h"
class Camera
{
private :
	bool m_viewdirty = true;
	bool m_projdirty = true;
	Transform m_transform;
	float m_near = 0.05;
	float m_far = 1000;
	float m_fov = XMConvertToRadians(60);
	float m_aspect = 1280.0/720.0;
	float m_width = 50.0;
	float m_height = 50.0;
	XMMATRIX m_ProjectionMatrix;
	XMMATRIX m_ViewMatrix;
	XMMATRIX m_InvVP;
public :
	bool perspective = true;
	Transform& GetTransform() { return m_transform; }
	float SetNear(float value) { m_near = value; m_projdirty = true; };
	float GetNear() { return m_near; };
	float SetFar(float value) { m_far = value; m_projdirty = true; };
	float GetFar() { return m_far; };
	float SetFov(float value) { m_fov = value; m_projdirty = true; };
	float GetFov() { return m_fov; };
	float SetAspect(float value) { m_aspect = value; m_projdirty = true; };
	float GetAspect() { return m_aspect; };
	float SetWidth(float value) { m_width = value; m_projdirty = true; };
	float GetWidth() { return m_width; };
	float SetHeight(float value) { m_height = value; m_projdirty = true; };
	float GetHeight() { return m_height; };
	void SetPos(XMFLOAT3 value) { m_transform.SetPos(value); m_viewdirty = true; }
	void SetRotation(XMFLOAT3 value) { m_transform.SetRotation(value); m_viewdirty = true; }

	const XMMATRIX& ProjectionMatrix()
	{
		if (m_projdirty)
		{
			m_ProjectionMatrix = perspective ? XMMatrixPerspectiveFovLH(m_fov, m_aspect, m_near, m_far) : XMMatrixOrthographicLH(m_width, m_height, m_near, m_far);
		}
		m_projdirty = false;
		return m_ProjectionMatrix;
	}
	const XMMATRIX& ViewMatrix()
	{
		const XMMATRIX cameraToDxView = XMMatrixSet(
			/*0, 0, 1, 0,
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1);*/
			1, 0, 0, 0,
			0, 0, 1, 0,
			0, 1, 0, 0,
			0, 0, 0, 1);
		if (m_viewdirty)
		{
			//XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(-m_transform.GetRotation().x,-m_transform.GetRotation().y,-m_transform.GetRotation().z);
			//XMMATRIX translationMatrix = XMMatrixTranslation(-m_transform.GetPos().x, -m_transform.GetPos().y, -m_transform.GetPos().z);
			m_ViewMatrix = m_transform.WorldMatrixInv() * cameraToDxView;
		}
		
		m_viewdirty = false;
		return m_ViewMatrix;
	}

	XMMATRIX& InvVPMatrix()
	{
		m_InvVP = XMMatrixInverse(nullptr, ViewMatrix() * ProjectionMatrix());
		return m_InvVP;
	}
};