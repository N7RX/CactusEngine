#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"

#include <mutex>

namespace Engine
{
	class CommandBuffer_VK;

	class RawBuffer_VK : public RawResource
	{
	public:
		RawBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& createInfo);
		virtual ~RawBuffer_VK();

	protected:
		UploadAllocator_VK* m_pAllocator;

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
		DataTransferBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& createInfo);
		~DataTransferBuffer_VK();

	private:
		UploadAllocator_VK* m_pAllocator;
		RawBuffer_VK* m_pBufferImpl;

		bool m_constantlyMapped;
		void* m_ppMappedData;

		friend class GraphicsHardwareInterface_VK;
	};

	class VertexBuffer_VK : public VertexBuffer
	{
	public:
		VertexBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& vertexBufferCreateInfo, const RawBufferCreateInfo_VK& indexBufferCreateInfo);
		~VertexBuffer_VK() = default;

		RawBuffer_VK* GetBufferImpl() const;
		RawBuffer_VK* GetIndexBufferImpl() const;
		VkIndexType GetIndexFormat() const;

	private:
		UploadAllocator_VK* m_pAllocator;
		RawBuffer_VK* m_pVertexBufferImpl;
		RawBuffer_VK* m_pIndexBufferImpl;
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
		uint32_t				size;
		VkShaderStageFlags		appliedStages;
	};

	class UniformBuffer_VK : public UniformBuffer
	{
	public:
		UniformBuffer_VK(UploadAllocator_VK* pAllocator, const UniformBufferCreateInfo_VK& createInfo);
		~UniformBuffer_VK();

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;
		SubUniformBuffer* AllocateSubBuffer(uint32_t size) override; // Remember to CE_DELETE the returned sub-buffer
		void ResetSubBufferAllocation() override;

		void UpdateToDevice(CommandBuffer_VK* pCmdBuffer = nullptr);
		RawBuffer_VK* GetBufferImpl() const;

		EUniformBufferType_VK GetType() const;

	private:
		RawBuffer_VK* m_pBufferImpl;
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
		SubUniformBuffer_VK(UniformBuffer_VK* pParentBuffer, uint32_t offset, uint32_t size);
		~SubUniformBuffer_VK() = default;

		void UpdateSubBufferData(const void* pData) override;

	private:
		UniformBuffer_VK* m_pParentBuffer;
		uint32_t m_offset;
		uint32_t m_size;

		friend class GraphicsHardwareInterface_VK;
		friend class UniformBuffer_VK;
	};
}