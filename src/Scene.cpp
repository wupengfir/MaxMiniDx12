#include "Scene.h"
#include "context.h"
#include "Camera.h"
GameObject::GameObject()
{
	uuid = nextID.fetch_add(1);
	Scene::CurrentScene->AddGameObject(this);
}

void Scene::DrawScene(ID3D12GraphicsCommandList* cmdList,UINT cameraIndex,DXGI_FORMAT formats[])
{
	
	for (GameObject* obj : m_sceneObjs)
	{
		/*if (obj->materials[0] && obj->mesh)
		{
			Context::pContext->DrawMesh(cmdList,obj->mesh,obj->material,&(obj->transform.WorldMatrix()),cameras[cameraIndex]);
		}*/
		if (obj->mesh)
		{
			int index = 0;
			while (index<obj->materials.size() && obj->materials[index])
			{
				Context::pContext->DrawMesh(cmdList,obj->mesh,index,obj->materials[index], &(obj->transform.WorldMatrix()), cameras[cameraIndex],formats, &(obj->transform.WorldMatrixInv()));
				index++;
			}
		}
		
	}
}
