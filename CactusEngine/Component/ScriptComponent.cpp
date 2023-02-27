#include "ScriptComponent.h"
#include "BaseScript.h"

namespace Engine
{
	ScriptComponent::ScriptComponent()
		: BaseComponent(EComponentType::Script)
	{
	}

	void ScriptComponent::BindScript(const std::shared_ptr<BaseScript> pScript)
	{
		m_pScript = pScript;
	}

	std::shared_ptr<BaseScript> ScriptComponent::GetScript() const
	{
		return m_pScript;
	}
}
