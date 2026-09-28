#include "Common.h"
#include "Resource.h"

void SetResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES targetStatus,ID3D12GraphicsCommandList* cmdList)
{
	    auto colorstatus = Resource::StatusMap.find(resource);
        if (colorstatus == Resource::StatusMap.end())
        {
            return;
        }
        if (colorstatus->second.status != targetStatus)
        {
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = resource;
            barrier.Transition.StateBefore = colorstatus->second.status;
            barrier.Transition.StateAfter = targetStatus;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);
            colorstatus->second.status = targetStatus;
        }
}

std::optional<std::pair<bool,D3D12_RESOURCE_STATES>> GetResourceStatus(ID3D12Resource* resource,D3D12_RESOURCE_STATES status)
{
	auto resourceStatus = Resource::StatusMap.find(resource);
    if (resourceStatus != Resource::StatusMap.end())
    {
		return std::pair(resourceStatus->second.status == status,resourceStatus->second.status);
    }
	return std::nullopt;
}