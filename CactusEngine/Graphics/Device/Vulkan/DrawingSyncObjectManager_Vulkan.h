#pragma once
#include "NoCopy.h"
#include "CommandResources.h"

#include <vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace Engine
{
	struct LogicalDevice_Vulkan;

	class DrawingSemaphore_Vulkan
	{
	public:
		DrawingSemaphore_Vulkan(const VkSemaphore& semaphoreHandle, uint32_t assignedID);

	public:
		VkSemaphore semaphore;
		VkPipelineStageFlags waitStage;

	private:
		uint32_t id;

		friend class DrawingSyncObjectManager_Vulkan;
	};

	// Timeline semaphore must be returned after each use to update timeline
	// TODO: add external updating support
	class TimelineSemaphore_Vulkan : public DrawingSemaphore
	{
	public:
		TimelineSemaphore_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const VkSemaphore& semaphoreHandle, uint32_t assignedID);

		VkResult Wait(uint64_t timeout = UINT64_MAX);

	private:
		void UpdateTimeline();

	public:
		VkSemaphore semaphore;
		VkPipelineStageFlags waitStage;

		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;

	private:
		uint32_t id;
		uint64_t m_signalValue;
		uint64_t m_waitValue;
		VkSemaphoreWaitInfo m_waitInfo;

		friend class DrawingSyncObjectManager_Vulkan;
		friend class DrawingCommandManager_Vulkan;
	};

	class DrawingSyncObjectManager_Vulkan : public NoCopy
	{
	public:
		DrawingSyncObjectManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingSyncObjectManager_Vulkan();

		std::shared_ptr<DrawingSemaphore_Vulkan> RequestSemaphore();
		std::shared_ptr<TimelineSemaphore_Vulkan> RequestTimelineSemaphore();

		void ReturnSemaphore(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void ReturnTimelineSemaphore(std::shared_ptr<TimelineSemaphore_Vulkan> pSemaphore);

	private:
		bool CreateNewSemaphore(uint32_t count);
		bool CreateNewTimelineSemaphore(uint32_t count, uint64_t initialValue = 0);

	public:
		const uint32_t MAX_SEMAPHORE_COUNT = 64;
		const uint32_t MAX_TIMELINE_SEMAPHORE_COUNT = 192;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		mutable std::mutex m_mutex;

		// TODO: make the pool easier to resize
		std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> m_semaphorePool;
		std::vector<std::shared_ptr<TimelineSemaphore_Vulkan>> m_timelineSemaphorePool;

		std::unordered_map<uint32_t, bool> m_semaphoreAvailability;
		std::unordered_map<uint32_t, bool> m_timelineSemaphoreAvailability;
	};
}