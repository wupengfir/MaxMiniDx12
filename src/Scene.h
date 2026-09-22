#pragma once
#include <vector>
#include <basetsd.h>
#include <atomic>
#include <DirectXMath.h>
#include <unordered_map>
#include <wtypes.h>
#include <d3d12.h>
#include "BasicComponents.h"
using namespace DirectX;

class Material;
class Mesh;
class Context;
class Camera;


class GameObject
{private :
	inline static std::atomic<UINT> nextID = 0;
public:
	Transform transform{};
	UINT uuid;
	Mesh* mesh;
	std::vector<Material*> materials;
	GameObject();

};

class Scene
{
private:
	std::unordered_map<UINT, UINT> m_sceneObjsMap;
	std::vector<GameObject*> m_sceneObjs;
	
public:
	inline static Scene* CurrentScene = nullptr;
	void AddGameObject(GameObject* obj)
	{
		if (!m_sceneObjsMap.contains(obj->uuid))
		{
			m_sceneObjsMap[obj->uuid] = m_sceneObjs.size();
			m_sceneObjs.push_back(obj);
		}
	}
	std::vector<Camera*> cameras;


	void RemoveObject(GameObject* obj)
	{
		auto it = m_sceneObjsMap.find(obj->uuid);
		if (it == m_sceneObjsMap.end()) return;
		size_t index = it->second;
		size_t lastIndex = m_sceneObjs.size() - 1;
		m_sceneObjs[index] = m_sceneObjs[lastIndex];
		m_sceneObjsMap.erase(obj->uuid);
		m_sceneObjsMap[m_sceneObjs[index]->uuid] = index;
	}

	void DrawScene(ID3D12GraphicsCommandList* cmdList,UINT cameraIndex,DXGI_FORMAT formats[]);

	Scene()
	{
		if (CurrentScene)
		{
			delete CurrentScene;
		}
		CurrentScene = this;
	}

};