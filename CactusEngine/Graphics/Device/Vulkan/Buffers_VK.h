#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"

#include <mutex>

namespace Engine
{
	struct LogicalDevice_VK;
	class  CommandBuffer_VK;

	class RawBuffer_VK : public RawResource
	{
	public:
		RawBuffer_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const RawBufferCreateInfo_VK& createInfo);
		virtual ~RawBuffer_VK();

	protected:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;

		VkBuffer		m_buffer;
		VmaAllocation	m_allocation;
		VkDeviceSize	m_deviceSize;

		friend class CommandBuffer_VK;
		friend class UploadAllocator_VK;
		friend class GraphicsHardwareInterface_VK;
		friend class UniformBuffer_VK;
		friend class DataTransferBuffer_VK;
	};

	class DataTransferBuffer_VK : public DataTransferBuffer
	{
	public:
		DataTransferBuffer_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const RawBufferCreateInfo_VK& createInfo);
		~DataTransferBuffer_VK();

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		std::shared_ptr<RawBuffer_VK> m_pBufferImpl;

		bool m_constantlyMapped;
		void* m_ppMappedData;

		friend class GraphicsHardwareInterface_VK;
	};

	class VertexBuffer_VK : public VertexBuffer
	{
	public:
		VertexBuffer_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const RawBufferCreateInfo_VK& vertexBufferCreateInfo, const RawBufferCreateInfo_VK& indexBufferCreateInfo);
		~VertexBuffer_VK() = default;

		std::shared_ptr<RawBuffer_VK> GetBufferImpl() const;
		std::shared_ptr<RawBuffer_VK> GetIndexBufferImpl() const;
		VkIndexType GetIndexFormat() const;

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		std::shared_ptr<RawBuffer_VK> m_pVertexBufferImpl;
		std::shared_ptr<RawBuffer_VK> m_pIndexBufferImpl;
		VkIndexType m_indexType;
	};

	enum class EUniformBufferType_VK
	{
		Undefined = 0,
		Uniform,
		PushConstant,
		COUNT
	};

	struct UniformBufferCreateInfo_VK
	{
		EUniformBufferType_VK	type;
		uint32_t					size;
		VkShaderStageFlags			appliedStages;
	};

	class UniformBuffer_VK : public UniformBuffer
	{
	public:
		UniformBuffer_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const UniformBufferCreateInfo_VK& createInfo);
		~UniformBuffer_VK();

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;
		std::shared_ptr<SubUniformBuffer> AllocateSubBuffer(uint32_t size) override;
		void ResetSubBufferAllocation() override;

		void UpdateToDevice(std::shared_ptr<CommandBuffer_VK> pCmdBuffer = nullptr);
		std::shared_ptr<RawBuffer_VK> GetBufferImpl() const;

		EUniformBufferType_VK GetType() const;

	private:
		std::shared_ptr<RawBuffer_VK> m_pBufferImpl;
		EUniformBufferType_VK m_eType;
		VkShaderStageFlags m_appliedShaderStage;

		const void* m_pRawData;
		void* m_pHostData; // Pointer to mapped host memory location

		uint32_t m_subAllocatedSize;
		mutable std::mutex m_subAllocateMutex;

		friend class SubUniformBuffer_VK;
	};

	class SubUniformBuffer_VK : public SubUniformBuffer
	{
	public:
		SubUniformBuffer_VK(UniformBuffer_VK* pParentBuffer, VkBuffer buffer, uint32_t offset, uint32_t size);
		~SubUniformBuffer_VK() = default;

		void UpdateSubBufferData(const void* pData) override;

	private:
		UniformBuffer_VK* m_pParentBuffer;
		VkBuffer m_buffer;
		uint32_t m_offset;
		uint32_t m_size;

		friend class GraphicsHardwareInterface_VK;
		friend class UniformBuffer_VK;
	};
}