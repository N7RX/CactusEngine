#pragma once
#include "BaseComponent.h"
#include "DrawingResources.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	class MaterialComponent : public BaseComponent
	{
	public:
		MaterialComponent();
		~MaterialComponent() = default;

		EBuiltInShaderProgramType GetShaderProgramType() const;
		void SetShaderProgram(EBuiltInShaderProgramType shaderProgramType);

		void SetAlbedoTexture(const std::shared_ptr<Texture2D> pAlbedoTexture);
		std::shared_ptr<Texture2D> GetAlbedoTexture() const;

		void SetAlbedoColor(Color4 albedo);
		Color4 GetAlbedoColor() const;

	private:
		EBuiltInShaderProgramType m_useShaderType;
		std::shared_ptr<Texture2D> m_pAlbedoTexture;
		Color4 m_albedoColor;
	};
}