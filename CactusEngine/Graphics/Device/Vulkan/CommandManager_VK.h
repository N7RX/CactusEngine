#pragma once
#include "SyncObjectManager_VK.h"
#include "SharedTypes.h"
#include "SafeQueue.h"
#include "SafeBasicTypes.h"
#include "NoCopy.h"
#include "BasicMathTypes.h"
#include "DescriptorAllocator_VK.h"
#include "CommandResources.h"

#include <vulkan.h>
#include <memory>
#include <vector>

namespace Engine
{
	struct CommandQueue_VK
	{		
		EQueueType	type;
		uint32_t	queueFamilyIndex;
		VkQueue		queue;

		bool		isValid = true;
	};

	enum class ECommandBufferUsageFlagBits_VK
	{
		Explicit = 0x1,
		Implicit = 0x2,
		COUNT = 2
	};

	struct LogicalDevice_VK;
	class  Texture2D_VK;
	class  RawBuffer_VK;	
	class  CommandPool_VK;
	class  CommandManager_VK;

	class CommandBuffer_VK : public GraphicsCommandBuffer
	{
	public:
		CommandBuffer_VK(const VkCommandBuffer& cmdBuffer);
		~CommandBuffer_VK();

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
		void BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<std::shared_ptr<DescriptorSet_VK>>& descriptorSets, uint32_t firstSet = 0);
		void DrawPrimitiveIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
		void DrawPrimitive(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
		void EndRenderPass();
		void EndCommandBuffer();

		void TransitionImageLayout(std::shared_ptr<Texture2D_VK> pImage, const VkImageLayout newLayout, uint32_t appliedStages);
		void GenerateMipmap(std::shared_ptr<Texture2D_VK> pImage, const VkImageLayout newLayout, uint32_t appliedStages);
		void CopyBufferToBuffer(const std::shared_ptr<RawBuffer_VK> pSrcBuffer, const std::shared_ptr<RawBuffer_VK> pDstBuffer, const VkBufferCopy& region);
		void CopyBufferToTexture2D(const std::shared_ptr<RawBuffer_VK> pSrcBuffer, std::shared_ptr<Texture2D_VK> pDstImage, const std::vector<VkBufferImageCopy>& regions);
		void CopyTexture2DToBuffer(std::shared_ptr<Texture2D_VK> pSrcImage, const std::shared_ptr<RawBuffer_VK> pDstBuffer, const std::vector<VkBufferImageCopy>& regions);

		// For presentation only
		void WaitPresentationSemaphore(const std::shared_ptr<Semaphore_VK> pSemaphore);
		void SignalPresentationSemaphore(const std::shared_ptr<Semaphore_VK> pSemaphore);

		// For general purpose
		void WaitSemaphore(const std::shared_ptr<TimelineSemaphore_VK> pSemaphore);
		void SignalSemaphore(const std::shared_ptr<TimelineSemaphore_VK> pSemaphore);

	private:
		VkCommandBuffer m_commandBuffer;
		CommandPool_VK* m_pAllocatedPool;
		uint32_t m_usageFlags; // Bitmap

		VkPipelineLayout m_pipelineLayout;

		std::shared_ptr<TimelineSemaphore_VK> m_pAssociatedSubmitSemaphore;
		std::vector<std::shared_ptr<Semaphore_VK>> m_waitPresentationSemaphores;
		std::vector<std::shared_ptr<Semaphore_VK>> m_signalPresentationSemaphores;
		std::vector<std::shared_ptr<TimelineSemaphore_VK>> m_waitSemaphores;
		std::vector<std::shared_ptr<TimelineSemaphore_VK>> m_signalSemaphores;
		std::shared_ptr<SyncObjectManager_VK> m_pSyncObjectManager;

		std::queue<std::shared_ptr<DescriptorSet_VK>> m_boundDescriptorSets;

		bool m_isRecording;
		bool m_inRenderPass;
		bool m_inExecution;
		bool m_isExternal;

		friend class CommandPool_VK;
		friend class CommandManager_VK;
		friend class GraphicsHardwareInterface_VK;
	};

