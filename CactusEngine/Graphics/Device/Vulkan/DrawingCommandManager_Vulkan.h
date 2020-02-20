#pragma once
#include "DrawingSyncObjectManager_Vulkan.h"
#include "SharedTypes.h"
#include "SafeQueue.h"
#include "SafeBasicTypes.h"
#include "NoCopy.h"
#include "BasicMathTypes.h"

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

	class Texture2D_Vulkan;
	class RawBuffer_Vulkan;
	struct LogicalDevice_Vulkan;

	class DrawingCommandBuffer_Vulkan
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
		void BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet = 0);
		void DrawPrimitiveIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);
		void EndRenderPass();

		void TransitionImageLayout(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, EShaderType shaderType = EShaderType::Fragment);
		void GenerateMipmap(std::shared_ptr<Texture2D_Vulkan> pImage);
		void CopyBufferToBuffer(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, const std::shared_ptr<RawBuffer_Vulkan> pDstBuffer, const VkBufferCopy& region);
		void CopyBufferToTexture2D(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, std::shared_ptr<Texture2D_Vulkan> pDstImage, const std::vector<VkBufferImageCopy>& regions);

		void WaitSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);
		void SignalSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore);

	private:
		VkCommandBuffer m_commandBuffer;
		VkPipelineLayout m_pipelineLayout;

		std::shared_ptr<DrawingFence_Vulkan> m_pAssociatedFence;
		std::queue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_waitSemaphores;
		std::queue<std::shared_ptr<DrawingSemaphore_Vulkan>> m_signalSemaphores;

		bool m_isRecording;
		bool m_inRenderPass;
		bool m_inExecution;

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

	class CommandSubmitInfo_Vulkan
	{
	public:
		~CommandSubmitInfo_Vulkan()
		{
			Clear();
		}

		void Clear()
		{
			buffersAwaitSubmit.clear();		
			waitStages.clear();
			semaphoresToWait.clear();
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
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore>		  semaphoresToWait;
		std::vector<VkSemaphore>		  semaphoresToSignal;

		friend class DrawingCommandManager_Vulkan;
	};

	class DrawingCommandManager_Vulkan : public NoCopy
	{
	public:
		DrawingCommandManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue);
		~DrawingCommandManager_Vulkan();

		void Destroy();
		void WaitFrameFenceSubmission();

		EQueueType GetWorkingQueueType() const;

		std::shared_ptr<DrawingCommandBuffer_Vulkan> RequestPrimaryCommandBuffer();
		void SubmitCommandBuffers(VkFence fence);
		void SubmitSingleCommandBuffer_Immediate(const std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer); // This would stall the queue, use with caution

	private:
		void CreateCommandPool();
		bool AllocatePrimaryCommandBuffer(uint32_t count);

		void SubmitCommandBufferAsync();
		void RecycleCommandBufferAsync();

	public:
		const uint32_t MAX_COMMAND_BUFFER_COUNT = 64;
		const uint64_t RECYCLE_TIMEOUT = 3e9; // 3 seconds

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		DrawingCommandQueue_Vulkan m_workingQueue;
		bool m_isRunning;

		VkCommandPool m_commandPool;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_freeCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inUseCommandBuffers;
		SafeQueue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> m_inExecutionCommandBuffers;

		SafeQueue<std::shared_ptr<CommandSubmitInfo_Vulkan>> m_commandSubmissionQueue;

		// Async command submission
		std::thread m_commandBufferSubmissionThread;
		std::mutex m_frameFenceMutex;
		std::unique_lock<std::mutex> m_frameFenceLock;
		std::condition_variable m_frameFenceCv;
		bool m_waitFrameFenceSubmission;

		// Asyn command buffer recycle
		std::thread m_commandBufferRecycleThread;
		std::mutex m_commandBufferRecycleMutex;
		std::condition_variable m_commandBufferRecycleCv;
		SafeBool m_commandBufferRecycleFlag;		
	};
}