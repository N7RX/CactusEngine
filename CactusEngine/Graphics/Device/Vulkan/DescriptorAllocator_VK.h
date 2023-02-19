#pragma once
#include "SafeBasicTypes.h"
#include <vulkan.h>
#include <vector>
#include <memory>

namespace Engine
{
	enum class EDescriptorResourceType_VK
	{
		Buffer = 0,
		Image,
		TexelBuffer,
		COUNT
	};

	struct DesciptorUpdateInfo_VK
	{
		EDescriptorResourceType_VK		infoType;
		bool								hasContent;
		VkDescriptorSet						dstDescriptorSet;
		uint32_t							dstDescriptorBinding;
		uint32_t							dstArrayElement;
		VkDescriptorType					dstDescriptorType;	
		std::vector<VkDescriptorBufferInfo>	bufferInfos;
		std::vector<VkDescriptorImageInfo>	imageInfos;
		std::vector<VkBufferView>			texelBufferViews;	
	};

	struct DescriptorCopyInfo_VK
	{
		VkDescriptorSet	dstDescriptorSet;
		uint32_t		dstDescriptorBinding;
		uint32_t		dstArrayElement;
		VkDescriptorSet	srcDescriptorSet;
		uint32_t		srcDescriptorBinding;
		uint32_t		srcArrayElement;
		uint32_t		descriptorCount;
	};

	struct LogicalDevice_VK;

	class DescriptorSet_VK
	{
	public:
		DescriptorSet_VK(VkDescriptorSet descSet);
		~DescriptorSet_VK() = default;

	private:
		VkDescriptorSet m_descriptorSet;
		std::atomic<bool> m_isInUse;
		
		friend class DescriptorPool_VK;
		friend class DescriptorAllocator_VK;
		friend class CommandManager_VK;
		friend class CommandBuffer_VK;
		friend class ShaderProgram_VK;
		friend class GraphicsHardwareInterface_VK;
	};

	class DescriptorSetLayout_VK
	{
	public:
		DescriptorSetLayout_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
		~DescriptorSetLayout_VK();

		const VkDescriptorSetLayout* GetDescriptorSetLayout() const;

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};

	class DescriptorPool_VK
	{
	public:
		DescriptorPool_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, uint32_t maxSets);
		~DescriptorPool_VK();

		bool AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<std::shared_ptr<DescriptorSet_VK>>& outSets, bool clearPrev = false);
		void UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_VK>& updateInfos);
		// TODO: add set copy support

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VkDescriptorPool m_descriptorPool;

		const uint32_t MAX_SETS;
		uint32_t m_allocatedSetsCount;

		friend class DescriptorAllocator_VK;
	};
	
	class DescriptorAllocator_VK
	{
	public:
		DescriptorAllocator_VK(const std::shared_ptr<LogicalDevice_VK> pDevice);
		~DescriptorAllocator_VK() = default;

		std::shared_ptr<DescriptorPool_VK> CreateDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
		// TODO: add pool deletion support

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		std::vector<std::shared_ptr<DescriptorPool_VK>> m_descriptorPools;
	};
}