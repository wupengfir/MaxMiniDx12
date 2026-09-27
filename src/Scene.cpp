#include "Scene.h"
#include "context.h"
#include "Camera.h"
GameObject::GameObject()
{
	uuid = nextID.fetch_add(1);
	Scene::CurrentScene->AddGameObject(this);
}

void Scene::DrawScene(ID3D12GraphicsCommandList* cmdList,DXGI_FORMAT formats[],Camera* camera)
{
	
	for (GameObject* obj : m_sceneObjs)
	{
		if (!camera)
			camera = cameras[0];
		if (obj->mesh)
		{
			int index = 0;
			while (index<obj->materials.size() && obj->materials[index])
			{
				Context::pContext->DrawMesh(cmdList,obj->mesh,index,obj->materials[index], &(obj->transform.WorldMatrix()), camera,formats, &(obj->transform.WorldMatrixInv()));
				index++;
			}
		}
		
	}
}

void Scene::DrawSceneShadow(ID3D12GraphicsCommandList* cmdList, XMMATRIX* matrixvp)
{

	for (GameObject* obj : m_sceneObjs)
	{
		if (obj->mesh)
		{
			int index = 0;
			while (index < obj->materials.size() && obj->materials[index])
			{
				Context::pContext->DrawShadow(cmdList, obj->mesh, index, obj->materials[index], &(obj->transform.WorldMatrix()), matrixvp, &(obj->transform.WorldMatrixInv()));
				index++;
			}
		}

	}
}