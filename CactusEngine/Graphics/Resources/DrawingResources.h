#pragma once
#include "SharedTypes.h"
#include "BasicMathTypes.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cassert>

namespace Engine
{
	class DrawingDevice;

	class RawResource
	{
	public:
		virtual ~RawResource() = default;

		virtual uint32_t GetResourceID() const;

		virtual void MarkSizeInByte(uint32_t size);
		virtual uint32_t GetSizeInByte() const;

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
	protected:
		TextureSampler() = default;
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
		EImageLayout   initialLayout;

		EGPUType	   deviceType;
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
		virtual void SetSampler(const std::shared_ptr<TextureSampler> pSampler) = 0;
		virtual std::shared_ptr<TextureSampler> GetSampler() const = 0;

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
		EGPUType	deviceType;
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
		EGPUType	 deviceType;

		std::vector<RenderPassAttachmentDescription> attachmentDescriptions;
		Color4		 clearColor;
		float		 clearDepth;
		unsigned int clearStencil;	

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
		FrameBuffer() = default;

	protected:
		uint32_t m_width;
		uint32_t m_height;
	};

	// For low-level APIs like Vulkan
	struct SwapchainFrameBuffers : public RawResource
	{
		std::vector<std::shared_ptr<FrameBuffer>> frameBuffers;
	};

	struct UniformBufferCreateInfo
	{
		uint32_t sizeInBytes;

		EGPUType deviceType;
		uint32_t appliedStages; // Bitmask, required for push constant
	};

	class SubUniformBuffer : public RawResource
	{
	public:
		virtual void UpdateSubBufferData(const void* pData) = 0;

	protected:
		SubUniformBuffer() = default;
	};

	class UniformBuffer : public RawResource
	{
	public:
		virtual void UpdateBufferData(const void* pData) = 0;
		virtual void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) = 0;
		virtual std::shared_ptr<SubUniformBuffer> AllocateSubBuffer(uint32_t size) = 0;
		virtual void ResetSubBufferAllocation() = 0;

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

		virtual unsigned int GetParamBinding(const char* paramName) const = 0;
		virtual void Reset() = 0;

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
			ShaderParameterTableEntry(unsigned int binding, EDescriptorType descType, const std::shared_ptr<RawResource> pRes)
				: binding(binding), type(descType), pResource(pRes)
			{
			}

			unsigned int					binding;
			EDescriptorType					type;
			std::shared_ptr<RawResource>	pResource;
		};

		std::vector<ShaderParameterTableEntry> m_table;

		void AddEntry(unsigned int binding, EDescriptorType descType, const std::shared_ptr<RawResource> pRes)
		{
			m_table.emplace_back(binding, descType, pRes);
		}

		void Clear()
		{
			m_table.clear();
		}
	};

	// Legacy structs for OpenGL
	// ---

	struct DeviceBlendStateInfo
	{
		bool enabled;
		EBlendFactor srcFactor;
		EBlendFactor dstFactor;
	};

	struct DeviceCullStateInfo
	{
		bool enabled;
		ECullMode cullMode;
	};

	struct DeviceDepthStateInfo
	{
		bool enableDepthTest;
		bool enableDepthMask;
	};

	// ---

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
	protected:
		GraphicsPipelineObject() = default;
	};
}