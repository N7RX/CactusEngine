#pragma once
#include "SafeBasicTypes.h"
#include <vulkan.h>
#include <vector>
#include <memory>

namespace Engine
{
	enum class EDescriptorResourceType_Vulkan
	{
		Buffer = 0,
		Image,
		TexelBuffer,
		COUNT
	};

	struct DesciptorUpdateInfo_Vulkan
	{
		EDescriptorResourceType_Vulkan		infoType;
		bool								hasContent;
		VkDescriptorSet						dstDescriptorSet;
		uint32_t							dstDescriptorBinding;
		uint32_t							dstArrayElement;
		VkDescriptorType					dstDescriptorType;	
		std::vector<VkDescriptorBufferInfo>	bufferInfos;
		std::vector<VkDescriptorImageInfo>	imageInfos;
		std::vector<VkBufferView>			texelBufferViews;	
	};

	struct DescriptorCopyInfo_Vulkan
	{
		VkDescriptorSet	dstDescriptorSet;
		uint32_t		dstDescriptorBinding;
		uint32_t		dstArrayElement;
		VkDescriptorSet	srcDescriptorSet;
		uint32_t		srcDescriptorBinding;
		uint32_t		srcArrayElement;
		uint32_t		descriptorCount;
	};

	struct LogicalDevice_Vulkan;

	class DrawingDescriptorSet_Vulkan
	{
	public:
		DrawingDescriptorSet_Vulkan(VkDescriptorSet descSet);
		~DrawingDescriptorSet_Vulkan() = default;

	private:
		VkDescriptorSet m_descriptorSet;
		SafeBool m_isInUse;
		
		friend class DrawingDescriptorPool_Vulkan;
		friend class DrawingDescriptorAllocator_Vulkan;
		friend class DrawingCommandManager_Vulkan;
		friend class DrawingCommandBuffer_Vulkan;
		friend class ShaderProgram_Vulkan;
		friend class DrawingDevice_Vulkan;
	};

	class DrawingDescriptorSetLayout_Vulkan
	{
	public:
		DrawingDescriptorSetLayout_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
		~DrawingDescriptorSetLayout_Vulkan();

		const VkDescriptorSetLayout* GetDescriptorSetLayout() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};

	class DrawingDescriptorPool_Vulkan
	{
	public:
		DrawingDescriptorPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, uint32_t maxSets);
		~DrawingDescriptorPool_Vulkan();

		bool AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<std::shared_ptr<DrawingDescriptorSet_Vulkan>>& outSets, bool clearPrev = false);
		void UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_Vulkan>& updateInfos);
		// TODO: add set copy support

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkDescriptorPool m_descriptorPool;

		const uint32_t MAX_SETS;
		uint32_t m_allocatedSetsCount;

		friend class DrawingDescriptorAllocator_Vulkan;
	};
	
	class DrawingDescriptorAllocator_Vulkan
	{
	public:
		DrawingDescriptorAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingDescriptorAllocator_Vulkan() = default;

		std::shared_ptr<DrawingDescriptorPool_Vulkan> CreateDescriptorPool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
		// TODO: add pool deletion support

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::vector<std::shared_ptr<DrawingDescriptorPool_Vulkan>> m_descriptorPools;
	};
}