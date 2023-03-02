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

		void BindScript(BaseScript* pScript);
		BaseScript* GetScript() const;

	private:
		BaseScript* m_pScript;
	};
}