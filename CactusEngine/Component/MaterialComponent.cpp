#include "MaterialComponent.h"
#include <iostream>

using namespace Engine;

MaterialComponent::MaterialComponent()
	: BaseComponent(eCompType_Material), m_albedoColor(Color4(1, 1, 1, 1)), m_transparentPass(false)
{
}

EBuiltInShaderProgramType MaterialComponent::GetShaderProgramType() const
{
	return m_useShaderType;
}

void MaterialComponent::SetShaderProgram(EBuiltInShaderProgramType shaderProgramType)
{
	m_useShaderType = shaderProgramType;
}

void MaterialComponent::SetTexture(EMaterialTextureType type, const std::shared_ptr<Texture2D> pTexture)
{
	m_Textures[type] = pTexture;
}

std::shared_ptr<Texture2D> MaterialComponent::GetTexture(EMaterialTextureType type) const
{
	if (m_Textures.find(type) != m_Textures.end())
	{
		return m_Textures.at(type);
	}
	return nullptr;
}

const MaterialTextureList& MaterialComponent::GetTextureList() const
{
	return m_Textures;
}

void MaterialComponent::SetAlbedoColor(Color4 albedo)
{
	m_albedoColor = albedo;
}

Color4 MaterialComponent::GetAlbedoColor() const
{
	return m_albedoColor;
}

void MaterialComponent::SetTransparent(bool val)
{
	m_transparentPass = val;
}

bool MaterialComponent::IsTransparent() const
{
	return m_transparentPass;
}