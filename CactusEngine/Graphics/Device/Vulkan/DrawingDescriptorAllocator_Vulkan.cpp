#include "DrawingDescriptorAllocator_Vulkan.h"
#include "DrawingDevice_Vulkan.h"

using namespace Engine;

DrawingDescriptorSet_Vulkan::DrawingDescriptorSet_Vulkan(VkDescriptorSet descSet)
	: m_descriptorSet(descSet)
{
	m_isInUse.AssignValue(false);
}

DrawingDescriptorSetLayout_Vulkan::DrawingDescriptorSetLayout_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
	: m_pDevice(pDevice)
{
	m_descriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = (uint32_t)bindings.size();
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->logicalDevice, &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: failed to create descriptor set layout.\n";
	}
}

DrawingDescriptorSetLayout_Vulkan::~DrawingDescriptorSetLayout_Vulkan()
{
	vkDestroyDescriptorSetLayout(m_pDevice->logicalDevice, m_descriptorSetLayout, nullptr);
}

const VkDescriptorSetLayout* DrawingDescriptorSetLayout_Vulkan::GetDescriptorSetLayout() const
{
	return &m_descriptorSetLayout;
}

DrawingDescriptorPool_Vulkan::DrawingDescriptorPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, uint32_t maxSets)
	: m_pDevice(pDevice), m_descriptorPool(VK_NULL_HANDLE), MAX_SETS(maxSets), m_allocatedSetsCount(0)
{

}

DrawingDescriptorPool_Vulkan::~DrawingDescriptorPool_Vulkan()
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_pDevice->logicalDevice, m_descriptorPool, nullptr);
	}
}

bool DrawingDescriptorPool_Vulkan::AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<std::shared_ptr<DrawingDescriptorSet_Vulkan>>& outSets, bool clearPrev)
{
	assert(m_descriptorPool != VK_NULL_HANDLE);
	assert(m_allocatedSetsCount + (uint32_t)layouts.size() <= MAX_SETS);

	if (clearPrev)
	{
		vkResetDescriptorPool(m_pDevice->logicalDevice, m_descriptorPool, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
		m_allocatedSetsCount = 0;
	}

	VkDescriptorSetAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = m_descriptorPool;
	allocateInfo.descriptorSetCount = (uint32_t)layouts.size();
	allocateInfo.pSetLayouts = layouts.data();

	m_allocatedSetsCount += allocateInfo.descriptorSetCount;

	std::vector<VkDescriptorSet> newSets(layouts.size());
	if (vkAllocateDescriptorSets(m_pDevice->logicalDevice, &allocateInfo, newSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: Failed to allocate descriptor set(s).");
		return false;
	}

	for (auto& set : newSets)
	{
		outSets.emplace_back(std::make_shared<DrawingDescriptorSet_Vulkan>(set));
	}

	return true;
}

void DrawingDescriptorPool_Vulkan::UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_Vulkan>& updateInfos)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	for (int i = 0; i < updateInfos.size(); ++i)
	{
		if (!updateInfos[i].hasContent)
		{
			continue;
		}

		VkWriteDescriptorSet descriptorWrite = {};

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = updateInfos[i].dstDescriptorSet;
		descriptorWrite.dstBinding = updateInfos[i].dstDescriptorBinding;
		descriptorWrite.dstArrayElement = updateInfos[i].dstArrayElement;
		descriptorWrite.descriptorType = updateInfos[i].dstDescriptorType;

		switch (updateInfos[i].infoType)
		{
		case EDescriptorResourceType_Vulkan::Buffer:
			descriptorWrite.descriptorCount = (uint32_t)updateInfos[i].bufferInfos.size();
			descriptorWrite.pBufferInfo = updateInfos[i].bufferInfos.data();
			break;

		case EDescriptorResourceType_Vulkan::Image:
			descriptorWrite.descriptorCount = (uint32_t)updateInfos[i].imageInfos.size();
			descriptorWrite.pImageInfo = updateInfos[i].imageInfos.data();
			break;

		case EDescriptorResourceType_Vulkan::TexelBuffer:
			descriptorWrite.descriptorCount = (uint32_t)updateInfos[i].imageInfos.size();
			descriptorWrite.pTexelBufferView = updateInfos[i].texelBufferViews.data();
			break;

		default:
			throw std::runtime_error("Vulkan: unhandled descriptor info type.");
			return;
		}

		descriptorWrites.emplace_back(descriptorWrite);
	}

	if (descriptorWrites.size() > 0)
	{
		vkUpdateDescriptorSets(m_pDevice->logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

DrawingDescriptorAllocator_Vulkan::DrawingDescriptorAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{

}

std::shared_ptr<DrawingDescriptorPool_Vulkan> DrawingDescriptorAllocator_Vulkan::CreateDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes)
{
	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = (uint32_t)poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Descriptor sets can return their individual allocations to the pool

	m_descriptorPools.emplace_back(std::make_shared<DrawingDescriptorPool_Vulkan>(m_pDevice, maxSets));

	if (vkCreateDescriptorPool(m_pDevice->logicalDevice, &createInfo, nullptr, &m_descriptorPools[m_descriptorPools.size() - 1]->m_descriptorPool) == VK_SUCCESS)
	{
		return m_descriptorPools[m_descriptorPools.size() - 1];
	}
	else
	{
		throw std::runtime_error("Vulkan: failed to create new descriptor pool.");
		return nullptr;
	}
}