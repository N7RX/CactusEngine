#pragma once
#include "DrawingSyncObjectManager_Vulkan.h"
#include "SharedTypes.h"
#include "SafeQueue.h"
#include "SafeBasicTypes.h"
#include "NoCopy.h"
#include "BasicMathTypes.h"
#include "DrawingDescriptorAllocator_Vulkan.h"
#include "CommandResources.h"

#include <vulkan.h>
#include <memory>
#include <vector>

namespace Engine
{
	struct DrawingCommandQueue_Vulkan
	{		
		EQueueType	type;
		uint32_t	queueFamilyIndex;
		VkQueue		queue;
	};

	enum EDrawingCommandBufferUsageFlagBits_Vulkan
	{
		Explicit = 0x1,
		Implicit = 0x2,
		COUNT = 2
	};

	struct LogicalDevice_Vulkan;
	class  Texture2D_Vulkan;
	class  RawBuffer_Vulkan;	
	class  DrawingCommandPool_Vulkan;

	class DrawingCommandBuffer_Vulkan : public DrawingCommandBuffer
	{
	public:
		DrawingCommandBuffer_Vulkan(const VkCommandBuffer& cmdBuffer);
		~DrawingCommandBuffer_Vulkan();

		bool IsRecording() const;
		bool InRenderPass() const;
		bool InExecution() const;

		void BeginCommandBuffer(VkCommandBufferUsageFlags usage);

		void BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pVertexBuffers, const VkDeviceSize* pOffsets);
		void BindIndexBuffer(const VkBuffer indexBuffer, const VkDeviceSize offset, VkIndexType type);
		void BeginRenderPass(const VkRenderPass renderPass, const VkFramebuffer frameBuffer, const std::vector<VkClearValue>& clearValues, const VkExtent2D& areaExtent, const VkOffset2D& areaOffset = { 0, 0 });
		void BindPipeline(const VkPipelineBindPoint bindPoint, const VkPipeline pipeline);
		void BindPipelineLayout(const VkPipelineLayout pipelineLayout); // TODO: integrate this function with BindPipeline
		void UpdatePushConstant(const VkShaderStageFlags shaderStage, uint32_t size, const void* pData, uint32_t offset = 0);
		void BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<std::shared_ptr<DrawingDescriptorSet_Vulkan>>& descriptorSets, uint32_t firstSet = 0);
		void DrawPrimitiveIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
		void DrawPrimitive(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
		void EndRenderPass();
		void EndCommandBuffer();

		void TransitionImageLayout(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, uint32_t appliedStages);
		void GenerateMipmap(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, uint32_t appliedStages);
		void CopyBufferToBuffer(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, const std::shared_ptr<RawBuffer_Vulkan> pDstBuffer, const VkBufferCopy& region);
		void CopyBufferToTexture2D(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, std::shared_ptr<Texture2D_Vulkan> pDstImage, const std::vector<VkBufferImageCopy>& regions);
		void CopyTexture2DToBuffer(std::shared_ptr<Texture2D_Vulkan> pSrcImage, const std::shared_ptr<RawBuffer_Vulkan> pDstBuffer, const std::vector<VkBufferImageCopy>& regions);

		void WaitSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void SignalSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);

	private:
		VkCommandBuffer m_commandBuffer;
		uint32_t m_usageFlags; // Bitmap
		DrawingCommandPool_Vulkan* m_pAllocatedPool;

		VkPipelineLayout m_pipelineLayout;

		std::shared_ptr<DrawingFence_Vulkan> m_pAssociatedFence;
		std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> m_waitSemaphores;
		std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> m_signalSemaphores;
		std::shared_ptr<DrawingSyncObjectManager_Vulkan> m_pSyncObjectManager;

		std::queue<std::shared_ptr<DrawingDescriptorSet_Vulkan>> m_boundDescriptorSets;

		bool m_isRecording;
		bool m_inRenderPass;
		bool m_inExecution;
		bool m_isExternal;

		friend class DrawingCommandPool_Vulkan;
		friend class DrawingCommandManager_Vulkan;
		friend class DrawingDevice_Vulkan;
	};

	class DrawingCommandPool_Vulkan : public NoCopy, public DrawingCommandPool, std::enable_shared_from_this<DrawingCommandPool_Vulkan>
	{
	public:
		DrawingCommandPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkCommandPool poolHandle);
		~DrawingCommandPool_Vulkan();

		std::shared_ptr<DrawingCommandBuffer_Vulkan> RequestPrimaryCommandBuffer();

	private:
		bool AllocatePrimaryCommandBuffer(uint32_t count);

	public:
		const uint32_t MAX_COMMAND_BUFFER_COUNT = 64;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkCommandPool m_commandPool;
		uint32_t m_allocatedCommandBufferCount;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_freeCommandBuffers;

		friend class DrawingCommandManager_Vulkan;
		friend class DrawingDevice_Vulkan;
	};

	struct CommandSubmitInfo_Vulkan
	{
		VkSubmitInfo submitInfo;
		VkFence		 fence;
		//std::shared_ptr<QueueSubmitConditionLock_Vulkan> submitConditionLock;
		std::queue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> queuedCmdBuffers;

		// Retained resources
		std::vector<VkCommandBuffer>	  buffersAwaitSubmit;	
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore>		  semaphoresToWait;
		std::vector<VkSemaphore>		  semaphoresToSignal;
	};

	class DrawingCommandManager_Vulkan : public NoCopy
	{
	public:
		DrawingCommandManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue);
		~DrawingCommandManager_Vulkan();

		void Destroy();

		EQueueType GetWorkingQueueType() const;

		std::shared_ptr<DrawingCommandBuffer_Vulkan> RequestPrimaryCommandBuffer();
		void SubmitCommandBuffers(std::shared_ptr<DrawingFence_Vulkan> pFence, uint32_t usageMask);
		void SubmitSingleCommandBuffer_Immediate(const std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer); // This function would stall the queue, use with caution
																												 // Also, it ONLY accepts command buffers allocated from default pool
		// For multithreading																					 // Also, DO NOT use this function for frequently called operations (e.g. per frame)
		std::shared_ptr<DrawingCommandPool_Vulkan> RequestExternalCommandPool();
		void ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer);

	private:
		VkCommandPool CreateCommandPool();

		void SubmitCommandBufferAsync();
		void RecycleCommandBufferAsync();

	public:
		const uint64_t RECYCLE_TIMEOUT = 3e9; // 3 seconds

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		DrawingCommandQueue_Vulkan m_workingQueue;
		bool m_isRunning;

		std::shared_ptr<DrawingCommandPool_Vulkan> m_pDefaultCommandPool;

		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inUseCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inExecutionCommandBuffers;

		SafeQueue<std::shared_ptr<CommandSubmitInfo_Vulkan>> m_commandSubmissionQueue;

		std::mutex m_externalCommandPoolCreationMutex;
		std::mutex m_inExecutionQueueRWMutex;

		// Async command submission
		std::thread m_commandBufferSubmissionThread;
		std::mutex m_commandBufferSubmissionMutex;
		std::condition_variable m_commandBufferSubmissionCv;
		SafeBool m_commandBufferSubmissionFlag;

		// Asyn command buffer recycle
		std::thread m_commandBufferRecycleThread;
		std::mutex m_commandBufferRecycleMutex;
		std::condition_variable m_commandBufferRecycleCv;
		SafeBool m_commandBufferRecycleFlag;
	};
}