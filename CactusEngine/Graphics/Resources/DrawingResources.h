#pragma once
#include "SharedTypes.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cassert>

namespace Engine
{
	class DrawingDevice;

	class RawResourceData
	{
	public:
		RawResourceData() = default;
		virtual ~RawResourceData() = default;

		const void* m_pData; // Alert: careful with resource memory management
		uint32_t m_sizeInBytes;
	};

	class RawResource
	{
	public:
		virtual ~RawResource() = default;

		virtual uint32_t GetResourceID() const;

		virtual uint32_t GetSize() const;
		virtual std::shared_ptr<RawResourceData> GetData() const;

		virtual void SetData(const std::shared_ptr<RawResourceData> pData, uint32_t size);

	protected:
		RawResource();

	protected:
		uint32_t m_resourceID;
		uint32_t m_sizeInBytes;
		std::shared_ptr<RawResourceData> m_pRawData;

	private:
		static uint32_t m_assignedID;
	};

	struct VertexBufferCreateInfo
	{
		int*	 pIndexData;
		uint32_t indexDataCount;

		float*	 pPositionData;
		uint32_t positionDataCount;
		static const uint32_t positionOffset = 0;

		float*	 pNormalData;
		uint32_t normalDataCount;
		static const uint32_t normalOffset = 3 * sizeof(float);

		float*	 pTexcoordData;
		uint32_t texcoordDataCount;
		static const uint32_t texcoordOffset = 6 * sizeof(float);

		float*	 pTangentData;
		uint32_t tangentDataCount;
		static const uint32_t tangentOffset = 8 * sizeof(float);

		float*	 pBitangentData;
		uint32_t bitangentDataCount;
		static const uint32_t bitangentOffset = 11 * sizeof(float);

		static const uint32_t interleavedStride = 14 * sizeof(float); // 3 + 3 + 2 + 3 + 3

		std::vector<float> ConvertToInterleavedData() const; // This would not pack index
	};

	class VertexBuffer : public RawResource
	{
	public:
		void SetNumberOfIndices(uint32_t count);
		uint32_t GetNumberOfIndices() const;

	protected:
		uint32_t m_numberOfIndices;
	};

	struct TextureSamplerCreateInfo
	{
		EGPUType			deviceType;
		ESamplerFilterMode	magMode;
		ESamplerFilterMode	minMode;
		ESamplerAddressMode	addressMode;
		bool				enableAnisotropy;
		float				maxAnisotropy;
		bool				enableCompareOp;
		ECompareOperation	compareOp;
		ESamplerMipmapMode	mipmapMode;
		float				minLod;
		float				maxLod;
		float				minLodBias;
	};

	class TextureSampler
	{

	};

	struct Texture2DCreateInfo
	{
		const void*	   pTextureData;
		uint32_t	   textureWidth;
		uint32_t	   textureHeight;
		EDataType	   dataType;
		ETextureFormat format;
		ETextureType   textureType;
		bool		   generateMipmap;
		std::shared_ptr<TextureSampler> pSampler;

		EGPUType	   deviceType;
	};

	class Texture2D : public RawResource
	{
	public:
		virtual ~Texture2D() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		virtual uint32_t GetTextureID() const = 0;

		void SetTextureType(ETextureType type);
		ETextureType GetTextureType() const;

		void SetFilePath(const char* filePath);
		const char* GetFilePath() const;

		virtual bool HasSampler() const = 0;
		virtual void SetSampler(const std::shared_ptr<TextureSampler> pSampler) = 0;
		virtual std::shared_ptr<TextureSampler> GetSampler() const = 0;

	protected:
		uint32_t m_width;
		uint32_t m_height;

		ETextureType m_type;
		std::string m_filePath; // TODO: remove this property
	};

