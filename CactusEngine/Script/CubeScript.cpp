#include "CubeScript.h"
#include "Timer.h"
#include <assert.h>
#include <iostream>

using namespace SampleScript;
using namespace Engine;

int CubeScript::m_instanceCounter = 0;

CubeScript::CubeScript(const std::shared_ptr<Engine::IEntity> pEntity)
	: m_id(eScript_Cube), m_pEntity(pEntity), m_started(false), m_pCubeTransform(nullptr), m_pCubeMaterial(nullptr)
{
	assert(pEntity != nullptr);

	m_instanceIndex = m_instanceCounter;
	m_instanceCounter++;

	m_instanceCounter %= 3;
}

EScriptID CubeScript::GetScriptID()
{
	return m_id;
}

bool CubeScript::ShouldCallStart()
{
	return !m_started;
}

void CubeScript::Start()
{
	m_pCubeTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
	assert(m_pCubeTransform != nullptr);

	if (m_instanceIndex == 2)
	{
		m_pCubeMaterial = std::static_pointer_cast<MaterialComponent>(m_pEntity->GetComponent(eCompType_Material));
		assert(m_pCubeMaterial != nullptr);
	}

	m_started = true;
}

void CubeScript::Update()
{
	if (m_instanceIndex == 0)
	{
		Vector3 currRotation = m_pCubeTransform->GetRotation();
		currRotation.y += Timer::GetFrameDeltaTime() * 200.0f;
		if (currRotation.y >= 360)
		{
			currRotation.y = 0;
		}
		m_pCubeTransform->SetRotation(currRotation);
	}
	else if (m_instanceIndex == 1)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		Vector3 currScale = abs(sinf(deltaTime * 2.0f)) * Vector3(1, 1, 1);
		m_pCubeTransform->SetScale(currScale);
	}
	else if (m_instanceIndex == 2)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		Vector3 currPos = m_pCubeTransform->GetPosition();
		currPos.x = 2.5f * sinf(deltaTime * 2.0f) + 0.25f;
		m_pCubeTransform->SetPosition(currPos);

		auto pMaterialImpl = m_pCubeMaterial->GetMaterialBySubmeshIndex(0);
		if (currPos.x < -2.2f)
		{
			pMaterialImpl->SetAlbedoColor(Color4(0.3f, 1.0f, 0.3f, 1));
		}
		if (currPos.x > 2.7f)
		{
			pMaterialImpl->SetAlbedoColor(Color4(1.0f, 0.3f, 0.3f, 1));
		}
	}
}