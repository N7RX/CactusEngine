#include "ScriptComponent.h"
#include "BaseScript.h"

namespace Engine
{
	ScriptComponent::ScriptComponent()
		: BaseComponent(EComponentType::Script)
	{
	}

	void ScriptComponent::BindScript(BaseScript* pScript)
	{
		m_pScript = pScript;
	}

	BaseScript* ScriptComponent::GetScript() const
	{
		return m_pScript;
	}
}
