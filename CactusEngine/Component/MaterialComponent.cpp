#include "MaterialComponent.h"

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

void MaterialComponent::SetAlbedoTexture(const std::shared_ptr<Texture2D> pAlbedoTexture)
{
	m_pAlbedoTexture = pAlbedoTexture;
}

std::shared_ptr<Texture2D> MaterialComponent::GetAlbedoTexture() const
{
	return m_pAlbedoTexture;
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