#pragma once
#include "SharedTypes.h"
#include "BasicMathTypes.h"

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace Engine
{
	class GraphicsDevice;

	class RawResource
	{
	public:
		virtual ~RawResource() = default;

		virtual uint32_t GetResourceID() const;

		virtual void MarkSizeInByte(uint32_t size);
		virtual uint32_t GetSizeInBytes() const;

	protected:
		RawResource();

	protected:
		uint32_t m_resourceID;
		uint32_t m_sizeInBytes;

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
		VertexBuffer() = default;

	protected:
		uint32_t m_numberOfIndices;
	};

	struct TextureSamplerCreateInfo
	{
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
	protected:
		TextureSampler() = default;
	};

	struct Texture2DCreateInfo
	{
		const void*	   pTextureData;
		uint32_t	   textureWidth;
		uint32_t	   textureHeight;
		ETextureFormat format;
		ETextureType   textureType;
		bool		   generateMipmap;
		TextureSampler* pSampler;
		EImageLayout   initialLayout;
	};

	enum class ETexture2DSource
	{
		ImageTexture = 0,
		RenderTexture,
		RawDeviceTexture,
		COUNT
	};

	class Texture2D : public RawResource
	{
	public:
		virtual ~Texture2D() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		void SetTextureType(ETextureType type);
		ETextureType GetTextureType() const;

		void SetFilePath(const char* filePath);
		const char* GetFilePath() const;

		virtual bool HasSampler() const = 0;
		virtual void SetSampler(const TextureSampler* pSampler) = 0;
		virtual TextureSampler* GetSampler() const = 0;

		ETexture2DSource QuerySource() const;

	protected:
		Texture2D(ETexture2DSource source);

	protected:
		uint32_t m_width;
		uint32_t m_height;

		const ETexture2DSource m_source;
		ETextureType m_type;
		std::string m_filePath;
	};

	struct DataTransferBufferCreateInfo
	{
		uint32_t	size;
		uint32_t	usageFlags; // Bitmap for EDataTransferBufferUsage
		EMemoryType memoryType;
		bool		cpuMapped;
	};

	class DataTransferBuffer : public RawResource
	{
	protected:
		DataTransferBuffer() = default;
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
		Color4		 clearColor;
		float		 clearDepth;
		uint32_t clearStencil;	

		// TODO: add subpass description
	};

	class RenderPassObject : public RawResource
	{
	protected:
		RenderPassObject() = default;
	};

	struct FrameBufferCreateInfo
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		std::vector<Texture2D*> attachments;

		RenderPassObject* pRenderPass;

		bool renderToSwapchain;
	};

	class FrameBuffer : public RawResource
	{
	public:
		virtual ~FrameBuffer() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		virtual uint32_t GetFrameBufferID() const = 0;

	protected:
		FrameBuffer();

	protected:
		uint32_t m_width;
		uint32_t m_height;
	};

	struct UniformBufferCreateInfo
	{
		uint32_t sizeInBytes;			// Per drawcall size
		uint32_t maxSubAllocationCount;	// Ignored by OpenGL; In Vulkan actual buffer size is sizeInBytes * maxSubAllocationCount
		uint32_t appliedStages;			// Bitmask, required for push constant
	};

	class UniformBuffer;
	struct SubUniformBuffer : public RawResource
	{
		SubUniformBuffer(UniformBuffer* pParentBuffer, uint32_t offset, uint32_t size)
			: m_pParentBuffer(pParentBuffer),
			m_offset(offset)
		{
			m_sizeInBytes = size;
		}

		~SubUniformBuffer() = default;

		UniformBuffer* m_pParentBuffer;
		uint32_t m_offset;
	};

	class UniformBuffer : public RawResource
	{
	public:
		virtual void UpdateBufferData(const void* pData, const SubUniformBuffer* pSubBuffer = nullptr) = 0;
		virtual void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) = 0;
		virtual SubUniformBuffer AllocateSubBuffer(uint32_t size) = 0;
		virtual void ResetSubBufferAllocation() = 0;
		virtual void FlushToDevice() = 0; // If buffer is updated at sub buffer granularity, this function must be called to flush the changes to device

	protected:
		UniformBuffer() = default;
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

		virtual uint32_t GetParamBinding(const char* paramName) const = 0;

	protected:
		ShaderProgram(uint32_t shaderStages);

	protected:
		uint32_t m_programID;
		GraphicsDevice* m_pDevice;
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
			ShaderParameterTableEntry(uint32_t binding, EDescriptorType descType, RawResource* pRes)
				: binding(binding),
				type(descType),
				pResource(pRes)
			{

			}

			uint32_t	binding;
			EDescriptorType	type;
			RawResource*	pResource;
		};
		std::vector<ShaderParameterTableEntry> m_table;

		void AddEntry(uint32_t binding, EDescriptorType descType, RawResource* pRes)
		{
			m_table.emplace_back(binding, descType, pRes);
		}

		void Clear()
		{
			m_table.clear();
		}
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
	protected:
		PipelineVertexInputState() = default;
	};

	struct PipelineInputAssemblyStateCreateInfo
	{
		EAssemblyTopology	topology;
		bool				enablePrimitiveRestart;
	};

	class PipelineInputAssemblyState
	{
	protected:
		PipelineInputAssemblyState() = default;
	};

	struct AttachmentColorBlendStateDescription
	{
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
	protected:
		PipelineColorBlendState() = default;
	};

	struct PipelineRasterizationStateCreateInfo
	{
		bool			enableDepthClamp;
		bool			discardRasterizerResults; // Used for obtaining vertices output only
		EPolygonMode	polygonMode;
		ECullMode		cullMode;
		bool			frontFaceCounterClockwise;
	};

	class PipelineRasterizationState
	{
	protected:
		PipelineRasterizationState() = default;
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
	protected:
		PipelineDepthStencilState() = default;
	};

	struct PipelineMultisampleStateCreateInfo
	{
		uint32_t	sampleCount;
		bool		enableSampleShading;
	};

	class PipelineMultisampleState
	{
	protected:
		PipelineMultisampleState() = default;
	};

	struct PipelineViewportStateCreateInfo
	{
		uint32_t width;
		uint32_t height;

		// TODO: add support for multi-viewport and scissor control
	};

	class PipelineViewportState
	{
	protected:
		PipelineViewportState() = default;
	};

	struct GraphicsPipelineCreateInfo
	{
		ShaderProgram*				pShaderProgram;
		PipelineVertexInputState*	pVertexInputState;
		PipelineInputAssemblyState*	pInputAssemblyState;
		PipelineColorBlendState*	pColorBlendState;
		PipelineRasterizationState*	pRasterizationState;
		PipelineDepthStencilState*	pDepthStencilState;
		PipelineMultisampleState*	pMultisampleState;
		PipelineViewportState*		pViewportState;
		RenderPassObject*			pRenderPass;
		//uint32_t					subpassIndex; // TODO: add subpass support
	};

	class GraphicsPipelineObject : public RawResource
	{
	protected:
		GraphicsPipelineObject() = default;
	};
}