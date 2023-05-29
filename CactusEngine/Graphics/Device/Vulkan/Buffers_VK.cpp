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

	BaseUniformBuffer_VK::BaseUniformBuffer_VK(UploadAllocator_VK* pAllocator, const BaseUniformBufferCreateInfo_VK& createInfo)
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

	BaseUniformBuffer_VK::~BaseUniformBuffer_VK()
	{
		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			m_pBufferImpl->m_pAllocator->UnmapMemory(m_pBufferImpl->m_allocation);
			CE_DELETE(m_pBufferImpl);
		}
	}

	void BaseUniformBuffer_VK::UpdateBufferData(const void* pData)
	{
		m_pRawData = pData; // WARNING: for push constant, the pointer may go wild before it's updated to the device

		if (m_eType == EUniformBufferType_VK::Uniform)
		{
			UpdateToDevice();
		}
		else
		{
			LOG_ERROR("Vulkan: Shouldn't call UpdateBufferData on push constant buffer.");
		}
	}

	void BaseUniformBuffer_VK::UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size)
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

	UniformBuffer BaseUniformBuffer_VK::AllocateSubBuffer(uint32_t size)
	{
		DEBUG_ASSERT_CE(m_subAllocatedSize + size <= m_sizeInBytes);

		UniformBuffer subBuffer(this, m_subAllocatedSize, size);
		m_subAllocatedSize += size;

		return subBuffer;
	}

	UniformBuffer BaseUniformBuffer_VK::AllocateSubBuffer(uint32_t offset, uint32_t size)
	{
		DEBUG_ASSERT_CE(offset + size <= m_sizeInBytes);

		UniformBuffer subBuffer(this, offset, size);
		// This call comes from reserved region, so m_subAllocatedSize is already updated

		return subBuffer;
	}

	uint32_t BaseUniformBuffer_VK::ReserveBufferRegion(uint32_t size)
	{
		DEBUG_ASSERT_CE(m_subAllocatedSize + size <= m_sizeInBytes);

		uint32_t offset = m_subAllocatedSize;
		m_subAllocatedSize += size;

		return offset;
	}

	void BaseUniformBuffer_VK::ResetSubBufferAllocation()
	{
		m_subAllocatedSize = 0;
	}

	void BaseUniformBuffer_VK::UpdateToDevice(CommandBuffer_VK* pCmdBuffer)
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

	RawBuffer_VK* BaseUniformBuffer_VK::GetBufferImpl() const
	{
		return m_pBufferImpl;
	}

	EUniformBufferType_VK BaseUniformBuffer_VK::GetType() const
	{
		return m_eType;
	}

	uint32_t BaseUniformBuffer_VK::GetFreeSpace() const
	{
		return m_sizeInBytes - m_subAllocatedSize;
	}

	UniformBufferManager_VK::UniformBufferManager_VK(UploadAllocator_VK* pAllocator)
		: m_pAllocator(pAllocator)
	{
		// Create one buffer in each pool by default

		BaseUniformBufferCreateInfo_VK createInfo{};
		createInfo.size = DEFAULT_BUFFER_SIZE;
		createInfo.type = EUniformBufferType_VK::Uniform;
		createInfo.appliedStages = VK_SHADER_STAGE_ALL_GRAPHICS;

		uint32_t maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		m_baseUniformBuffers.resize(maxFramesInFlight);

		for (uint32_t i = 0; i < maxFramesInFlight; i++)
		{
			BaseUniformBuffer_VK* pBuffer = nullptr;
			CE_NEW(pBuffer, BaseUniformBuffer_VK, m_pAllocator, createInfo);
			m_baseUniformBuffers[i].push_back(pBuffer);
		}
	}

	UniformBufferManager_VK::~UniformBufferManager_VK()
	{
		for (auto& bufferPool : m_baseUniformBuffers)
		{
			for (auto& pBuffer : bufferPool)
			{
				CE_DELETE(pBuffer);
			}
		}
	}

	UniformBuffer UniformBufferManager_VK::GetUniformBuffer(uint32_t size)
	{
		UniformBuffer buffer{};
		auto& bufferPool = m_baseUniformBuffers[m_currentFrameIndex];

		std::lock_guard<std::mutex> lock(m_bufferPoolMutex);

		for (auto& pBuffer : bufferPool)
		{
			if (pBuffer->GetFreeSpace() >= size)
			{
				buffer = pBuffer->AllocateSubBuffer(size);
				break;
			}
		}

		if (!buffer.IsValid())
		{
			BaseUniformBufferCreateInfo_VK createInfo{};
			createInfo.size = DEFAULT_BUFFER_SIZE;
			createInfo.type = EUniformBufferType_VK::Uniform;
			createInfo.appliedStages = VK_SHADER_STAGE_ALL_GRAPHICS;

			BaseUniformBuffer_VK* pBuffer = nullptr;
			CE_NEW(pBuffer, BaseUniformBuffer_VK, m_pAllocator, createInfo);
			bufferPool.push_back(pBuffer);

			buffer = pBuffer->AllocateSubBuffer(size);
		}

		return buffer;
	}

	UniformBuffer UniformBufferManager_VK::GetUniformBuffer(UniformBufferReservedRegion& region, uint32_t size)
	{
		DEBUG_ASSERT_MESSAGE_CE(size <= region.availableSize, "Reserved buffer region does not have enough space.");
		auto& bufferPool = m_baseUniformBuffers[m_currentFrameIndex];

		uint32_t internalOffset = region.totalSize - region.availableSize;
		auto buffer = bufferPool[region.index]->AllocateSubBuffer(region.offset + internalOffset, size);
		region.availableSize -= size;

		return buffer;
	}

	UniformBufferReservedRegion UniformBufferManager_VK::ReserveBufferRegion(uint32_t size)
	{
		DEBUG_ASSERT_MESSAGE_CE(size <= DEFAULT_BUFFER_SIZE, "Invalid buffer region reservation size.");
		auto& bufferPool = m_baseUniformBuffers[m_currentFrameIndex];

		UniformBufferReservedRegion region{};
		region.totalSize = size;
		region.availableSize = size;

		std::lock_guard<std::mutex> lock(m_bufferPoolMutex);

		for (uint32_t i = 0; i < bufferPool.size(); ++i)
		{
			if (bufferPool[i]->GetFreeSpace() >= size)
			{
				region.index = i;
				region.offset = bufferPool[i]->ReserveBufferRegion(size);
				return region;
			}
		}

		// No buffer has enough space, create a new one

		BaseUniformBufferCreateInfo_VK createInfo{};
		createInfo.size = DEFAULT_BUFFER_SIZE;
		createInfo.type = EUniformBufferType_VK::Uniform;
		createInfo.appliedStages = VK_SHADER_STAGE_ALL_GRAPHICS;

		BaseUniformBuffer_VK* pBuffer = nullptr;
		CE_NEW(pBuffer, BaseUniformBuffer_VK, m_pAllocator, createInfo);
		bufferPool.push_back(pBuffer);

		region.index = bufferPool.size() - 1;
		region.offset = pBuffer->ReserveBufferRegion(size);

		return region;
	}

	void UniformBufferManager_VK::ResetBufferAllocation()
	{
		auto& bufferPool = m_baseUniformBuffers[m_currentFrameIndex];
		for (auto& pBuffer : bufferPool)
		{
			pBuffer->ResetSubBufferAllocation();
		}
	}
}