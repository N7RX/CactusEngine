#pragma once
#include <vulkan.h>
#include <vector>
#include <memory>

namespace Engine
{
	enum class EDescriptorInfoType_Vulkan
	{
		Buffer = 0,
		Image,
		TexelBuffer,
		COUNT
	};

	struct DesciptorUpdateInfo_Vulkan
	{
		EDescriptorInfoType_Vulkan			infoType;
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
	class DrawingDescriptorPool_Vulkan
	{
	public:
		DrawingDescriptorPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingDescriptorPool_Vulkan();

		bool AllocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, std::vector<VkDescriptorSet>& outSets, bool clearPrev = false);
		bool UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_Vulkan>& updateInfos);
		// TODO: add set copy support

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkDescriptorPool m_descriptorPool;

		friend class DrawingDescriptorAllocator_Vulkan;
	};

	class DrawingDescriptorSetLayout_Vulkan
	{
	public:
		DrawingDescriptorSetLayout_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
		~DrawingDescriptorSetLayout_Vulkan();

		VkDescriptorSetLayout GetSetLayout() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkDescriptorSetLayout m_setLayout;
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