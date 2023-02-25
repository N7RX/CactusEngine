#pragma once
#include "ScriptIDList.h"

namespace Engine
{
	class BaseScript
	{
	public:
		BaseScript();
		virtual ~BaseScript() = default;

		// TODO: find better solutions
		SampleScript::EScriptID GetScriptID();
		bool ShouldCallStart();

		virtual void Start() = 0;
		virtual void Update() = 0;

	public:
		SampleScript::EScriptID m_id;
		bool m_started;
	};
}