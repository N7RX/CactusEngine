#pragma once
#include "BaseComponent.h"
#include "BaseScript.h"

namespace SampleScript
{
	class BaseScript;
}

namespace Engine
{
	class ScriptComponent : public BaseComponent
	{
	public:
		ScriptComponent();
		~ScriptComponent() = default;

		void BindScript(const std::shared_ptr<SampleScript::BaseScript> pScript);
		std::shared_ptr<SampleScript::BaseScript> GetScript() const;

	private:
		std::shared_ptr<SampleScript::BaseScript> m_pScript;
	};
}