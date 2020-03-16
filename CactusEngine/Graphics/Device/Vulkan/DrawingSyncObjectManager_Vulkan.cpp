#include "DrawingSyncObjectManager_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include <assert.h>

using namespace Engine;

DrawingSemaphore_Vulkan::DrawingSemaphore_Vulkan(const VkSemaphore& semaphoreHandle, uint32_t assignedID)
	: semaphore(semaphoreHandle), waitStage(0), id(assignedID)
{

}

TimelineSemaphore_Vulkan::TimelineSemaphore_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const VkSemaphore& semaphoreHandle, uint32_t assignedID)
	: m_pDevice(pDevice), semaphore(semaphoreHandle), id(assignedID), m_waitValue(1), m_signalValue(1)
{
	m_waitInfo = {};
	m_waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	m_waitInfo.semaphoreCount = 1;
	m_waitInfo.pSemaphores = &semaphore;
}

VkResult TimelineSemaphore_Vulkan::Wait(uint64_t timeout)
{
	m_waitInfo.pValues = &m_waitValue;
	return vkWaitSemaphores(m_pDevice->logicalDevice, &m_waitInfo, timeout);
}

void TimelineSemaphore_Vulkan::UpdateTimeline()
{
	m_waitValue = m_signalValue + 1;
	m_signalValue = m_waitValue;
}

DrawingSyncObjectManager_Vulkan::DrawingSyncObjectManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{
	CreateNewSemaphore(3); // TODO: adjust the initial count based on usage scenario
	CreateNewTimelineSemaphore(16);
}

DrawingSyncObjectManager_Vulkan::~DrawingSyncObjectManager_Vulkan()
{
	for (auto& pSemaphore : m_semaphorePool)
	{
		assert(pSemaphore->semaphore != VK_NULL_HANDLE);
		vkDestroySemaphore(m_pDevice->logicalDevice, pSemaphore->semaphore, nullptr);
	}

	for (auto& pSemaphore : m_timelineSemaphorePool)
	{
		assert(pSemaphore->semaphore != VK_NULL_HANDLE);
		vkDestroySemaphore(m_pDevice->logicalDevice, pSemaphore->semaphore, nullptr);
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

std::shared_ptr<TimelineSemaphore_Vulkan> DrawingSyncObjectManager_Vulkan::RequestTimelineSemaphore()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& entry : m_timelineSemaphoreAvailability)
	{
		if (entry.second)
		{
			entry.second = false;
			m_timelineSemaphorePool[entry.first]->UpdateTimeline();
			return m_timelineSemaphorePool[entry.first];
		}
	}

	CreateNewTimelineSemaphore(1);
	uint32_t id = static_cast<uint32_t>(m_timelineSemaphorePool.size() - 1);
	m_timelineSemaphoreAvailability.at(id) = false;
	return m_timelineSemaphorePool[id];
}

void DrawingSyncObjectManager_Vulkan::ReturnSemaphore(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_semaphoreAvailability.at(pSemaphore->id) = true;
}

void DrawingSyncObjectManager_Vulkan::ReturnTimelineSemaphore(std::shared_ptr<TimelineSemaphore_Vulkan> pSemaphore)
{
	m_timelineSemaphoreAvailability.at(pSemaphore->id) = true;	
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

bool DrawingSyncObjectManager_Vulkan::CreateNewTimelineSemaphore(uint32_t count, uint64_t initialValue)
{
	assert(m_timelineSemaphorePool.size() + count <= MAX_TIMELINE_SEMAPHORE_COUNT);

	VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
	timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue = initialValue;

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = &timelineCreateInfo;

	for (uint32_t i = 0; i < count; ++i)
	{
		VkSemaphore newSemaphore;
		if (vkCreateSemaphore(m_pDevice->logicalDevice, &semaphoreInfo, nullptr, &newSemaphore) == VK_SUCCESS)
		{
			uint32_t newID = static_cast<uint32_t>(m_timelineSemaphorePool.size()); // We use current pool size as assigned ID
			m_timelineSemaphorePool.emplace_back(std::make_shared<TimelineSemaphore_Vulkan>(m_pDevice, newSemaphore, newID));
			m_timelineSemaphoreAvailability.emplace(newID, true);
		}
		else
		{
			throw std::runtime_error("Vulkan: Failed to create timeline semaphore.");
			return false;
		}
	}

	return true;
}