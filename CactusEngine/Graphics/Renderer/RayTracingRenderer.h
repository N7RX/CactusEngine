#pragma once
#include "BaseRenderer.h"
#include "BuiltInShaderType.h"
#include "CommandResources.h"
#include "SafeBasicTypes.h"

namespace Engine
{
	class RayTracingRenderer : public BaseRenderer, std::enable_shared_from_this<RayTracingRenderer>
	{
	public:
		RayTracingRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~RayTracingRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;
		void WriteCommandRecordList(const char* pNodeName, const std::shared_ptr<DrawingCommandBuffer>& pCommandBuffer) override;

	private:
		std::shared_ptr<RenderGraphResource> m_pGraphResources;

		std::unordered_map<uint32_t, std::shared_ptr<DrawingCommandBuffer>> m_commandRecordReadyList; // Submit Priority - Recorded Command Buffer
		std::unordered_map<uint32_t, bool> m_commandRecordReadyListFlag; // Submit Priority - Ready to submit or has been submitted

		std::mutex m_commandRecordListWriteMutex;
		std::condition_variable m_commandRecordListCv;
		std::queue<uint32_t> m_writtenCommandPriorities;
		bool m_newCommandRecorded;
	};
}