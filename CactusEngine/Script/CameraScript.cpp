#include "CameraScript.h"
#include "InputSystem.h"
#include "Timer.h"
#include <assert.h>
#include <iostream>

using namespace SampleScript;
using namespace Engine;

CameraScript::CameraScript(const std::shared_ptr<Engine::IEntity> pEntity)
	: m_id(eScript_Camera), m_pEntity(pEntity), m_started(false), m_pCameraTransform(nullptr)
{
	assert(pEntity != nullptr);
	m_prevCursorPosition = Vector2(0);
}

EScriptID CameraScript::GetScriptID()
{
	return m_id;
}

bool CameraScript::ShouldCallStart()
{
	return !m_started;
}

void CameraScript::Start()
{
	m_pCameraTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
	assert(m_pCameraTransform != nullptr);

	m_started = true;
}

void CameraScript::Update()
{
	Vector3 currPos = m_pCameraTransform->GetPosition();

	if (InputSystem::GetKeyPress('w'))
	{				
		currPos += m_pCameraTransform->GetForwardDirection() * Timer::GetFrameDeltaTime();
		m_pCameraTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('s'))
	{
		currPos -= m_pCameraTransform->GetForwardDirection() * Timer::GetFrameDeltaTime();
		m_pCameraTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('a'))
	{
		currPos -= m_pCameraTransform->GetRightDirection() * Timer::GetFrameDeltaTime();
		m_pCameraTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('d'))
	{
		currPos += m_pCameraTransform->GetRightDirection() * Timer::GetFrameDeltaTime();
		m_pCameraTransform->SetPosition(currPos);
	}

	Vector2 cursorPos = InputSystem::GetCursorPosition();

	if (InputSystem::GetMousePress(1))
	{		
		Vector2 rotation = (m_prevCursorPosition - cursorPos) * Timer::GetFrameDeltaTime() * m_cameraMoveSpeed;

		Vector3 currRot = m_pCameraTransform->GetRotation();
		Vector3 newRot = currRot + Vector3(rotation.y, -rotation.x, 0);
		// Clamp pitch value to prevent screen getting flipped
		newRot.x = fmin(89.0f, fmax(newRot.x, -89.0f));

		m_pCameraTransform->SetRotation(newRot);
	}

	m_prevCursorPosition = cursorPos;
}