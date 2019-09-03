#pragma once
#include "SharedTypes.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace Engine
{
	__interface IEntity;
	__interface IRenderer
	{
		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera);

		ERendererType GetRendererType() const;

		void SetRendererPriority(uint32_t priority);
		uint32_t GetRendererPriority() const;
	};
}