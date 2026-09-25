#pragma once
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <d3d12.h>
#include <vector>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>
#include <optional>
using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct RenderSetting
{
	float PrevRenderScale = 1;
	float RenderScale = 1;
	float Exposure = 0;
	XMFLOAT4 MainLightDirection{0,0,1,0};
	XMFLOAT4 MainLightColor{8,8,6,0};
};

	template<typename T>
	void SwapValue(T& a,T& b)
	{
		T temp = a;
		a = b;
		b = temp;
	}

inline void to_lower_inplace(std::string& s)
{
	for (char& c : s)
	{
		c = std::tolower(c);
	}
}

inline void to_lower_inplace(std::wstring& s)
{
	for (wchar_t& c : s)
	{
		c = std::tolower(c);
	}
}

inline void to_upper_inplace(std::string& s)
{
	for (char& c : s)
	{
		c = std::toupper(c);
	}
}

inline void to_upper_inplace(std::wstring& s)
{
	for (wchar_t& c : s)
	{
		c = std::toupper(c);
	}
}

inline std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::vector<std::wstring> Split(const std::wstring& str, wchar_t delimiter) {
    std::vector<std::wstring> tokens;
    std::wstringstream ss(str);
    std::wstring token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::optional<std::pair<bool, D3D12_RESOURCE_STATES>> GetResourceStatus(ID3D12Resource* resource, D3D12_RESOURCE_STATES status);