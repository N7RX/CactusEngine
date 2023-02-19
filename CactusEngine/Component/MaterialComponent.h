#pragma once
#include "BaseComponent.h"
#include "GraphicsResources.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	typedef std::unordered_map<EMaterialTextureType, std::shared_ptr<Texture2D>> MaterialTextureList;

	class Material
	{
	public:
		Material();
		~Material() = default;

		EBuiltInShaderProgramType GetShaderProgramType() const;
		void SetShaderProgram(EBuiltInShaderProgramType shaderProgramType);

		void SetTexture(EMaterialTextureType type, const std::shared_ptr<Texture2D> pTexture);
		std::shared_ptr<Texture2D> GetTexture(EMaterialTextureType type) const;
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

	typedef std::unordered_map<unsigned int, std::shared_ptr<Material>> MaterialList;

	class MaterialComponent : public BaseComponent
	{
	public:
		MaterialComponent();
		~MaterialComponent() = default;

		void AddMaterial(unsigned int submeshIndex, const std::shared_ptr<Material> pMaterialComp);
		const MaterialList& GetMaterialList() const;
		const std::shared_ptr<Material> GetMaterialBySubmeshIndex(unsigned int submeshIndex) const;
		unsigned int GetMaterialCount() const;

	private:
		MaterialList m_materialList; // This is a temporary solution, a better way is to implement auto sub-entity creation
	};
}