	struct RenderPassAttachmentDescription
	{
		// This is strictly modeled after Vulkan specification, may not be versatile
		ETextureFormat			format;
		uint32_t				sampleCount;
		EAttachmentOperation	loadOp;
		EAttachmentOperation	storeOp;
		EAttachmentOperation	stencilLoadOp;
		EAttachmentOperation	stencilStoreOp;
		EImageLayout			initialLayout;
		EImageLayout			usageLayout;
		EImageLayout			finalLayout;

		EAttachmentType			type;
		uint32_t				index;
	};

	struct RenderPassCreateInfo
	{
		std::vector<RenderPassAttachmentDescription> attachmentDescriptions;
		EGPUType deviceType;

		// TODO: add subpass description
	};

	class RenderPassObject : public RawResource
	{

	};

	struct FrameBufferCreateInfo
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		std::vector<std::shared_ptr<Texture2D>> attachments;

		EGPUType deviceType;
		std::shared_ptr<RenderPassObject> pRenderPass;
	};

	class FrameBuffer : public RawResource
	{
	public:
		virtual ~FrameBuffer() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		virtual uint32_t GetFrameBufferID() const = 0;

	protected:
		uint32_t m_width;
		uint32_t m_height;
	};

	struct SwapchainFrameBuffers : public RawResource
	{
		std::vector<std::shared_ptr<FrameBuffer>> frameBuffers;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;
		EShaderType GetType() const;

	protected:
		Shader(EShaderType type);

	protected:
		EShaderType m_shaderType;
	};

	class VertexShader : public Shader
	{
	protected:
		VertexShader();
	};

	class FragmentShader : public Shader
	{
	protected:
		FragmentShader();
	};

	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() = default;

		void SetProgramID(uint32_t id);

		uint32_t GetProgramID() const;
		uint32_t GetShaderStages() const;

		// Works on higher level APIs like OpenGL
		virtual unsigned int GetParamLocation(const char* paramName) const = 0;
		virtual void Reset() = 0;

		// For lower level APIs like Vulkan
		virtual unsigned int GetParamBinding(const char* paramName) const = 0;

	protected:
		ShaderProgram(uint32_t shaderStages);

	protected:
		uint32_t m_programID;
		DrawingDevice* m_pDevice;
		uint32_t m_shaderStages; // This is a bitmap
	};

	struct ShaderParameterTable
	{
		ShaderParameterTable() = default;
		~ShaderParameterTable()
		{
			Clear();
		}
		
		// For high-level APIs like OpenGL
		struct ShaderParameterTableEntry
		{
			ShaderParameterTableEntry(unsigned int loc, EShaderParamType paramType, const void* pVal)
				: location(loc), type(paramType), pValue(pVal)
			{
			}

			unsigned int		location;
			EShaderParamType	type;
			const void*			pValue;
		};

		std::vector<ShaderParameterTableEntry> m_table;

		void AddEntry(unsigned int loc, EShaderParamType type, const void* pVal)
		{
			m_table.emplace_back(loc, type, pVal);
		}

		// For low-level APIs like Vulkan
		struct DescriptorUpdateTableEntry
		{
			DescriptorUpdateTableEntry(unsigned int bindingIndex, EDescriptorType descType, const std::shared_ptr<RawResource> pRes)
				: binding(bindingIndex), type(descType), pResource(pRes), pValue(nullptr)
			{
			}

			DescriptorUpdateTableEntry(unsigned int bindingIndex, EDescriptorType descType, const void* pVal)
				: binding(bindingIndex), type(descType), pValue(pVal), pResource(nullptr)
			{
			}

			unsigned int					binding;
			EDescriptorType					type;
			std::shared_ptr<RawResource>	pResource;
			const void*						pValue;
		};

		std::vector<DescriptorUpdateTableEntry> m_hpTable;

		void AddEntry(unsigned int binding, EDescriptorType descType, const std::shared_ptr<RawResource> pRes)
		{		
			m_hpTable.emplace_back(binding, descType, pRes);
		}

		void AddEntry(unsigned int binding, EDescriptorType descType, const void* pVal) // For numerical uniform buffer only
		{
			assert(descType == EDescriptorType::UniformBuffer); // Uniform buffers are managed by (Vulkan) shader program internally
			m_hpTable.emplace_back(binding, descType, pVal);
		}

		void Clear()
		{
			m_table.clear();
			m_hpTable.clear();
		}
	};

	// TODO: remove this legacy struct
	struct DeviceBlendStateInfo
	{
		bool enabled;
		EBlendFactor srcFactor;
		EBlendFactor dstFactor;
	};

	struct VertexInputBindingDescription
	{
		uint32_t			binding;
		uint32_t			stride;
		EVertexInputRate	inputRate;
	};

	struct VertexInputAttributeDescription
	{
		uint32_t		location;
		uint32_t		binding;
		ETextureFormat	format;
		uint32_t		offset;
	};

	struct PipelineVertexInputStateCreateInfo
	{
		std::vector<VertexInputBindingDescription> bindingDescs;
		std::vector<VertexInputAttributeDescription> attributeDescs;
	};

	class PipelineVertexInputState
	{

	};

	struct PipelineInputAssemblyStateCreateInfo
	{
		EAssemblyTopology	topology;
		bool				enablePrimitiveRestart;
	};

	class PipelineInputAssemblyState
	{

	};

	struct AttachmentColorBlendStateDescription
	{
		std::shared_ptr<Texture2D> pAttachment;

		bool enableBlend;
		EBlendFactor	srcColorBlendFactor;
		EBlendFactor	dstColorBlendFactor;
		EBlendOperation	colorBlendOp;
		EBlendFactor	srcAlphaBlendFactor;
		EBlendFactor	dstAlphaBlendFactor;
		EBlendOperation	alphaBlendOp;

		// TODO: support for color write mask
	};

	struct PipelineColorBlendStateCreateInfo
	{
		std::vector<AttachmentColorBlendStateDescription> blendStateDescs;
	};

	class PipelineColorBlendState
	{

	};

	struct PipelineRasterizationStateCreateInfo
	{
		bool			enableDepthClamp;
		EPolygonMode	polygonMode;
		ECullMode		cullMode;
	};

	class PipelineRasterizationState
	{

	};

	struct PipelineDepthStencilStateCreateInfo
	{
		bool				enableDepthTest;
		bool				enableDepthWrite;
		bool				enableStencilTest;
		ECompareOperation	depthCompareOP;
		// TODO: include stencil compare op
	};

	class PipelineDepthStencilState
	{

	};

	struct PipelineMultisampleStateCreateInfo
	{
		uint32_t	sampleCount;
		bool		enableSampleShading;
	};

	class PipelineMultisampleState
	{

	};

	struct PipelineViewportStateCreateInfo
	{
		uint32_t width;
		uint32_t height;

		// TODO: add support for multi-viewport and scissor control
	};

	class PipelineViewportState
	{

	};

	struct GraphicsPipelineCreateInfo
	{
		EGPUType									deviceType;
		std::shared_ptr<ShaderProgram>				pShaderProgram;
		std::shared_ptr<PipelineVertexInputState>	pVertexInputState;
		std::shared_ptr<PipelineInputAssemblyState>	pInputAssemblyState;
		std::shared_ptr<PipelineColorBlendState>	pColorBlendState;
		std::shared_ptr<PipelineRasterizationState>	pRasterizationState;
		std::shared_ptr<PipelineDepthStencilState>	pDepthStencilState;
		std::shared_ptr<PipelineMultisampleState>	pMultisampleState;
		std::shared_ptr<PipelineViewportState>		pViewportState;
		std::shared_ptr<RenderPassObject>			pRenderPass;
		//uint32_t									subpassIndex; // TODO: add subpass support
	};

	class GraphicsPipelineObject : public RawResource
	{

	};
}