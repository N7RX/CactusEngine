#pragma once
#include "DrawingSyncObjectManager_Vulkan.h"
#include "SharedTypes.h"
#include "SafeQueue.h"
#include "SafeBasicTypes.h"
#include "NoCopy.h"
#include <vulkan.h>
#include <memory>
#include <vector>

namespace Engine
{
	struct DrawingCommandQueue_Vulkan
	{		
		EQueueType type;
		uint32_t queueFamilyIndex;
		VkQueue queue;
	};

	class DrawingCommandBuffer_Vulkan
	{
	public:
		DrawingCommandBuffer_Vulkan(const VkCommandBuffer& cmdBuffer);
		~DrawingCommandBuffer_Vulkan();

		bool IsRecording() const;
		bool InRenderPass() const;
		bool InExecution() const;

	private:
		VkCommandBuffer m_commandBuffer;
		bool m_isRecording;
		bool m_inRenderPass;
		bool m_inExecution;
		uint32_t m_fenceID;

		friend class DrawingCommandManager_Vulkan;
	};

	class QueueSubmitConditionLock_Vulkan
	{
	public:
		QueueSubmitConditionLock_Vulkan(uint32_t id) : lockID(id), inUse(false) {}
		~QueueSubmitConditionLock_Vulkan() = default;

		bool Wait()
		{
			return !GetNotifyState();
		}
		void Notify()
		{
			SetNotifyState(true);
		}

		void Reset()
		{
			alreadyNotified = false; // It should be safe not to use lock guard since this lock has already finished it's duty
			inUse = false;
		}

	private:
		bool GetNotifyState()
		{
			std::lock_guard<std::mutex> guard(notifyStateMutex);
			return alreadyNotified;
		}

		void SetNotifyState(bool val)
		{
			std::lock_guard<std::mutex> guard(notifyStateMutex);
			alreadyNotified = val;
		}

	public:
		uint32_t lockID;
		bool inUse;

	private:
		bool alreadyNotified = false;
		std::mutex notifyStateMutex;
	};

	class CommandSubmitInfo
	{
	public:
		~CommandSubmitInfo()
		{
			Clear();
		}

		void Clear()
		{
			buffersAwaitSubmit.clear();
			waitSemaphores.clear();
			waitStages.clear();
			semaphoresToSignal.clear();
		}

	private:
		VkSubmitInfo submitInfo;
		VkFence		 fence;
		bool		 waitFrameFenceSubmission;
		bool		 initRecycle;
		std::shared_ptr<QueueSubmitConditionLock_Vulkan> submitConditionLock;

		// Retained resources
		std::vector<VkCommandBuffer>	  buffersAwaitSubmit;
		std::vector<VkSemaphore>		  waitSemaphores;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore>		  semaphoresToSignal;

		friend class DrawingCommandManager_Vulkan;
	};

	struct LogicalDevice_Vulkan;
	class DrawingCommandManager_Vulkan : public NoCopy
	{
	public:
		DrawingCommandManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue);
		~DrawingCommandManager_Vulkan();

		void Destroy();

		EQueueType GetWorkingQueueType() const;

		std::shared_ptr<DrawingCommandBuffer_Vulkan> RequestPrimaryCommandBuffer();
		void SubmitCommandBuffers(VkFence fence);
		void AddWaitSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void AddSignalSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);

	private:
		void CreateCommandPool();
		bool AllocatePrimaryCommandBuffer(uint32_t count);

		void SubmitCommandBufferAsync();
		void RecycleCommandBufferAsync();

	public:
		const uint32_t MAX_COMMAND_BUFFER_COUNT = 64;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		DrawingCommandQueue_Vulkan m_workingQueue;
		bool m_isRunning;

		VkCommandPool m_commandPool;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_freeCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inUseCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inExecutionCommandBuffers;

		SafeQueue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_submitWaitSemaphores;
		SafeQueue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_submitSignalSemaphores;

		SafeQueue<std::shared_ptr<CommandSubmitInfo>> m_submissionQueue;

		// Async command submission
		std::thread m_commandBufferSubmissionThread;
		std::mutex m_frameFenceMutex;
		std::unique_lock<std::mutex> m_frameFenceLock;
		std::condition_variable m_frameFenceCv;
		bool m_waitFrameFenceSubmission = false;

		// Asyn command buffer recycle
		std::thread m_commandBufferRecycleThread;
		std::mutex m_commandBufferRecycleMutex;
		std::condition_variable m_commandBufferRecycleCv;
		SafeBool m_commandBufferRecycleFlag;
		const uint64_t RECYCLE_TIMEOUT = UINT64_MAX;
	};
}