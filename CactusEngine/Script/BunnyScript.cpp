#include "BunnyScript.h"
#include "InputSystem.h"
#include "Timer.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace SampleScript;
using namespace Engine;

int BunnyScript::m_instanceCounter = 0;

BunnyScript::BunnyScript(const std::shared_ptr<Engine::IEntity> pEntity)
	: m_id(eScript_Bunny), m_pEntity(pEntity), m_started(false), m_pBunnyTransform(nullptr)
{
	assert(pEntity != nullptr);

	m_instanceIndex = m_instanceCounter;
	m_instanceCounter++;

	m_instanceCounter %= 2;
}

EScriptID BunnyScript::GetScriptID()
{
	return m_id;
}

bool BunnyScript::ShouldCallStart()
{
	return !m_started;
}

void BunnyScript::Start()
{
	m_pBunnyTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
	assert(m_pBunnyTransform != nullptr);

	m_started = true;
}

void BunnyScript::Update()
{
	if (m_instanceIndex == 0)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		Vector3 currPos = m_pBunnyTransform->GetPosition();
		currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f), 0.0f, 1.0f);
		m_pBunnyTransform->SetPosition(currPos);
	}
	else
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		Vector3 currPos = m_pBunnyTransform->GetPosition();
		currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f + 3.1415926536f), 0.0f, 1.0f);
		m_pBunnyTransform->SetPosition(currPos);
	}
}