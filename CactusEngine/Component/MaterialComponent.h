#pragma once
#include "BaseComponent.h"
#include "DrawingResources.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	typedef std::unordered_map<EMaterialTextureType, std::shared_ptr<Texture2D>> MaterialTextureList;

	class MaterialComponent : public BaseComponent
	{
	public:
		MaterialComponent();
		~MaterialComponent() = default;

		EBuiltInShaderProgramType GetShaderProgramType() const;
		void SetShaderProgram(EBuiltInShaderProgramType shaderProgramType);

		void SetTexture(EMaterialTextureType type, const std::shared_ptr<Texture2D> pTexture);
		std::shared_ptr<Texture2D> GetTexture(EMaterialTextureType type) const;
		const MaterialTextureList& GetTextureList() const;

		void SetAlbedoColor(Color4 albedo);
		Color4 GetAlbedoColor() const;

		void SetTransparent(bool val);
		bool IsTransparent() const;

	private:
		EBuiltInShaderProgramType m_useShaderType;
		MaterialTextureList m_Textures;
		Color4 m_albedoColor;
		bool m_transparentPass;
	};
}