#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"

namespace Engine
{
	struct LogicalDevice_VK;
	class ShaderProgram_VK;
	class PipelineViewportState_VK;

	class RenderPass_VK : public RenderPassObject
	{
	public:
		RenderPass_VK(LogicalDevice_VK* pDevice);
		~RenderPass_VK();

	private:
		LogicalDevice_VK* m_pDevice;
		VkRenderPass m_renderPass;
		std::vector<VkClearValue> m_clearValues;

		friend class GraphicsHardwareInterface_VK;
	};

	class GraphicsPipeline_VK : public GraphicsPipelineObject
	{
	public:
		GraphicsPipeline_VK(LogicalDevice_VK* pDevice, ShaderProgram_VK* pShaderProgram, VkGraphicsPipelineCreateInfo& createInfo);
		~GraphicsPipeline_VK();

		void UpdateViewportState(const PipelineViewportState* pNewState) override;

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;
		VkPipelineBindPoint GetBindPoint() const;

		const VkViewport* GetViewport() const;
		const VkRect2D* GetScissor() const;

	private:
		LogicalDevice_VK* m_pDevice;
		ShaderProgram_VK* m_pShaderProgram;
		PipelineViewportState_VK* m_pViewportState;

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};

	class PipelineVertexInputState_VK : public PipelineVertexInputState
	{
	public:
		PipelineVertexInputState_VK(const std::vector<VkVertexInputBindingDescription>& bindingDescs, const std::vector<VkVertexInputAttributeDescription>& attributeDescs);
		~PipelineVertexInputState_VK() = default;

		const VkPipelineVertexInputStateCreateInfo* GetVertexInputStateCreateInfo() const;

	private:
		std::vector<VkVertexInputBindingDescription>	m_vertexBindingDesc;
		std::vector<VkVertexInputAttributeDescription>	m_vertexAttributeDesc;
		VkPipelineVertexInputStateCreateInfo			m_pipelineVertexInputStateCreateInfo;
	};

	class PipelineInputAssemblyState_VK : public PipelineInputAssemblyState
	{
	public:
		PipelineInputAssemblyState_VK(const PipelineInputAssemblyStateCreateInfo& createInfo);
		~PipelineInputAssemblyState_VK() = default;

		const VkPipelineInputAssemblyStateCreateInfo* GetInputAssemblyStateCreateInfo() const;

	private:
		VkPipelineInputAssemblyStateCreateInfo m_pipelineInputAssemblyStateCreateInfo;
	};

	class PipelineColorBlendState_VK : public PipelineColorBlendState
	{
	public:
		PipelineColorBlendState_VK(const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachmentStates);
		~PipelineColorBlendState_VK() = default;

		const VkPipelineColorBlendStateCreateInfo* GetColorBlendStateCreateInfo() const;

		void EnableLogicOperation();
		void DisableLogicOpeartion();
		void SetLogicOpeartion(VkLogicOp logicOp);

	private:
		std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;
		VkPipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo;
	};

	class PipelineRasterizationState_VK : public PipelineRasterizationState
	{
	public:
		PipelineRasterizationState_VK(const PipelineRasterizationStateCreateInfo& createInfo);
		~PipelineRasterizationState_VK() = default;

		const VkPipelineRasterizationStateCreateInfo* GetRasterizationStateCreateInfo() const;

	private:
		VkPipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo;
	};

	class PipelineDepthStencilState_VK : public PipelineDepthStencilState
	{
	public:
		PipelineDepthStencilState_VK(const PipelineDepthStencilStateCreateInfo& createInfo);
		~PipelineDepthStencilState_VK() = default;

		const VkPipelineDepthStencilStateCreateInfo* GetDepthStencilStateCreateInfo() const;

	private:
		VkPipelineDepthStencilStateCreateInfo m_pipelineDepthStencilStateCreateInfo;
	};

	class PipelineMultisampleState_VK : public PipelineMultisampleState
	{
	public:
		PipelineMultisampleState_VK(const PipelineMultisampleStateCreateInfo& createInfo);
		~PipelineMultisampleState_VK() = default;

		const VkPipelineMultisampleStateCreateInfo* GetMultisampleStateCreateInfo() const;

	private:
		VkPipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo;
	};

	class PipelineViewportState_VK : public PipelineViewportState
	{
	public:
		PipelineViewportState_VK(const PipelineViewportStateCreateInfo& createInfo);
		~PipelineViewportState_VK() = default;

		void UpdateResolution(uint32_t width, uint32_t height) override;

		const VkPipelineViewportStateCreateInfo* GetViewportStateCreateInfo() const;
		const VkViewport* GetViewport() const;
		const VkRect2D* GetScissor() const;

	private:
		// Currently only one viewport is supported
		VkViewport m_viewport;
		VkRect2D m_scissor;
		VkPipelineViewportStateCreateInfo m_pipelineViewportStateCreateInfo;
	};
}
