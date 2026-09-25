#include "Material.h"
#include <unordered_set>
#include "context.h"
#include "Common.h"



bool Material::Create()
{
	
	std::vector<D3D12_ROOT_PARAMETER> paramsList{};
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplerList{};	
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges{};
	ranges.reserve((m_shader != nullptr?m_shader->GetShaderResourceParams():m_computeShader->GetShaderResourceParams()).size());
	for (size_t i = 0; i < (m_shader != nullptr?m_shader->GetShaderResourceParams():m_computeShader->GetShaderResourceParams()).size(); i++)
	{						
		auto& paramDatas = (m_shader != nullptr?m_shader->GetShaderResourceParams():m_computeShader->GetShaderResourceParams());
		D3D12_ROOT_PARAMETER_TYPE type = D3D12_ROOT_PARAMETER_TYPE_UAV;
		int lastIndex = 0;
		switch (paramDatas[i].bindDesc.Type)
		{
		case D3D_SIT_CBUFFER:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_CBV;
			lastIndex = paramsList.size() - 1;
			paramsList[lastIndex].ParameterType = type;
			paramsList[lastIndex].Descriptor.ShaderRegister = paramDatas[i].bindDesc.BindPoint;
			paramsList[lastIndex].Descriptor.RegisterSpace = paramDatas[i].bindDesc.Space;
			paramsList[lastIndex].ShaderVisibility = paramDatas[i].visibility;
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_STRUCTURED:
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_SRV;
			lastIndex = paramsList.size() - 1;
			paramsList[lastIndex].ParameterType = type;
			paramsList[lastIndex].Descriptor.ShaderRegister = paramDatas[i].bindDesc.BindPoint;
			paramsList[lastIndex].Descriptor.RegisterSpace = paramDatas[i].bindDesc.Space;
			paramsList[lastIndex].ShaderVisibility = paramDatas[i].visibility;
			break;
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		{
			paramsList.emplace_back();
			type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			lastIndex = paramsList.size() - 1;
			D3D12_DESCRIPTOR_RANGE range = {};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			range.NumDescriptors = 1;
			range.BaseShaderRegister = paramDatas[i].bindDesc.BindPoint;  // t0
			range.RegisterSpace = paramDatas[i].bindDesc.Space;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			ranges.push_back(range);
			D3D12_ROOT_DESCRIPTOR_TABLE table{1,&ranges.back()};
			paramsList[lastIndex].DescriptorTable = table;
			paramsList[lastIndex].ShaderVisibility = paramDatas[i].visibility;
		}

			break;
		case D3D_SIT_TEXTURE:
			{	
				paramsList.emplace_back();
				type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				lastIndex = paramsList.size() - 1;
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = paramDatas[i].bindDesc.BindPoint;  // t0
				range.RegisterSpace = paramDatas[i].bindDesc.Space;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back(range);
				D3D12_ROOT_DESCRIPTOR_TABLE table{1,&ranges.back()};
				paramsList[lastIndex].DescriptorTable = table;
				paramsList[lastIndex].ShaderVisibility = paramDatas[i].visibility;
			}
			
			break;
		case D3D_SIT_SAMPLER:
			std::string name = paramDatas[i].name;
			to_upper_inplace(name);
			D3D12_STATIC_SAMPLER_DESC sampler{};
			sampler.ShaderRegister = paramDatas[i].bindDesc.BindPoint;
			sampler.RegisterSpace = paramDatas[i].bindDesc.Space;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.MaxLOD = 1024;
			sampler.Filter = name.find("POINT") != std::string::npos ? D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			samplerList.push_back(sampler);
			break;
		}
		
	}

	if (m_shader)
	{
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
		m_psoDesc.RasterizerState.CullMode = CullMode;
		m_psoDesc.RasterizerState.DepthClipEnable = TRUE;
		m_psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		if (BlendEnable)
		{
			auto& rtBlend = m_psoDesc.BlendState.RenderTarget[0];
			rtBlend.BlendEnable = TRUE;
			rtBlend.LogicOpEnable = FALSE;

			rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rtBlend.BlendOp = D3D12_BLEND_OP_ADD;

			rtBlend.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		}
		m_psoDesc.DepthStencilState.DepthEnable = DepthEnable;
		m_psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		m_psoDesc.DepthStencilState.DepthFunc = DepthTest;  // 深度小的（近）通过
		m_psoDesc.DepthStencilState.StencilEnable = FALSE;
		m_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		m_psoDesc.SampleMask = UINT_MAX;
		m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_psoDesc.NumRenderTargets = 1;
		m_psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		m_psoDesc.SampleDesc.Count = 1;
	}
	else if (m_computeShader)
	{
		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = paramsList.size();
		rsDesc.pParameters = paramsList.data();
		if (samplerList.size() > 0)
		{
			rsDesc.NumStaticSamplers = samplerList.size();
			rsDesc.pStaticSamplers = samplerList.data();
		}

		ComPtr<ID3DBlob> rsBlob, rsError;
		if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsError))) {
			if (rsError) ShowError((char*)rsError->GetBufferPointer());
			return false;
		}
		if (FAILED(m_device->DxDevice()->CreateRootSignature(0, rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)))) {
			return false;
		}


		ComputePsoDesc().pRootSignature = m_rootSig.Get();
		ComputePsoDesc().CS = {m_computeShader->GetCsBlob()->GetBufferPointer(), m_computeShader->GetCsBlob()->GetBufferSize()};
		m_device->DxDevice()->CreateComputePipelineState(&ComputePsoDesc(), IID_PPV_ARGS(&GetPso()));

	}

	


	//m_device->SrvHeap()->CreateView();

	return true;
}
