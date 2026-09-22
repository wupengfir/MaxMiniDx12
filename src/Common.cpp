#include "Common.h"
#include "Resource.h"

std::optional<std::pair<bool,D3D12_RESOURCE_STATES>> GetResourceStatus(ID3D12Resource* resource,D3D12_RESOURCE_STATES status)
{
	auto resourceStatus = Resource::StatusMap.find(resource);
    if (resourceStatus != Resource::StatusMap.end())
    {
		return std::pair(resourceStatus->second.status == status,resourceStatus->second.status);
    }
	return std::nullopt;
}