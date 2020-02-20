#include "DrawingDescriptorAllocator_Vulkan.h"
#include "DrawingDevice_Vulkan.h"

using namespace Engine;

DrawingDescriptorPool_Vulkan::DrawingDescriptorPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice), m_descriptorPool(VK_NULL_HANDLE)
{

}

DrawingDescriptorPool_Vulkan::~DrawingDescriptorPool_Vulkan()
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_pDevice->logicalDevice, m_descriptorPool, nullptr);
	}
}

bool DrawingDescriptorPool_Vulkan::AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<VkDescriptorSet>& outSets, bool clearPrev)
{
	assert(m_descriptorPool != VK_NULL_HANDLE);

	if (clearPrev)
	{
		vkResetDescriptorPool(m_pDevice->logicalDevice, m_descriptorPool, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	}

	VkDescriptorSetAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = m_descriptorPool;
	allocateInfo.descriptorSetCount = (uint32_t)layouts.size();
	allocateInfo.pSetLayouts = layouts.data();

	outSets.resize(layouts.size());
	return vkAllocateDescriptorSets(m_pDevice->logicalDevice, &allocateInfo, outSets.data()) == VK_SUCCESS;
}

bool DrawingDescriptorPool_Vulkan::UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_Vulkan>& updateInfos)
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
			return false;
		}

		descriptorWrites.emplace_back(descriptorWrite);
	}

	if (descriptorWrites.size() > 0)
	{
		vkUpdateDescriptorSets(m_pDevice->logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	return true;
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

	m_descriptorPools.emplace_back(std::make_shared<DrawingDescriptorPool_Vulkan>(m_pDevice));

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