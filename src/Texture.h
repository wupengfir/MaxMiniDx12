#pragma once
#include "Common.h"
#include <DirectXTex.h>
#include "Resource.h"
class TextureLoader
{
public:
	ScratchImage image;
	HRESULT LoadImageX(const std::wstring& path,TextureBuffer* texture)
	{
		HRESULT hr = S_FALSE;
		auto list = Split(path,L'.');
		to_lower_inplace(list[list.size() - 1]);
		if (list[list.size() - 1] == L"dds")
		{
			hr = DirectX::LoadFromDDSFile(path.c_str(),DirectX::DDS_FLAGS_NONE,nullptr,image);
			//texture->Format = image.GetMetadata().format;
			if (image.GetMetadata().IsCubemap())
			{
				texture->SrvDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			}
			else if (image.GetMetadata().IsVolumemap())
			{
				texture->SrvDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			}
		}			
		else if (list[list.size() - 1] == L"exr")
		{
			hr = DirectX::LoadFromHDRFile(path.c_str(),nullptr,image);
			//texture->Format = image.GetMetadata().format;
		}
			
		else if(list[list.size()-1] == L"tga")
		{
			//texture->Format = texture->Srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
			hr = DirectX::LoadFromTGAFile(path.c_str(),nullptr,image);
		}			
		else
		{
			//texture->Format = texture->Srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
			hr = DirectX::LoadFromWICFile(path.c_str(),WIC_FLAGS_NONE,nullptr,image);
		}
		texture->Format = texture->Srgb ? DirectX::MakeSRGB(image.GetMetadata().format) : image.GetMetadata().format;
		return hr;
	}
};

class Texture : public TextureBuffer
{
private:
	TextureLoader m_loader;
public:

	const Image* GetImage()
	{
		return m_loader.image.GetImages();
	}

	void LoadTexture(const std::wstring& path,bool useMip,bool srgb)
	{
		this->Srgb = srgb;
		if (SUCCEEDED(m_loader.LoadImageX(path,this)))
		{			
			DirectX::ScratchImage mipChain;
			if (useMip )//&& this->SrvDimension != D3D12_SRV_DIMENSION_TEXTURECUBE)
			{
				GenerateMipMaps(m_loader.image.GetImages(), m_loader.image.GetImageCount(),m_loader.image.GetMetadata(),TEX_FILTER_DEFAULT,0,mipChain);
				m_loader.image.Release();
				m_loader.image = std::move(mipChain);
			}
			this->MipLevels = m_loader.image.GetMetadata().mipLevels;
			this->Width = m_loader.image.GetMetadata().width;
			this->Height = m_loader.image.GetMetadata().height;
			this->Depth = this->SrvDimension != D3D12_SRV_DIMENSION_TEXTURECUBE?m_loader.image.GetMetadata().depth : 6;
			this->SubresourceCount = m_loader.image.GetImageCount();
			this->CreateTexture();
		}
	}
};