#include "Material.h"
#include <unordered_set>
#include "context.h"
void to_upper_inplace(std::string& s)
{
	for (char& c : s)
	{
		c = std::toupper(c);
	}
}

bool Material::Create()
{
	std::string shaderContent = m_shader->GetContent();
	if (shaderContent.empty())
		return false;
	if (!(m_shader->Compile(shaderContent, m_shader->GetPath().c_str(), "VS", "vs_5_0", m_shader->GetVsBlob())))
	{
		return false;
	}
	if (!(m_shader->Compile(shaderContent, m_shader->GetPath().c_str(), "PS", "ps_5_0", m_shader->GetPsBlob())))
	{
		return false;
	}
	ComPtr<ID3D12ShaderReflection> reflection;
	ID3DBlob* blob = m_shader->GetVsBlob().Get();
	D3D12_SHADER_INPUT_BIND_DESC bindDesc;

	D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),IID_PPV_ARGS(&reflection));	
	for (size_t i = 0; i < reflection->GetNumInterfaceSlots(); i++)
	{
		reflection->GetResourceBindingDesc(0,&bindDesc);
		m_shader->GetVsInputDesc().push_back(bindDesc);
	}
	blob = m_shader->GetPsBlob().Get();
	D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),IID_PPV_ARGS(&reflection));	
	for (size_t i = 0; i < reflection->GetNumInterfaceSlots(); i++)
	{
		reflection->GetResourceBindingDesc(0,&bindDesc);
		m_shader->GetPsInputDesc().push_back(bindDesc);
	}


	
	std::unordered_map<std::string, D3D12_ROOT_PARAMETER*> deDoubleSet{};
	std::vector<D3D12_ROOT_PARAMETER> paramsList{};
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplerList{};
	for (size_t i = 0; i < m_shader->GetVsInputDesc().size(); i++)
	{
		D3D12_ROOT_PARAMETER_TYPE type = D3D12_ROOT_PARAMETER_TYPE_UAV;
		switch (m_shader->GetVsInputDesc()[i].Type)
		{
		case D3D_SIT_CBUFFER:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_CBV;
			paramsList[i].ParameterType = type;
			paramsList[i].Descriptor.ShaderRegister = m_shader->GetVsInputDesc()[0].BindPoint;
			paramsList[i].Descriptor.RegisterSpace = m_shader->GetVsInputDesc()[0].Space;
			paramsList[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			deDoubleSet.insert(std::make_pair(m_shader->GetVsInputDesc()[i].Name, &paramsList[i]));
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_SRV;
			paramsList[i].ParameterType = type;
			paramsList[i].Descriptor.ShaderRegister = m_shader->GetVsInputDesc()[0].BindPoint;
			paramsList[i].Descriptor.RegisterSpace = m_shader->GetVsInputDesc()[0].Space;
			paramsList[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			deDoubleSet.insert(std::make_pair(m_shader->GetVsInputDesc()[i].Name, &paramsList[i]));
			break;
		//case D3D_SIT_SAMPLER:
		//	std::string name = m_shader->GetVsInputDesc()[i].Name;
		//	to_upper_inplace(name);
		//	D3D12_STATIC_SAMPLER_DESC sampler{};
		//	sampler.ShaderRegister = m_shader->GetVsInputDesc()[0].BindPoint;
		//	sampler.RegisterSpace = m_shader->GetVsInputDesc()[0].Space;
		//	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		//	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		//	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		//	sampler.Filter = name.find("POINT") != std::string::npos ? D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		//	samplerList.push_back(sampler);
		//	break;
		}
		
	}
	
	for (size_t i = 0; i < m_shader->GetPsInputDesc().size(); i++)
	{	
		if (!deDoubleSet.insert({ m_shader->GetVsInputDesc()[i].Name,nullptr }).second)
		{
			for (auto& p : deDoubleSet)
			{
				if (p.first == m_shader->GetVsInputDesc()[i].Name)
				{
					p.second->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					break;
				}
					
			}
			continue;
		}
			
		D3D12_ROOT_PARAMETER_TYPE type = D3D12_ROOT_PARAMETER_TYPE_UAV;
		switch (m_shader->GetPsInputDesc()[i].Type)
		{
		case D3D_SIT_CBUFFER:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_CBV;
			paramsList[i].ParameterType = type;
			paramsList[i].Descriptor.ShaderRegister = m_shader->GetPsInputDesc()[0].BindPoint;
			paramsList[i].Descriptor.RegisterSpace = m_shader->GetPsInputDesc()[0].Space;
			paramsList[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			deDoubleSet.insert(std::make_pair(m_shader->GetVsInputDesc()[i].Name, &paramsList[i]));
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_SRV;
			paramsList[i].ParameterType = type;
			paramsList[i].Descriptor.ShaderRegister = m_shader->GetPsInputDesc()[0].BindPoint;
			paramsList[i].Descriptor.RegisterSpace = m_shader->GetPsInputDesc()[0].Space;
			paramsList[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			deDoubleSet.insert(std::make_pair(m_shader->GetVsInputDesc()[i].Name, &paramsList[i]));
			break;
		case D3D_SIT_SAMPLER:
			std::string name = m_shader->GetPsInputDesc()[i].Name;
			to_upper_inplace(name);
			D3D12_STATIC_SAMPLER_DESC sampler{};
			sampler.ShaderRegister = m_shader->GetPsInputDesc()[0].BindPoint;
			sampler.RegisterSpace = m_shader->GetPsInputDesc()[0].Space;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.Filter = name.find("POINT") != std::string::npos ? D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			samplerList.push_back(sampler);
			break;
		}
		
	}

	D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = paramsList.size();
    rsDesc.pParameters = paramsList.data();
	if (samplerList.size() > 0)
	{
		rsDesc.NumStaticSamplers = samplerList.size();
		rsDesc.pStaticSamplers = samplerList.data();
	}
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rsBlob, rsError;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsError))) {
        if (rsError) ShowError((char*)rsError->GetBufferPointer());
        return false;
    }
    if (FAILED(m_device->DxDevice()->CreateRootSignature(0, rsBlob->GetBufferPointer(),
        rsBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)))) {
        return false;
    }

    m_psoDesc.pRootSignature = m_rootSig.Get();
    m_psoDesc.VS = { m_shader->GetVsBlob()->GetBufferPointer(), m_shader->GetVsBlob()->GetBufferSize() };
    m_psoDesc.PS = { m_shader->GetPsBlob()->GetBufferPointer(), m_shader->GetPsBlob()->GetBufferSize() };
    //m_psoDesc.InputLayout = { layout, 2 };
    m_psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    m_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    m_psoDesc.RasterizerState.DepthClipEnable = TRUE;
    m_psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    m_psoDesc.DepthStencilState.DepthEnable = TRUE;
    m_psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    m_psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;  // 深度小的（近）通过
    m_psoDesc.DepthStencilState.StencilEnable = FALSE;
    m_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    m_psoDesc.SampleMask = UINT_MAX;
    m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_psoDesc.NumRenderTargets = 1;
    m_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_psoDesc.SampleDesc.Count = 1;

	return true;
}
