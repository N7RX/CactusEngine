#pragma once
#include "NoCopy.h"

#include <vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace Engine
{
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

	class DrawingFence_Vulkan
	{
	public:
		DrawingFence_Vulkan(const VkFence& fenceHandle, uint32_t assignedID);
		void Wait(); // This would stall the thread until the fence is hit and recycled
		void Notify();

	public:
		VkFence fence;

	private:
		uint32_t id;
		std::mutex m_fenceMutex;
		std::unique_lock<std::mutex> m_fenceLock;
		std::condition_variable m_fenceCv;

		friend class DrawingSyncObjectManager_Vulkan;
	};

	struct LogicalDevice_Vulkan;
	class DrawingSyncObjectManager_Vulkan : public NoCopy
	{
	public:
		DrawingSyncObjectManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingSyncObjectManager_Vulkan();

		std::shared_ptr<DrawingSemaphore_Vulkan> RequestSemaphore();
		std::shared_ptr<DrawingFence_Vulkan> RequestFence();

		void ReturnSemaphore(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void ReturnFence(std::shared_ptr<DrawingFence_Vulkan> pFence);

	private:
		bool CreateNewSemaphore(uint32_t count);
		bool CreateNewFence(uint32_t count, bool signaled = false);

	public:
		const uint32_t MAX_SEMAPHORE_COUNT = 128;
		const uint32_t MAX_FENCE_COUNT = 64;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		mutable std::mutex m_mutex;

		// TODO: make the pool easier to resize
		std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> m_semaphorePool;
		std::vector<std::shared_ptr<DrawingFence_Vulkan>> m_fencePool;

		std::unordered_map<uint32_t, bool> m_semaphoreAvailability;
		std::unordered_map<uint32_t, bool> m_fenceAvailability;
	};
}