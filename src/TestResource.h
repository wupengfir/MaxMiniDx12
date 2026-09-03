#pragma once
#include "mesh.h"
class TestResource
{
public:
	static MyMesh* GetTestMesh01()
	{
		MyMesh* mesh = new MyMesh();
		XMFLOAT3 vertices[] = {{-0.3f,  0.5f, 0.2f},{-0.8f, -0.5f, 0.2f},{ 0.2f, -0.5f, 0.2f}};
		XMFLOAT3 colors[] = {{1.0f, 0.0f, 0.0f},{1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f, 0.0f}};
		uint32_t indices[] = { 0, 2, 1 };
		mesh->vertices.insert(mesh->vertices.end(),vertices,vertices+sizeof(vertices)/sizeof(vertices[0]));
		mesh->color.insert(mesh->color.end(),colors,colors+sizeof(colors)/sizeof(colors[0]));
		mesh->indices.insert(mesh->indices.end(),indices,indices+sizeof(indices)/sizeof(indices[0]));
		return mesh;
	}
	static MyMesh* GetTestMesh02()
	{
		MyMesh* mesh = new MyMesh();
		XMFLOAT3 vertices[] = {{0.3f,  0.5f, 0.8f},{-0.2f, -0.5f, 0.8f},{ 0.8f, -0.5f, 0.8f}};
		XMFLOAT3 colors[] = {{0.0f, 0.0f, 1.0f},{0.0f, 0.0f, 1.0f},{ 0.0f, 0.0f, 1.0f}};
		uint32_t indices[] = { 0, 2, 1 };
		mesh->vertices.insert(mesh->vertices.end(),vertices,vertices+sizeof(vertices)/sizeof(vertices[0]));
		mesh->color.insert(mesh->color.end(),colors,colors+sizeof(colors)/sizeof(colors[0]));
		mesh->indices.insert(mesh->indices.end(),indices,indices+sizeof(indices)/sizeof(indices[0]));
		return mesh;
	}
};