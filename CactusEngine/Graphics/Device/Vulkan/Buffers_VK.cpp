#include "Buffers_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GHIUtilities_VK.h"

using namespace Engine;

RawBuffer_VK::RawBuffer_VK(const std::shared_ptr<UploadAllocator_VK> pAllocator, const RawBufferCreateInfo_VK& createInfo)
	: m_pAllocator(pAllocator), m_deviceSize(createInfo.size)
{
	assert(pAllocator);

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

DataTransferBuffer_VK::DataTransferBuffer_VK(const std::shared_ptr<UploadAllocator_VK> pAllocator, const RawBufferCreateInfo_VK& createInfo)
	: m_pAllocator(pAllocator), m_constantlyMapped(false), m_ppMappedData(nullptr)
{
	m_sizeInBytes = createInfo.size;
	m_pBufferImpl = std::make_shared<RawBuffer_VK>(m_pAllocator, createInfo);
}

DataTransferBuffer_VK::~DataTransferBuffer_VK()
{
	if (m_constantlyMapped)
	{
		m_pAllocator->UnmapMemory(m_pBufferImpl->m_allocation);
	}
}

VertexBuffer_VK::VertexBuffer_VK(const std::shared_ptr<UploadAllocator_VK> pAllocator, const RawBufferCreateInfo_VK& vertexBufferCreateInfo, const RawBufferCreateInfo_VK& indexBufferCreateInfo)
	: m_pAllocator(pAllocator)
{
	m_pVertexBufferImpl = std::make_shared<RawBuffer_VK>(pAllocator, vertexBufferCreateInfo);

	m_pIndexBufferImpl = std::make_shared<RawBuffer_VK>(pAllocator, indexBufferCreateInfo);
	m_indexType = indexBufferCreateInfo.indexFormat;

	m_sizeInBytes = vertexBufferCreateInfo.size + indexBufferCreateInfo.size;
}

std::shared_ptr<RawBuffer_VK> VertexBuffer_VK::GetBufferImpl() const
{
	return m_pVertexBufferImpl;
}

std::shared_ptr<RawBuffer_VK> VertexBuffer_VK::GetIndexBufferImpl() const
{
	return m_pIndexBufferImpl;
}

VkIndexType VertexBuffer_VK::GetIndexFormat() const
{
	return m_indexType;
}

UniformBuffer_VK::UniformBuffer_VK(const std::shared_ptr<UploadAllocator_VK> pAllocator, const UniformBufferCreateInfo_VK& createInfo)
	: m_eType(createInfo.type), m_appliedShaderStage(createInfo.appliedStages), m_pHostData(nullptr), m_subAllocatedSize(0)
{
	RawBufferCreateInfo_VK bufferImplCreateInfo = {};
	bufferImplCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferImplCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	bufferImplCreateInfo.size = createInfo.size;

	m_pBufferImpl = std::make_shared<RawBuffer_VK>(pAllocator, bufferImplCreateInfo);
	m_sizeInBytes = createInfo.size;

	if (m_eType == EUniformBufferType_VK::Uniform)
	{
		if (!m_pBufferImpl->m_pAllocator->MapMemory(m_pBufferImpl->m_allocation, &m_pHostData))
		{
			std::cerr << "Vulkan: Failed to map uniform buffer memory.\n";
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

std::shared_ptr<SubUniformBuffer> UniformBuffer_VK::AllocateSubBuffer(uint32_t size)
{
	std::lock_guard<std::mutex> lock(m_subAllocateMutex);
	assert(m_subAllocatedSize + size <= m_sizeInBytes);

	auto pSubBuffer = std::make_shared<SubUniformBuffer_VK>(this, m_pBufferImpl->m_buffer, m_subAllocatedSize, size);
	m_subAllocatedSize += size;

	return pSubBuffer;
}

void UniformBuffer_VK::ResetSubBufferAllocation()
{
	m_subAllocatedSize = 0;
}

void UniformBuffer_VK::UpdateToDevice(std::shared_ptr<CommandBuffer_VK> pCmdBuffer)
{
	assert(m_pBufferImpl->m_pAllocator);
	assert(m_pRawData != nullptr);

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

std::shared_ptr<RawBuffer_VK> UniformBuffer_VK::GetBufferImpl() const
{
	return m_pBufferImpl;
}

EUniformBufferType_VK UniformBuffer_VK::GetType() const
{
	return m_eType;
}

SubUniformBuffer_VK::SubUniformBuffer_VK(UniformBuffer_VK* pParentBuffer, VkBuffer buffer, uint32_t offset, uint32_t size)
	: m_pParentBuffer(pParentBuffer), m_buffer(buffer), m_offset(offset), m_size(size)
{
	m_sizeInBytes = size;
}

void SubUniformBuffer_VK::UpdateSubBufferData(const void* pData)
{
	assert(m_pParentBuffer != nullptr);
	m_pParentBuffer->UpdateBufferSubData(pData, m_offset, m_size);
}