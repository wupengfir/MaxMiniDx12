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
	XMMATRIX m_ProjectionMatrix;
	XMMATRIX m_ViewMatrix;
public :
	Transform& GetTransform() { return m_transform; }
	float SetNear(float value) { m_near = value; m_projdirty = true; };
	float GetNear() { return m_near; };
	float SetFar(float value) { m_far = value; m_projdirty = true; };
	float GetFar() { return m_far; };
	float SetFov(float value) { m_fov = value; m_projdirty = true; };
	float GetFov() { return m_fov; };
	float SetAspect(float value) { m_aspect = value; m_projdirty = true; };
	float GetAspect() { return m_aspect; };
	void SetPos(XMFLOAT3 value) { m_transform.SetPos(value); m_viewdirty = true; }
	void SetRotation(XMFLOAT3 value) { m_transform.SetRotation(value); m_viewdirty = true; }

	const XMMATRIX& ProjectionMatrix()
	{
		if (m_projdirty)
		{
			m_ProjectionMatrix = XMMatrixPerspectiveFovLH(m_fov,m_aspect,m_near,m_far);
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
};