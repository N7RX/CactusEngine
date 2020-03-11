#include "DrawingSyncObjectManager_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include <assert.h>

using namespace Engine;

DrawingSemaphore_Vulkan::DrawingSemaphore_Vulkan(const VkSemaphore& semaphoreHandle, uint32_t assignedID)
	: semaphore(semaphoreHandle), waitStage(0), id(assignedID)
{

}

DrawingFence_Vulkan::DrawingFence_Vulkan(const VkFence& fenceHandle, uint32_t assignedID)
	: fence(fenceHandle), id(assignedID), m_signaled(false)
{

}

void DrawingFence_Vulkan::Wait()
{
	std::unique_lock<std::mutex> lock(m_fenceMutex);
	m_fenceCv.wait(lock, [this]() { return m_signaled; });
	m_signaled = false;
}

void DrawingFence_Vulkan::Notify()
{
	{
		std::lock_guard<std::mutex> guard(m_fenceMutex);
		assert(!m_signaled);
		m_signaled = true;
	}

	m_fenceCv.notify_all();
}

DrawingSyncObjectManager_Vulkan::DrawingSyncObjectManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{
	CreateNewSemaphore(8); // TODO: adjust the initial count based on usage scenario
	CreateNewFence(4);
}

DrawingSyncObjectManager_Vulkan::~DrawingSyncObjectManager_Vulkan()
{
	for (auto& pSemaphore : m_semaphorePool)
	{
		assert(pSemaphore->semaphore != VK_NULL_HANDLE);
		vkDestroySemaphore(m_pDevice->logicalDevice, pSemaphore->semaphore, nullptr);
	}

	for (auto& pFence : m_fencePool)
	{
		assert(pFence->fence != VK_NULL_HANDLE);
		vkDestroyFence(m_pDevice->logicalDevice, pFence->fence, nullptr);
	}
}

std::shared_ptr<DrawingSemaphore_Vulkan> DrawingSyncObjectManager_Vulkan::RequestSemaphore()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& entry : m_semaphoreAvailability)
	{
		if (entry.second)
		{
			entry.second = false;
			return m_semaphorePool[entry.first];
		}
	}

	CreateNewSemaphore(1);
	uint32_t id = static_cast<uint32_t>(m_semaphorePool.size() - 1);
	m_semaphoreAvailability.at(id) = false;
	return m_semaphorePool[id];
}

std::shared_ptr<DrawingFence_Vulkan> DrawingSyncObjectManager_Vulkan::RequestFence()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& entry : m_fenceAvailability)
	{
		if (entry.second)
		{
			entry.second = false;
			m_fencePool[entry.first]->m_signaled = false;
			return m_fencePool[entry.first];
		}
	}

	CreateNewFence(1);
	uint32_t id = static_cast<uint32_t>(m_fencePool.size() - 1);
	m_fenceAvailability.at(id) = false;
	m_fencePool[id]->m_signaled = false;
	return m_fencePool[id];
}

void DrawingSyncObjectManager_Vulkan::ReturnSemaphore(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_semaphoreAvailability.at(pSemaphore->id) = true;
}

void DrawingSyncObjectManager_Vulkan::ReturnFence(std::shared_ptr<DrawingFence_Vulkan> pFence)
{
	vkResetFences(m_pDevice->logicalDevice, 1, &pFence->fence);
	m_fenceAvailability.at(pFence->id) = true;
}

bool DrawingSyncObjectManager_Vulkan::CreateNewSemaphore(uint32_t count)
{
	assert(m_semaphorePool.size() + count <= MAX_SEMAPHORE_COUNT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < count; ++i)
	{
		VkSemaphore newSemaphore;
		if (vkCreateSemaphore(m_pDevice->logicalDevice, &semaphoreInfo, nullptr, &newSemaphore) == VK_SUCCESS)
		{
			uint32_t newID = static_cast<uint32_t>(m_semaphorePool.size()); // We use current pool size as assigned ID
			m_semaphorePool.emplace_back(std::make_shared<DrawingSemaphore_Vulkan>(newSemaphore, newID));
			m_semaphoreAvailability.emplace(newID, true);
		}
		else
		{
			throw std::runtime_error("Vulkan: Failed to create semaphore.");
			return false;
		}
	}

	return true;
}

bool DrawingSyncObjectManager_Vulkan::CreateNewFence(uint32_t count, bool signaled)
{
	assert(m_fencePool.size() + count <= MAX_FENCE_COUNT);

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	for (uint32_t i = 0; i < count; ++i)
	{
		VkFence newFence;
		if (vkCreateFence(m_pDevice->logicalDevice, &fenceInfo, nullptr, &newFence) == VK_SUCCESS)
		{
			uint32_t newID = static_cast<uint32_t>(m_fencePool.size()); // We use current pool size as assigned ID
			m_fencePool.emplace_back(std::make_shared<DrawingFence_Vulkan>(newFence, newID));
			m_fenceAvailability.emplace(newID, true);			
		}
		else
		{
			throw std::runtime_error("Vulkan: Failed to create new fence.");
			return false;
		}
	}

	return true;
}