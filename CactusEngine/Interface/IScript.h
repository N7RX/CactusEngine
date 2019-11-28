#pragma once
#include "AllComponents.h"
#include "ScriptIDList.h"

namespace Engine
{
	__interface IScript
	{
	public:
		// TODO: find better solutions
		SampleScript::EScriptID GetScriptID();
		bool ShouldCallStart();
		
		void Start();
		void Update();
	};
}