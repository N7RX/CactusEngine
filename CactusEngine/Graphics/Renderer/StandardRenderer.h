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

	class StandardRenderer : public BaseRenderer
	{
	public:
		StandardRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem);
		~StandardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<BaseEntity*>& drawList, BaseEntity* pCamera) override;
		void WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer) override;

	private:
		RenderGraphResource* m_pGraphResources;

		std::unordered_map<uint32_t, GraphicsCommandBuffer*> m_commandRecordReadyList; // Submit Priority - Recorded Command Buffer
		std::unordered_map<uint32_t, bool> m_commandRecordReadyListFlag; // Submit Priority - Ready to submit or has been submitted

		std::mutex m_commandRecordListWriteMutex;
		std::condition_variable m_commandRecordListCv;
		std::queue<uint32_t> m_writtenCommandPriorities;
		bool m_newCommandRecorded;
	};
}