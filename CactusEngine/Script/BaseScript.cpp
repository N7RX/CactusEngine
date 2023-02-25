#include "BaseScript.h"

namespace Engine
{
	BaseScript::BaseScript()
		: m_id(SampleScript::EScriptID::Invalid), m_started(false)
	{

	}

	SampleScript::EScriptID BaseScript::GetScriptID()
	{
		return m_id;
	}

	bool BaseScript::ShouldCallStart()
	{
		return !m_started;
	}
}