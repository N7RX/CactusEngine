#pragma once
#include "DrawingSyncObjectManager_Vulkan.h"
#include "SharedTypes.h"
#include "SafeQueue.h"
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

	class DrawingDevice_Vulkan;
	class DrawingCommandManager_Vulkan : public NoCopy
	{
	public:
		DrawingCommandManager_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue);
		~DrawingCommandManager_Vulkan();

		EQueueType GetWorkingQueueType() const;

		std::shared_ptr<DrawingCommandBuffer_Vulkan> RequestPrimaryCommandBuffer();
		void SubmitCommandBuffers(VkFence fence);
		void AddWaitSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void AddSignalSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);

	private:
		void CreateCommandPool();
		bool AllocatePrimaryCommandBuffer(uint32_t count);

	public:
		const uint32_t MAX_COMMAND_BUFFER_COUNT = 64;

	private:
		std::shared_ptr<DrawingDevice_Vulkan> m_pDevice;
		DrawingCommandQueue_Vulkan m_workingQueue;

		VkCommandPool m_commandPool;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_freeCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inUseCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inExecutionCommandBuffers;

		SafeQueue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_submitWaitSemaphores;
		SafeQueue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_submitSignalSemaphores;
	};
}