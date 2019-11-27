#include "ScriptComponent.h"

using namespace Engine;

ScriptComponent::ScriptComponent()
	: BaseComponent(eCompType_Script)
{
}

void ScriptComponent::BindScript(const std::shared_ptr<SampleScript::BaseScript> pScript)
{
	m_pScript = pScript;
}

std::shared_ptr<SampleScript::BaseScript> ScriptComponent::GetScript() const
{
	return m_pScript;
}

