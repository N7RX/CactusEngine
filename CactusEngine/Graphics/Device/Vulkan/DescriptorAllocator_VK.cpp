#include "DescriptorAllocator_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GHIUtilities_VK.h"
#include "LogUtility.h"

using namespace Engine;

DescriptorSet_VK::DescriptorSet_VK(VkDescriptorSet descSet)
	: m_descriptorSet(descSet)
{
	m_isInUse = false;
}

DescriptorSetLayout_VK::DescriptorSetLayout_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
	: m_pDevice(pDevice)
{
	m_descriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = (uint32_t)bindings.size();
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->logicalDevice, &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Vulkan: failed to create descriptor set layout.");
	}
}

DescriptorSetLayout_VK::~DescriptorSetLayout_VK()
{
	vkDestroyDescriptorSetLayout(m_pDevice->logicalDevice, m_descriptorSetLayout, nullptr);
}

const VkDescriptorSetLayout* DescriptorSetLayout_VK::GetDescriptorSetLayout() const
{
	return &m_descriptorSetLayout;
}

DescriptorPool_VK::DescriptorPool_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, uint32_t maxSets)
	: m_pDevice(pDevice), m_descriptorPool(VK_NULL_HANDLE), MAX_SETS(maxSets), m_allocatedSetsCount(0)
{

}

DescriptorPool_VK::~DescriptorPool_VK()
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_pDevice->logicalDevice, m_descriptorPool, nullptr);
	}
}

bool DescriptorPool_VK::AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<std::shared_ptr<DescriptorSet_VK>>& outSets, bool clearPrev)
{
	DEBUG_ASSERT_CE(m_descriptorPool != VK_NULL_HANDLE);
	DEBUG_ASSERT_CE(m_allocatedSetsCount + (uint32_t)layouts.size() <= MAX_SETS);

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
		outSets.emplace_back(std::make_shared<DescriptorSet_VK>(set));
	}

	return true;
}

void DescriptorPool_VK::UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_VK>& updateInfos)
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
		case EDescriptorResourceType_VK::Buffer:
			descriptorWrite.descriptorCount = (uint32_t)updateInfos[i].bufferInfos.size();
			descriptorWrite.pBufferInfo = updateInfos[i].bufferInfos.data();
			break;

		case EDescriptorResourceType_VK::Image:
			descriptorWrite.descriptorCount = (uint32_t)updateInfos[i].imageInfos.size();
			descriptorWrite.pImageInfo = updateInfos[i].imageInfos.data();
			break;

		case EDescriptorResourceType_VK::TexelBuffer:
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

DescriptorAllocator_VK::DescriptorAllocator_VK(const std::shared_ptr<LogicalDevice_VK> pDevice)
	: m_pDevice(pDevice)
{

}

std::shared_ptr<DescriptorPool_VK> DescriptorAllocator_VK::CreateDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes)
{
	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = (uint32_t)poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Descriptor sets can return their individual allocations to the pool

	m_descriptorPools.emplace_back(std::make_shared<DescriptorPool_VK>(m_pDevice, maxSets));

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