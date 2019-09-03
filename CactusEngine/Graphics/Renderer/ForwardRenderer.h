#pragma once
#include "BaseRenderer.h"

namespace Engine
{
	class ForwardRenderer : public BaseRenderer
	{
	public:
		ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~ForwardRenderer() = default;

		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;
	};
}