#pragma once
#include "BaseComponent.h"
#include "GraphicsResources.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	typedef std::unordered_map<EMaterialTextureType, Texture2D*> MaterialTextureList;

	class Material
	{
	public:
		Material();
		~Material() = default;

		EBuiltInShaderProgramType GetShaderProgramType() const;
		void SetShaderProgram(EBuiltInShaderProgramType shaderProgramType);

		void SetTexture(EMaterialTextureType type, Texture2D* pTexture);
		Texture2D* GetTexture(EMaterialTextureType type) const;
		const MaterialTextureList& GetTextureList() const;

		void SetAlbedoColor(Color4 albedo);
		Color4 GetAlbedoColor() const;

		void SetAnisotropy(float val);
		float GetAnisotropy() const;
		void SetRoughness(float val);
		float GetRoughness() const;

		void SetTransparent(bool val);
		bool IsTransparent() const;

	private:
		EBuiltInShaderProgramType m_useShaderType;
		bool m_transparentPass;

		MaterialTextureList m_Textures;

		Color4 m_albedoColor;

		float m_anisotropy;
		float m_roughness;
	};

	class MaterialComponent : public BaseComponent
	{
	public:
		MaterialComponent();
		~MaterialComponent() = default;

		void AddMaterial(uint32_t submeshIndex, Material* pMaterialComp);
		const std::vector<Material*>& GetMaterialList() const;
		Material* GetMaterialBySubmeshIndex(uint32_t submeshIndex) const;
		uint32_t GetMaterialCount() const;

		bool HasTransparency();
		void RecheckTransparency();

	private:
		bool m_hasTransparency;
		bool m_hasTransparencyChecked;

		std::vector<Material*> m_materialList;
	};
}