#pragma once
#include "BaseComponent.h"
#include "BaseScript.h"

namespace Engine
{
	class ScriptComponent : public BaseComponent
	{
	public:
		ScriptComponent();
		~ScriptComponent() = default;

		void BindScript(const std::shared_ptr<BaseScript> pScript);
		std::shared_ptr<BaseScript> GetScript() const;

	private:
		std::shared_ptr<BaseScript> m_pScript;
	};
}