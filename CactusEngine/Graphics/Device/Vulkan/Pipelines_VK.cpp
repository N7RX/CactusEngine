#include "Pipelines_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GHIUtilities_VK.h"

namespace Engine
{
	RenderPass_VK::RenderPass_VK(LogicalDevice_VK* pDevice)
		: m_pDevice(pDevice),
		m_renderPass(VK_NULL_HANDLE)
	{

	}

	RenderPass_VK::~RenderPass_VK()
	{
		DEBUG_ASSERT_CE(m_renderPass != VK_NULL_HANDLE);
		vkDestroyRenderPass(m_pDevice->logicalDevice, m_renderPass, nullptr);
	}

	GraphicsPipeline_VK::GraphicsPipeline_VK(LogicalDevice_VK* pDevice, ShaderProgram_VK* pShaderProgram, VkGraphicsPipelineCreateInfo& createInfo)
		: m_pDevice(pDevice),
		m_pShaderProgram(pShaderProgram)
	{
		// TODO: support creation from cache & batched creations to reduce startup time
		m_pipelineLayout = createInfo.layout;
		m_pipeline = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(pDevice->logicalDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan: failed to create graphics pipeline.");
		}
	}

	GraphicsPipeline_VK::~GraphicsPipeline_VK()
	{
		DEBUG_ASSERT_CE(m_pipeline != VK_NULL_HANDLE);

		vkDestroyPipelineLayout(m_pDevice->logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyPipeline(m_pDevice->logicalDevice, m_pipeline, nullptr);
	}

	VkPipeline GraphicsPipeline_VK::GetPipeline() const
	{
		return m_pipeline;
	}

	VkPipelineLayout GraphicsPipeline_VK::GetPipelineLayout() const
	{
		return m_pipelineLayout;
	}

	VkPipelineBindPoint GraphicsPipeline_VK::GetBindPoint() const
	{
		return VK_PIPELINE_BIND_POINT_GRAPHICS;
	}

	PipelineVertexInputState_VK::PipelineVertexInputState_VK(const std::vector<VkVertexInputBindingDescription>& bindingDescs, const std::vector<VkVertexInputAttributeDescription>& attributeDescs)
		: m_vertexBindingDesc(bindingDescs), m_vertexAttributeDesc(attributeDescs)
	{
		m_pipelineVertexInputStateCreateInfo = {};
		m_pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)m_vertexBindingDesc.size();
		m_pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = m_vertexBindingDesc.data();
		m_pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)m_vertexAttributeDesc.size();
		m_pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = m_vertexAttributeDesc.data();
	}

	const VkPipelineVertexInputStateCreateInfo* PipelineVertexInputState_VK::GetVertexInputStateCreateInfo() const
	{
		return &m_pipelineVertexInputStateCreateInfo;
	}

	PipelineInputAssemblyState_VK::PipelineInputAssemblyState_VK(const PipelineInputAssemblyStateCreateInfo& createInfo)
	{
		m_pipelineInputAssemblyStateCreateInfo = {};
		m_pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_pipelineInputAssemblyStateCreateInfo.topology = VulkanPrimitiveTopology(createInfo.topology);
		m_pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = createInfo.enablePrimitiveRestart ? VK_TRUE : VK_FALSE;
	}

	const VkPipelineInputAssemblyStateCreateInfo* PipelineInputAssemblyState_VK::GetInputAssemblyStateCreateInfo() const
	{
		return &m_pipelineInputAssemblyStateCreateInfo;
	}

	PipelineColorBlendState_VK::PipelineColorBlendState_VK(const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachmentStates)
		: m_colorBlendAttachmentStates(blendAttachmentStates)
	{
		m_pipelineColorBlendStateCreateInfo = {};
		m_pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		m_pipelineColorBlendStateCreateInfo.attachmentCount = (uint32_t)m_colorBlendAttachmentStates.size();
		m_pipelineColorBlendStateCreateInfo.pAttachments = m_colorBlendAttachmentStates.data();
		// Alert: these values might need to change according to blend factor
		m_pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		m_pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		m_pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		m_pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	}

	const VkPipelineColorBlendStateCreateInfo* PipelineColorBlendState_VK::GetColorBlendStateCreateInfo() const
	{
		return &m_pipelineColorBlendStateCreateInfo;
	}

	void PipelineColorBlendState_VK::EnableLogicOperation()
	{
		// Applied only for signed and unsigned integer and normalized integer framebuffers
		m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_TRUE;
	}

	void PipelineColorBlendState_VK::DisableLogicOpeartion()
	{
		m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	}

	void PipelineColorBlendState_VK::SetLogicOpeartion(VkLogicOp logicOp)
	{
		m_pipelineColorBlendStateCreateInfo.logicOp = logicOp;
	}

	PipelineRasterizationState_VK::PipelineRasterizationState_VK(const PipelineRasterizationStateCreateInfo& createInfo)
	{
		m_pipelineRasterizationStateCreateInfo = {};
		m_pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_pipelineRasterizationStateCreateInfo.depthClampEnable = createInfo.enableDepthClamp ? VK_TRUE : VK_FALSE;
		m_pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = createInfo.discardRasterizerResults ? VK_TRUE : VK_FALSE;
		m_pipelineRasterizationStateCreateInfo.polygonMode = VulkanPolygonMode(createInfo.polygonMode);
		m_pipelineRasterizationStateCreateInfo.cullMode = VulkanCullMode(createInfo.cullMode);
		m_pipelineRasterizationStateCreateInfo.frontFace = createInfo.frontFaceCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		m_pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		m_pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	}

	const VkPipelineRasterizationStateCreateInfo* PipelineRasterizationState_VK::GetRasterizationStateCreateInfo() const
	{
		return &m_pipelineRasterizationStateCreateInfo;
	}

	PipelineDepthStencilState_VK::PipelineDepthStencilState_VK(const PipelineDepthStencilStateCreateInfo& createInfo)
	{
		m_pipelineDepthStencilStateCreateInfo = {};
		m_pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_pipelineDepthStencilStateCreateInfo.depthTestEnable = createInfo.enableDepthTest ? VK_TRUE : VK_FALSE;
		m_pipelineDepthStencilStateCreateInfo.depthWriteEnable = createInfo.enableDepthWrite ? VK_TRUE : VK_FALSE;
		m_pipelineDepthStencilStateCreateInfo.depthCompareOp = VulkanCompareOp(createInfo.depthCompareOP);
		m_pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		m_pipelineDepthStencilStateCreateInfo.stencilTestEnable = createInfo.enableStencilTest ? VK_TRUE : VK_FALSE;

		// TODO: add support for stencil operations and depth bounds test
	}

	const VkPipelineDepthStencilStateCreateInfo* PipelineDepthStencilState_VK::GetDepthStencilStateCreateInfo() const
	{
		return &m_pipelineDepthStencilStateCreateInfo;
	}

	PipelineMultisampleState_VK::PipelineMultisampleState_VK(const PipelineMultisampleStateCreateInfo& createInfo)
	{
		m_pipelineMultisampleStateCreateInfo = {};
		m_pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_pipelineMultisampleStateCreateInfo.rasterizationSamples = VulkanSampleCount(createInfo.sampleCount);
		m_pipelineMultisampleStateCreateInfo.sampleShadingEnable = createInfo.enableSampleShading ? VK_TRUE : VK_FALSE;

		// TODO: add support for sample mask and alpha config
	}

	const VkPipelineMultisampleStateCreateInfo* PipelineMultisampleState_VK::GetMultisampleStateCreateInfo() const
	{
		return &m_pipelineMultisampleStateCreateInfo;
	}

	PipelineViewportState_VK::PipelineViewportState_VK(const PipelineViewportStateCreateInfo& createInfo)
	{
		m_viewport = {};
		m_viewport.x = 0.0f;
		m_viewport.y = (float)createInfo.height; // Flipping the viewport in compatibility with OpenGL coordinate system
		m_viewport.width = (float)createInfo.width;
		m_viewport.height = -(float)createInfo.height;
		m_viewport.minDepth = 0.0f;
		m_viewport.maxDepth = 1.0f;

		m_scissor.offset = { 0, 0 };
		m_scissor.extent = { createInfo.width, createInfo.height };

		m_pipelineViewportStateCreateInfo = {};
		m_pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_pipelineViewportStateCreateInfo.viewportCount = 1;
		m_pipelineViewportStateCreateInfo.pViewports = &m_viewport;
		m_pipelineViewportStateCreateInfo.scissorCount = 1;
		m_pipelineViewportStateCreateInfo.pScissors = &m_scissor;
	}

	const VkPipelineViewportStateCreateInfo* PipelineViewportState_VK::GetViewportStateCreateInfo() const
	{
		return &m_pipelineViewportStateCreateInfo;
	}
}