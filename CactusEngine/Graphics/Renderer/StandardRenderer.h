#pragma once
#include "BaseRenderer.h"
#include "BuiltInShaderType.h"
#include "SafeBasicTypes.h"

#include <unordered_map>
#include <queue>

namespace Engine
{
	class RenderGraphResource;
	class GraphicsCommandBuffer;

	class StandardRenderer : public BaseRenderer, std::enable_shared_from_this<StandardRenderer>
	{
	public:
		StandardRenderer(const std::shared_ptr<GraphicsDevice> pDevice, RenderingSystem* pSystem);
		~StandardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<BaseEntity>>& drawList, const std::shared_ptr<BaseEntity> pCamera) override;
		void WriteCommandRecordList(const char* pNodeName, const std::shared_ptr<GraphicsCommandBuffer>& pCommandBuffer) override;

	private:
		std::shared_ptr<RenderGraphResource> m_pGraphResources;

		std::unordered_map<uint32_t, std::shared_ptr<GraphicsCommandBuffer>> m_commandRecordReadyList; // Submit Priority - Recorded Command Buffer
		std::unordered_map<uint32_t, bool> m_commandRecordReadyListFlag; // Submit Priority - Ready to submit or has been submitted

		std::mutex m_commandRecordListWriteMutex;
		std::condition_variable m_commandRecordListCv;
		std::queue<uint32_t> m_writtenCommandPriorities;
		bool m_newCommandRecorded;
	};
}