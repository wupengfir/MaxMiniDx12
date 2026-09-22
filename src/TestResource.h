#pragma once
#include "mesh.h"
class TestResource
{
private:
	
public:

	inline static Mesh* QuadMesh = nullptr;

	static Mesh* GetTestMesh01()
	{
		Mesh* mesh = Mesh::Create();
		XMFLOAT3 vertices[] = {{-0.3f,  0.5f, 0.2f},{-0.8f, -0.5f, 0.2f},{ 0.2f, -0.5f, 0.2f}};
		XMFLOAT4 colors[] = {{1.0f, 0.0f, 0.0f,1.0f},{1.0f, 0.0f, 0.0f,1.0f},{ 1.0f, 0.0f, 0.0f,1.0f}};
        XMFLOAT4 uvs[] = {{0.0f, 0.0f, 0.0f, 0.0f},{0.5f, 1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f, 0.0f, 0.0f}};
		uint32_t indices[] = { 0, 2, 1 };
		mesh->vertices.insert(mesh->vertices.end(),vertices,vertices+sizeof(vertices)/sizeof(vertices[0]));
		mesh->color.insert(mesh->color.end(),colors,colors+sizeof(colors)/sizeof(colors[0]));
		mesh->indices.insert(mesh->indices.end(),indices,indices+sizeof(indices)/sizeof(indices[0]));
        mesh->uv0.insert(mesh->uv0.end(),uvs,uvs+sizeof(uvs)/sizeof(uvs[0]));
		mesh->IndicesOffsets.push_back({ 0, (UINT)mesh->indices.size()});
		return mesh;
	}
	static Mesh* GetTestMesh02()
	{
		Mesh* mesh = Mesh::Create();
		XMFLOAT3 vertices[] = {{0.3f,  0.5f, 0.8f},{-0.2f, -0.5f, 0.8f},{ 0.8f, -0.5f, 0.8f}};
		XMFLOAT4 colors[] = {{0.0f, 0.0f, 1.0f,1.0f},{0.0f, 0.0f, 1.0f,1.0f},{ 0.0f, 0.0f, 1.0f,1.0f}};
        XMFLOAT4 uvs[] = {{0.0f, 0.0f, 0.0f, 0.0f},{0.5f, 1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f, 0.0f, 0.0f}};
		uint32_t indices[] = { 0, 2, 1 };
		mesh->vertices.insert(mesh->vertices.end(),vertices,vertices+sizeof(vertices)/sizeof(vertices[0]));
		mesh->color.insert(mesh->color.end(),colors,colors+sizeof(colors)/sizeof(colors[0]));
		mesh->indices.insert(mesh->indices.end(),indices,indices+sizeof(indices)/sizeof(indices[0]));
        mesh->uv0.insert(mesh->uv0.end(),uvs,uvs+sizeof(uvs)/sizeof(uvs[0]));
		mesh->IndicesOffsets.push_back({ 0, (UINT)mesh->indices.size()});
		return mesh;
	}

    static Mesh* GetMeshQuad()
	{
		Mesh* mesh = Mesh::Create();
		XMFLOAT3 vertices[] = {{-1,-1,0},{-1,1,0},{ 1,1,0},{ 1,-1,0}};
		XMFLOAT4 colors[4] = {};
        XMFLOAT4 uvs[] = {{0.0f, 0.0f, 0.0f, 0.0f},{0.f, 1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f, 0.0f, 0.0f}};
		uint32_t indices[] = { 0, 2, 1 ,0,3,2};
		mesh->vertices.insert(mesh->vertices.end(),vertices,vertices+sizeof(vertices)/sizeof(vertices[0]));
		mesh->color.insert(mesh->color.end(),colors,colors+sizeof(colors)/sizeof(colors[0]));
		mesh->indices.insert(mesh->indices.end(),indices,indices+sizeof(indices)/sizeof(indices[0]));
        mesh->uv0.insert(mesh->uv0.end(),uvs,uvs+sizeof(uvs)/sizeof(uvs[0]));
		mesh->IndicesOffsets.push_back({ 0, (UINT)mesh->indices.size()});
		return mesh;
	}

	static void GenerateCheckerboard(
    int width,
    int height,
    uint8_t* data,
    int tileSize = 16,
    const uint8_t color0[4] = nullptr,
    const uint8_t color1[4] = nullptr
) {
    // 默认颜色：黑色和白色
    const uint8_t defaultColor0[4] = {0, 0, 0, 255};
    const uint8_t defaultColor1[4] = {255, 255, 255, 255};
    
    const uint8_t* c0 = color0 ? color0 : defaultColor0;
    const uint8_t* c1 = color1 ? color1 : defaultColor1;
    
    int bytesPerPixel = 4;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int tileX = x / tileSize;
            int tileY = y / tileSize;
            
            // 棋盘格模式
            bool isColor0 = ((tileX + tileY) % 2) == 0;
            const uint8_t* color = isColor0 ? c0 : c1;
            
            int pixelIndex = (y * width + x) * bytesPerPixel;
            data[pixelIndex + 0] = color[0];  // R
            data[pixelIndex + 1] = color[1];  // G
            data[pixelIndex + 2] = color[2];  // B
            data[pixelIndex + 3] = color[3];  // A
        }
    }
}


	static TextureBuffer* GetTestTexture()
	{
		TextureBuffer* texture = new TextureBuffer();
		texture->Width = 64;
		texture->Height = 64;
		texture->CreateTexture();

        return texture;
	}

};