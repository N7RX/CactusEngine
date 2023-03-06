#include "Buffers_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GHIUtilities_VK.h"
#include "LogUtility.h"
#include "MemoryAllocator.h"

namespace Engine
{
	RawBuffer_VK::RawBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& createInfo)
		: m_pAllocator(pAllocator),
		m_deviceSize(createInfo.size)
	{
		DEBUG_ASSERT_CE(pAllocator);

		m_buffer = VK_NULL_HANDLE;
		m_allocation = VK_NULL_HANDLE;
		m_sizeInBytes = (uint32_t)createInfo.size;

		if (m_deviceSize > 0)
		{
			m_pAllocator->CreateBuffer(createInfo, *this);
		}
	}

	RawBuffer_VK::~RawBuffer_VK()
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			m_pAllocator->FreeBuffer(m_buffer, m_allocation);
		}
	}

	DataTransferBuffer_VK::DataTransferBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& createInfo)
		: m_pAllocator(pAllocator),
		m_constantlyMapped(false),
		m_ppMappedData(nullptr)
	{
		m_sizeInBytes = createInfo.size;
		CE_NEW(m_pBufferImpl, RawBuffer_VK, m_pAllocator, createInfo);
	}

	DataTransferBuffer_VK::~DataTransferBuffer_VK()
	{
		if (m_constantlyMapped)
		{
			m_pAllocator->UnmapMemory(m_pBufferImpl->m_allocation);
		}
	}

	VertexBuffer_VK::VertexBuffer_VK(UploadAllocator_VK* pAllocator, const RawBufferCreateInfo_VK& vertexBufferCreateInfo, const RawBufferCreateInfo_VK& indexBufferCreateInfo)
		: m_pAllocator(pAllocator)
	{
		CE_NEW(m_pVertexBufferImpl, RawBuffer_VK, pAllocator, vertexBufferCreateInfo);
		CE_NEW(m_pIndexBufferImpl, RawBuffer_VK, pAllocator, indexBufferCreateInfo);

		m_indexType = indexBufferCreateInfo.indexFormat;
		m_sizeInBytes = vertexBufferCreateInfo.size + indexBufferCreateInfo.size;
	}

	RawBuffer_VK* VertexBuffer_VK::GetBufferImpl() const
	{
		return m_pVertexBufferImpl;
	}

	RawBuffer_VK* VertexBuffer_VK::GetIndexBufferImpl() const
	{
		return m_pIndexBufferImpl;
	}

	VkIndexType VertexBuffer_VK::GetIndexFormat() const
	{
		return m_indexType;
	}

	UniformBuffer_VK::UniformBuffer_VK(UploadAllocator_VK* pAllocator, const UniformBufferCreateInfo_VK& createInfo)
		: m_eType(createInfo.type),
		m_appliedShaderStage(createInfo.appliedStages),
		m_pRawData(nullptr),
		m_pHostData(nullptr),
		m_subAllocatedSize(0)
	{
		RawBufferCreateInfo_VK bufferImplCreateInfo{};
		bufferImplCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferImplCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		bufferImplCreateInfo.size = createInfo.size;

		CE_NEW(m_pBufferImpl, RawBuffer_VK, pAllocator, bufferImplCreateInfo);
		m_sizeInBytes = createInfo.size;

		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			if (!m_pBufferImpl->m_pAllocator->MapMemory(m_pBufferImpl->m_allocation, &m_pHostData))
			{
				LOG_ERROR("Vulkan: Failed to map uniform buffer memory.");
			}
		}
	}

	UniformBuffer_VK::~UniformBuffer_VK()
	{
		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			m_pBufferImpl->m_pAllocator->UnmapMemory(m_pBufferImpl->m_allocation);
		}
	}

	void UniformBuffer_VK::UpdateBufferData(const void* pData)
	{
		m_pRawData = pData; // ERROR: for push constant, the pointer may go wild before it's updated to the device

		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			UpdateToDevice();
		}
	}

	void UniformBuffer_VK::UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size)
	{
		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			void* start = (unsigned char*)m_pHostData + offset;
			memcpy(start, pData, size);
		}
		else
		{
			throw std::runtime_error("Vulkan: Shouldn't call UpdateBufferSubData on push constant buffer.");
		}
	}

	SubUniformBuffer* UniformBuffer_VK::AllocateSubBuffer(uint32_t size)
	{
		std::lock_guard<std::mutex> lock(m_subAllocateMutex);
		DEBUG_ASSERT_CE(m_subAllocatedSize + size <= m_sizeInBytes);

		SubUniformBuffer* pSubBuffer;
		CE_NEW(pSubBuffer, SubUniformBuffer_VK, this, m_subAllocatedSize, size);
		m_subAllocatedSize += size;

		return pSubBuffer;
	}

	void UniformBuffer_VK::ResetSubBufferAllocation()
	{
		m_subAllocatedSize = 0;
	}

	void UniformBuffer_VK::UpdateToDevice(CommandBuffer_VK* pCmdBuffer)
	{
		DEBUG_ASSERT_CE(m_pBufferImpl->m_pAllocator);
		DEBUG_ASSERT_CE(m_pRawData != nullptr);

		switch (m_eType)
		{
		case EUniformBufferType_VK::PushConstant:
			assert(pCmdBuffer);
			pCmdBuffer->UpdatePushConstant(m_appliedShaderStage, m_sizeInBytes, m_pRawData);
			break;

		case EUniformBufferType_VK::Uniform:
			memcpy(m_pHostData, m_pRawData, m_sizeInBytes);
			break;

		default:
			throw std::runtime_error("Vulkan: Unhandled uniform buffer type.");
			return;
		}
	}

	RawBuffer_VK* UniformBuffer_VK::GetBufferImpl() const
	{
		return m_pBufferImpl;
	}

	EUniformBufferType_VK UniformBuffer_VK::GetType() const
	{
		return m_eType;
	}

	SubUniformBuffer_VK::SubUniformBuffer_VK(UniformBuffer_VK* pParentBuffer, uint32_t offset, uint32_t size)
		: m_pParentBuffer(pParentBuffer),
		m_offset(offset),
		m_size(size)
	{
		m_sizeInBytes = size;
	}

	void SubUniformBuffer_VK::UpdateSubBufferData(const void* pData)
	{
		DEBUG_ASSERT_CE(m_pParentBuffer != nullptr);
		m_pParentBuffer->UpdateBufferSubData(pData, m_offset, m_size);
	}
}