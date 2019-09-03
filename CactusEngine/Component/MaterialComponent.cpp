#include "MaterialComponent.h"

using namespace Engine;

MaterialComponent::MaterialComponent()
	: BaseComponent(eCompType_Material), m_albedoColor(Color4(1, 1, 1, 1))
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

void MaterialComponent::SetAlbedoColor(Color4 albedo)
{
	m_albedoColor = albedo;
}

Color4 MaterialComponent::GetAlbedoColor() const
{
	return m_albedoColor;
}