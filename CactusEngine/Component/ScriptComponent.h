#pragma once
#include "BaseComponent.h"
#include "IScript.h"

namespace Engine
{
	__interface IScript;
	class ScriptComponent : public BaseComponent
	{
	public:
		ScriptComponent();
		~ScriptComponent() = default;

		void BindScript(const std::shared_ptr<IScript> pScript);
		std::shared_ptr<IScript> GetScript() const;

	private:
		std::shared_ptr<IScript> m_pScript;
	};
}