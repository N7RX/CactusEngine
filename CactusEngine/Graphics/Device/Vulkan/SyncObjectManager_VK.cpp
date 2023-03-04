#include "SyncObjectManager_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "MemoryAllocator.h"

namespace Engine
{
	Semaphore_VK::Semaphore_VK(const VkSemaphore& semaphoreHandle, uint32_t assignedID)
		: semaphore(semaphoreHandle),
		waitStage(0),
		id(assignedID)
	{

	}

	TimelineSemaphore_VK::TimelineSemaphore_VK(LogicalDevice_VK* pDevice, const VkSemaphore& semaphoreHandle, uint32_t assignedID)
		: m_pDevice(pDevice),
		semaphore(semaphoreHandle),
		waitStage(0),
		id(assignedID),
		m_waitValue(1),
		m_signalValue(1)
	{
		m_waitInfo = {};
		m_waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		m_waitInfo.semaphoreCount = 1;
		m_waitInfo.pSemaphores = &semaphore;
	}

	VkResult TimelineSemaphore_VK::Wait(uint64_t timeout)
	{
		m_waitInfo.pValues = &m_waitValue;
		return vkWaitSemaphores(m_pDevice->logicalDevice, &m_waitInfo, timeout);
	}

	void TimelineSemaphore_VK::UpdateTimeline()
	{
		m_waitValue = m_signalValue + 1;
		m_signalValue = m_waitValue;
	}

	SyncObjectManager_VK::SyncObjectManager_VK(LogicalDevice_VK* pDevice)
		: m_pDevice(pDevice)
	{
		CreateNewSemaphore(3);
		CreateNewTimelineSemaphore(16);
	}

	SyncObjectManager_VK::~SyncObjectManager_VK()
	{
		for (auto& pSemaphore : m_semaphorePool)
		{
			DEBUG_ASSERT_CE(pSemaphore->semaphore != VK_NULL_HANDLE);
			vkDestroySemaphore(m_pDevice->logicalDevice, pSemaphore->semaphore, nullptr);
		}

		for (auto& pSemaphore : m_timelineSemaphorePool)
		{
			DEBUG_ASSERT_CE(pSemaphore->semaphore != VK_NULL_HANDLE);
			vkDestroySemaphore(m_pDevice->logicalDevice, pSemaphore->semaphore, nullptr);
		}
	}

	Semaphore_VK* SyncObjectManager_VK::RequestSemaphore()
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

	TimelineSemaphore_VK* SyncObjectManager_VK::RequestTimelineSemaphore()
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

	void SyncObjectManager_VK::ReturnSemaphore(Semaphore_VK* pSemaphore)
	{
		m_semaphoreAvailability.at(pSemaphore->id) = true;
	}

	void SyncObjectManager_VK::ReturnTimelineSemaphore(TimelineSemaphore_VK* pSemaphore)
	{
		m_timelineSemaphoreAvailability.at(pSemaphore->id) = true;
	}

	bool SyncObjectManager_VK::CreateNewSemaphore(uint32_t count)
	{
		DEBUG_ASSERT_CE(m_semaphorePool.size() + count <= MAX_SEMAPHORE_COUNT);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < count; ++i)
		{
			VkSemaphore newSemaphore;
			if (vkCreateSemaphore(m_pDevice->logicalDevice, &semaphoreInfo, nullptr, &newSemaphore) == VK_SUCCESS)
			{
				uint32_t newID = static_cast<uint32_t>(m_semaphorePool.size()); // We use current pool size as assigned ID
				Semaphore_VK* pSemaphore;
				CE_NEW(pSemaphore, Semaphore_VK, newSemaphore, newID);
				m_semaphorePool.emplace_back(pSemaphore);
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

	bool SyncObjectManager_VK::CreateNewTimelineSemaphore(uint32_t count, uint64_t initialValue)
	{
		DEBUG_ASSERT_CE(m_timelineSemaphorePool.size() + count <= MAX_TIMELINE_SEMAPHORE_COUNT);

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
				TimelineSemaphore_VK* pTimelineSemaphore;
				CE_NEW(pTimelineSemaphore, TimelineSemaphore_VK, m_pDevice, newSemaphore, newID);
				m_timelineSemaphorePool.emplace_back(pTimelineSemaphore);
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
}