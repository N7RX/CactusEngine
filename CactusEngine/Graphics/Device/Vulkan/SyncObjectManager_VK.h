#pragma once
#include "NoCopy.h"
#include "CommandResources.h"
#include "VulkanIncludes.h"

#include <vector>
#include <unordered_map>
#include <mutex>

namespace Engine
{
	struct LogicalDevice_VK;

	class Semaphore_VK
	{
	public:
		Semaphore_VK(const VkSemaphore& semaphoreHandle, uint32_t assignedID);

	public:
		VkSemaphore semaphore;
		VkPipelineStageFlags waitStage;

	private:
		uint32_t id;

		friend class SyncObjectManager_VK;
	};

	// Timeline semaphore must be returned after each use to update timeline
	// TODO: add external updating support
	class TimelineSemaphore_VK : public GraphicsSemaphore
	{
	public:
		TimelineSemaphore_VK(LogicalDevice_VK* pDevice, const VkSemaphore& semaphoreHandle, uint32_t assignedID);

		VkResult Wait(uint64_t timeout = UINT64_MAX);

	private:
		void UpdateTimeline();

	public:
		VkSemaphore semaphore;
		VkPipelineStageFlags waitStage;

		LogicalDevice_VK* m_pDevice;

	private:
		uint32_t id;
		uint64_t m_signalValue;
		uint64_t m_waitValue;
		VkSemaphoreWaitInfo m_waitInfo;

		friend class SyncObjectManager_VK;
		friend class CommandManager_VK;
	};

	// TODO: add fallback VkFence support

	class SyncObjectManager_VK : public NoCopy
	{
	public:
		SyncObjectManager_VK(LogicalDevice_VK* pDevice);
		~SyncObjectManager_VK();

		Semaphore_VK* RequestSemaphore();
		TimelineSemaphore_VK* RequestTimelineSemaphore();

		void ReturnSemaphore(Semaphore_VK* pSemaphore);
		void ReturnTimelineSemaphore(TimelineSemaphore_VK* pSemaphore);

	private:
		bool CreateNewSemaphore(uint32_t count);
		bool CreateNewTimelineSemaphore(uint32_t count, uint64_t initialValue = 0);

	public:
		const uint32_t MAX_SEMAPHORE_COUNT = 64;
		const uint32_t MAX_TIMELINE_SEMAPHORE_COUNT = 192;

	private:
		LogicalDevice_VK* m_pDevice;
		mutable std::mutex m_mutex;

		// TODO: make the pool easier to resize
		std::vector<Semaphore_VK*> m_semaphorePool;
		std::vector<TimelineSemaphore_VK*> m_timelineSemaphorePool;

		std::unordered_map<uint32_t, bool> m_semaphoreAvailability;
		std::unordered_map<uint32_t, bool> m_timelineSemaphoreAvailability;
	};
}