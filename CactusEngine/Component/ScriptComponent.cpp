#include "ScriptComponent.h"

using namespace Engine;

ScriptComponent::ScriptComponent()
	: BaseComponent(eCompType_Script)
{
}

void ScriptComponent::BindScript(const std::shared_ptr<IScript> pScript)
{
	m_pScript = pScript;
}

std::shared_ptr<IScript> ScriptComponent::GetScript() const
{
	return m_pScript;
}

