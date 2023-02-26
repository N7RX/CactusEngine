#include "MaterialComponent.h"

namespace Engine
{
	Material::Material()
		: m_useShaderType(EBuiltInShaderProgramType::Basic), m_transparentPass(false), m_albedoColor(Color4(1, 1, 1, 1)), m_anisotropy(0.0f), m_roughness(0.75f)
	{
	}

	EBuiltInShaderProgramType Material::GetShaderProgramType() const
	{
		return m_useShaderType;
	}

	void Material::SetShaderProgram(EBuiltInShaderProgramType shaderProgramType)
	{
		m_useShaderType = shaderProgramType;
	}

	void Material::SetTexture(EMaterialTextureType type, const std::shared_ptr<Texture2D> pTexture)
	{
		m_Textures[type] = pTexture;
	}

	std::shared_ptr<Texture2D> Material::GetTexture(EMaterialTextureType type) const
	{
		if (m_Textures.find(type) != m_Textures.end())
		{
			return m_Textures.at(type);
		}
		return nullptr;
	}

	const MaterialTextureList& Material::GetTextureList() const
	{
		return m_Textures;
	}

	void Material::SetAlbedoColor(Color4 albedo)
	{
		m_albedoColor = albedo;
	}

	Color4 Material::GetAlbedoColor() const
	{
		return m_albedoColor;
	}

	void Material::SetAnisotropy(float val)
	{
		m_anisotropy = val;
	}

	float Material::GetAnisotropy() const
	{
		return m_anisotropy;
	}

	void Material::SetRoughness(float val)
	{
		m_roughness = val;
	}

	float Material::GetRoughness() const
	{
		return m_roughness;
	}

	void Material::SetTransparent(bool val)
	{
		m_transparentPass = val;
	}

	bool Material::IsTransparent() const
	{
		return m_transparentPass;
	}

	MaterialComponent::MaterialComponent()
		: BaseComponent(EComponentType::Material)
	{
	}

	void MaterialComponent::AddMaterial(unsigned int submeshIndex, const std::shared_ptr<Material> pMaterialComp)
	{
		m_materialList[submeshIndex] = pMaterialComp;
	}

	const MaterialList& MaterialComponent::GetMaterialList() const
	{
		return m_materialList;
	}

	const std::shared_ptr<Material> MaterialComponent::GetMaterialBySubmeshIndex(unsigned int submeshIndex) const
	{
		if (m_materialList.find(submeshIndex) != m_materialList.end())
		{
			return m_materialList.at(submeshIndex);
		}
		if (m_materialList.size() > 0)
		{
			return m_materialList.begin()->second; // Pick a material as default
		}
		return nullptr;
	}

	unsigned int MaterialComponent::GetMaterialCount() const
	{
		return (unsigned int)m_materialList.size();
	}
}