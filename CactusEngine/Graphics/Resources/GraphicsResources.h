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

		virtual uint64_t GetResourceID() const;

		virtual void MarkSizeInByte(uint32_t size);
		virtual uint32_t GetSizeInBytes() const;

	protected:
		RawResource();

	protected:
		uint64_t m_resourceID;
		uint32_t m_sizeInBytes;

	private:
		static uint64_t m_assignedID;
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

		static const uint32_t interleavedStride = 11 * sizeof(float); // 3 + 3 + 2 + 3

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
		bool		   reserveMipmapMemory; // Reserve memory space for mipmap, but not generate at creation. Useful for render textures
											// If generateMipmap is true, this will be ignored as if always true
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
		ETextureFormat		 format;
		uint32_t			 sampleCount;
		EAttachmentOperation loadOp;
		EAttachmentOperation storeOp;
		EAttachmentOperation stencilLoadOp;
		EAttachmentOperation stencilStoreOp;
		EImageLayout		 initialLayout;
		EImageLayout		 usageLayout;
		EImageLayout		 finalLayout;

		EAttachmentType		 type;
		uint32_t			 index;
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

	struct UniformBuffer;
	// Should not be used in render graph directly, managed by UniformBufferManager
	class BaseUniformBuffer : public RawResource
	{
	protected:
		virtual void UpdateBufferData(const void* pData) = 0; // Update whole buffer; used for push constant
		virtual void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) = 0;

		virtual UniformBuffer AllocateSubBuffer(uint32_t size) = 0;
		virtual UniformBuffer AllocateSubBuffer(uint32_t offset, uint32_t size) = 0; // For reserved region only
		virtual uint32_t ReserveBufferRegion(uint32_t size) = 0; // Return the internal offset of the reserved region

		virtual void ResetSubBufferAllocation() = 0;

	protected:
		BaseUniformBuffer() = default;

		friend struct UniformBuffer;
		friend class UniformBufferManager;
	};

	// Sub buffer region allocated from a large base uniform buffer
	// Because shader update takes RawResource input, it has to inherit from RawResource
	struct UniformBuffer : public RawResource
	{
		UniformBuffer()
			: m_pParentBuffer(nullptr),
			m_offset(0)
		{
			m_sizeInBytes = 0;
		}

		UniformBuffer(BaseUniformBuffer* pParentBuffer, uint32_t offset, uint32_t size)
			: m_pParentBuffer(pParentBuffer),
			m_offset(offset)

		{
			m_sizeInBytes = size;
		}

		~UniformBuffer() = default;

		bool IsValid() const
		{
			return m_pParentBuffer != nullptr;
		}

		void UpdateBufferData(const void* pData)
		{
			m_pParentBuffer->UpdateBufferSubData(pData, m_offset, m_sizeInBytes);
		}

		BaseUniformBuffer* m_pParentBuffer;
		uint32_t m_offset;
	};

	struct UniformBufferReservedRegion
	{
		// Region identifier
		uint32_t index;
		uint32_t offset;

		// Size tracking
		uint32_t totalSize;
		uint32_t availableSize;
	};

	class UniformBufferManager
	{
	public:
		virtual ~UniformBufferManager() = default;

		// Safe to be called concurrently, but slow due to locking
		virtual UniformBuffer GetUniformBuffer(uint32_t size) = 0;

		// (Almost) Lockless version of GetUniformBuffer, but requires a reserved region
		virtual UniformBuffer GetUniformBuffer(UniformBufferReservedRegion& region, uint32_t size) = 0;
		virtual UniformBufferReservedRegion ReserveBufferRegion(uint32_t size) = 0;

		// Unsafe to be called concurrently
		virtual void ResetBufferAllocation() = 0;

		void SetCurrentFrameIndex(uint32_t index);

	protected:
		UniformBufferManager() = default;

	protected:
		uint32_t m_currentFrameIndex;
	};

	// A helper class that speeds up uniform buffer allocation from multiple threads
	// This is achieved by reducing mutex locking through reserving a region of memory for each thread
	class UniformBufferConcurrentAllocator
	{
	public:
		// reservedSize is the size of the reserved region each time, and if the current region is full, a new region of the same size will be allocated automatically
		UniformBufferConcurrentAllocator(UniformBufferManager* pBufferManager, uint32_t reservedSize);

		~UniformBufferConcurrentAllocator() = default;

		UniformBuffer GetUniformBuffer(uint32_t size);

		void ResetReservedRegion();

	private:
		UniformBufferManager* m_pBufferManager;
		uint32_t m_reservedSize;

		std::vector<UniformBufferReservedRegion> m_reservedRegions;
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
	public:
		virtual ~VertexShader() = default;

	protected:
		VertexShader();
	};

	class FragmentShader : public Shader
	{
	public:
		virtual ~FragmentShader() = default;

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
		
		struct ShaderParameterTableEntry
		{
			ShaderParameterTableEntry(uint32_t binding, EDescriptorType descType, RawResource* pRes)
				: binding(binding),
				type(descType),
				pResource(pRes)
			{

			}

			uint32_t		binding;
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
		uint32_t		 binding;
		uint32_t		 stride;
		EVertexInputRate inputRate;
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
	public:
		virtual ~PipelineVertexInputState() = default;

	protected:
		PipelineVertexInputState() = default;
	};

	struct PipelineInputAssemblyStateCreateInfo
	{
		EAssemblyTopology topology;
		bool			  enablePrimitiveRestart;
	};

	class PipelineInputAssemblyState
	{
	public:
		virtual ~PipelineInputAssemblyState() = default;

	protected:
		PipelineInputAssemblyState() = default;
	};

	struct AttachmentColorBlendStateDescription
	{
		bool			enableBlend;
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
	public:
		virtual ~PipelineColorBlendState() = default;

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
	public:
		virtual ~PipelineRasterizationState() = default;

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
	public:
		virtual ~PipelineDepthStencilState() = default;

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
	public:
		virtual ~PipelineMultisampleState() = default;

	protected:
		PipelineMultisampleState() = default;
	};

	struct PipelineViewportStateCreateInfo
	{
		uint32_t width;
		uint32_t height;
		// Optional: add support for multi-viewport and scissor control
	};

	class PipelineViewportState
	{
	public:
		virtual ~PipelineViewportState() = default;

		virtual void UpdateResolution(uint32_t width, uint32_t height) = 0;

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
	public:
		virtual ~GraphicsPipelineObject() = default;

		virtual void UpdateViewportState(const PipelineViewportState* pNewState) = 0;

	protected:
		GraphicsPipelineObject() = default;
	};
}