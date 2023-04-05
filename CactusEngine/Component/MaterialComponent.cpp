#include "MaterialComponent.h"

namespace Engine
{
	Material::Material()
		: m_useShaderType(EBuiltInShaderProgramType::Basic),
		m_transparentPass(false),
		m_albedoColor(Color4(1, 1, 1, 1)),
		m_anisotropy(0.0f),
		m_roughness(0.75f)
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

	void Material::SetTexture(EMaterialTextureType type, Texture2D* pTexture)
	{
		m_Textures[type] = pTexture;
	}

	Texture2D* Material::GetTexture(EMaterialTextureType type) const
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
		: BaseComponent(EComponentType::Material),
		m_hasTransparency(false),
		m_hasTransparencyChecked(false)
	{
	}

	void MaterialComponent::AddMaterial(uint32_t submeshIndex, Material* pMaterialComp)
	{
		if (submeshIndex >= m_materialList.size())
		{
			m_materialList.resize(submeshIndex + 1);
		}
		m_materialList[submeshIndex] = pMaterialComp;
	}

	const std::vector<Material*>& MaterialComponent::GetMaterialList() const
	{
		return m_materialList;
	}

	Material* MaterialComponent::GetMaterialBySubmeshIndex(uint32_t submeshIndex) const
	{
		if (submeshIndex < m_materialList.size())
		{
			return m_materialList[submeshIndex];
		}
		if (m_materialList.size() > 0)
		{
			return m_materialList[0]; // Pick first material as default
		}

		LOG_ERROR("No material can be found in the material component.");
		return nullptr;
	}

	uint32_t MaterialComponent::GetMaterialCount() const
	{
		return (uint32_t)m_materialList.size();
	}

	bool MaterialComponent::HasTransparency()
	{
		if (!m_hasTransparencyChecked)
		{
			RecheckTransparency();
		}
		return m_hasTransparency;
	}

	void MaterialComponent::RecheckTransparency()
	{
		for (auto& pMaterial : m_materialList)
		{
			if (pMaterial->IsTransparent())
			{
				m_hasTransparency = true;
				break;
			}
		}
		m_hasTransparencyChecked = true;
	}
}