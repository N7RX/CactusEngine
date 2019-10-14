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
	switch (type)
	{
	case eMaterialTexture_Albedo:
		m_pAlbedoTexture = pTexture;
		break;
	case eMaterialTexture_Noise:
		m_pNoiseTexture = pTexture;
		break;
	default:
		std::cerr << "Unhandled texture type.\n";
		break;
	}
}

std::shared_ptr<Texture2D> MaterialComponent::GetTexture(EMaterialTextureType type) const
{
	switch (type)
	{
	case eMaterialTexture_Albedo:
		return m_pAlbedoTexture;
	case eMaterialTexture_Noise:
		return m_pNoiseTexture;
	default:
		std::cerr << "Unhandled texture type.\n";
		return nullptr;
	}
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