	class CommandPool_VK : public NoCopy, public GraphicsCommandPool, std::enable_shared_from_this<CommandPool_VK>
	{
	public:
		CommandPool_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, VkCommandPool poolHandle, CommandManager_VK* pManager);
		~CommandPool_VK();

		std::shared_ptr<CommandBuffer_VK> RequestPrimaryCommandBuffer();

	private:
		bool AllocatePrimaryCommandBuffer(uint32_t count);

	public:
		const uint32_t MAX_COMMAND_BUFFER_COUNT = 64;

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VkCommandPool m_commandPool;
		CommandManager_VK* m_pManager;
		uint32_t m_allocatedCommandBufferCount;
		SafeQueue<std::shared_ptr<CommandBuffer_VK>> m_freeCommandBuffers;

		friend class CommandManager_VK;
		friend class GraphicsHardwareInterface_VK;
	};

	struct CommandSubmitInfo_VK
	{
		VkSubmitInfo submitInfo;

		//std::shared_ptr<QueueSubmitConditionLock_Vulkan> submitConditionLock;
		std::queue<std::shared_ptr<CommandBuffer_VK>> queuedCmdBuffers;

		// Retained resources
		std::vector<VkCommandBuffer>	  buffersAwaitSubmit;	
		std::vector<uint64_t>			  waitSemaphoreValues;
		std::vector<uint64_t>			  signalSemaphoreValues;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore>		  semaphoresToWait;
		std::vector<VkSemaphore>		  semaphoresToSignal;
		VkTimelineSemaphoreSubmitInfo	  timelineSemaphoreSubmitInfo;
	};

	class CommandManager_VK : public NoCopy
	{
	public:
		CommandManager_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const CommandQueue_VK& queue);
		~CommandManager_VK();

		void Destroy();

		EQueueType GetWorkingQueueType() const;

		std::shared_ptr<CommandBuffer_VK> RequestPrimaryCommandBuffer();
		void SubmitCommandBuffers(std::shared_ptr<TimelineSemaphore_VK> pSubmitSemaphore, uint32_t usageMask);
		void SubmitSingleCommandBuffer_Immediate(const std::shared_ptr<CommandBuffer_VK> pCmdBuffer); // This function would stall the queue, use with caution
																												 // Also, it ONLY accepts command buffers allocated from default pool
		// For multithreading																					 // Also, DO NOT use this function for frequently called operations (e.g. per frame)
		std::shared_ptr<CommandPool_VK> RequestExternalCommandPool();
		void ReturnExternalCommandBuffer(std::shared_ptr<CommandBuffer_VK> pCmdBuffer);

	private:
		VkCommandPool CreateCommandPool();

		void SubmitCommandBufferAsync();
		void RecycleCommandBufferAsync();

	public:
		const uint64_t RECYCLE_TIMEOUT = 3e9; // 3 seconds

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		CommandQueue_VK m_workingQueue;
		bool m_isRunning;

		std::shared_ptr<CommandPool_VK> m_pDefaultCommandPool;

		SafeQueue<std::shared_ptr<CommandBuffer_VK>> m_inUseCommandBuffers;
		SafeQueue<std::shared_ptr<CommandBuffer_VK>> m_inExecutionCommandBuffers;

		SafeQueue<std::shared_ptr<CommandSubmitInfo_VK>> m_commandSubmissionQueue;

		std::mutex m_externalCommandPoolCreationMutex;
		std::mutex m_inExecutionQueueRWMutex;

		// Async command submission
		std::thread m_commandBufferSubmissionThread;
		std::mutex m_commandBufferSubmissionMutex;
		std::condition_variable m_commandBufferSubmissionCv;
		std::atomic<bool> m_commandBufferSubmissionFlag;

		// Asyn command buffer recycle
		std::thread m_commandBufferRecycleThread;
		std::mutex m_commandBufferRecycleMutex;
		std::condition_variable m_commandBufferRecycleCv;
		std::atomic<bool> m_commandBufferRecycleFlag;
	};
}