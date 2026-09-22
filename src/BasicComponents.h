#pragma once
#include "Common.h"
class Transform
{
private :
	bool m_dirty = true;
	XMFLOAT3 m_localPos = XMFLOAT3(0,0,0);
	XMFLOAT3 m_pos = XMFLOAT3(0,0,0);
	XMFLOAT3 m_rotation = XMFLOAT3(0,0,0);
	XMFLOAT3 m_scale = XMFLOAT3(1,1,1);
	XMMATRIX m_WorldMatrix;
	XMMATRIX m_WorldMatrixInv;
public:
	XMFLOAT3 GetPos() const{ return m_pos; };
	XMFLOAT3 GetRotation() const{ return m_rotation; };
	XMFLOAT3 GetScale() const{ return m_scale; };

	void SetPos(XMFLOAT3 value) { m_pos = value; m_dirty = true; }
	void SetRotation(XMFLOAT3 value) { m_rotation = value;m_dirty = true; }
	void SetScale(XMFLOAT3 value) { m_scale = value;m_dirty = true; }

	void Refresh()
	{
		if (m_dirty)
		{
			const XMMATRIX obj2UeWorld = XMMatrixSet(
			0, 1, 0, 0,
			1, 0, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
			//ue坐标系
			XMMATRIX scaleMatreix = XMMatrixScaling(m_scale.x,m_scale.y,m_scale.z);
			// Row-vector convention: apply the stored X, Y, then Z rotations.
			// With camera Y kept at zero, this becomes Rx(pitch) * Rz(yaw).
			XMMATRIX rotationMatrix =
				XMMatrixRotationX(m_rotation.x) *
				XMMatrixRotationY(m_rotation.y) *
				XMMatrixRotationZ(m_rotation.z);
			XMMATRIX translationMatrix = XMMatrixTranslation(m_pos.x,m_pos.y,m_pos.z);
			m_WorldMatrix = scaleMatreix * rotationMatrix * obj2UeWorld * translationMatrix ;
			m_WorldMatrixInv = XMMatrixInverse(nullptr, m_WorldMatrix);
			m_dirty = false;
		}
	}

	const XMMATRIX& WorldMatrix ()
	{
		Refresh();
		return m_WorldMatrix;
	}

	const XMMATRIX& WorldMatrixInv ()
	{
		Refresh();
		return m_WorldMatrixInv;
	}

};
