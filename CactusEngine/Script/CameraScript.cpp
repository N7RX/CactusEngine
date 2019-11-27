#include "CameraScript.h"
#include "InputSystem.h"
#include "Timer.h"
#include <assert.h>
#include <iostream>

using namespace SampleScript;
using namespace Engine;

CameraScript::CameraScript(const std::shared_ptr<Engine::IEntity> pEntity)
	: m_pEntity(pEntity)
{
	assert(pEntity != nullptr);
}

void CameraScript::Start()
{

}

void CameraScript::Update()
{
	if (InputSystem::GetKeyPress('w'))
	{		
		auto pTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos += pTransform->GetForwardDirection() * Timer::GetFrameDeltaTime();
		pTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('s'))
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos -= pTransform->GetForwardDirection() * Timer::GetFrameDeltaTime();
		pTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('a'))
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos -= pTransform->GetRightDirection() * Timer::GetFrameDeltaTime();
		pTransform->SetPosition(currPos);
	}
	if (InputSystem::GetKeyPress('d'))
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos += pTransform->GetRightDirection() * Timer::GetFrameDeltaTime();
		pTransform->SetPosition(currPos);
	}

	Vector2 cursorPos = InputSystem::GetCursorPosition();
	if (InputSystem::GetMousePress(1))
	{		
		Vector2 rotation = (m_prevCursorPosition - cursorPos) * Timer::GetFrameDeltaTime() * 5.0f;

		auto pTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(eCompType_Transform));
		Vector3 currRot = pTransform->GetRotation(); 
		Vector3 newRot = currRot + Vector3(rotation.y, -rotation.x, 0);
		// Clamp pitch value to prevent screen getting flipped
		newRot.x = fmin(89.0f, fmax(newRot.x, -89.0f));

		pTransform->SetRotation(newRot);
	}
	m_prevCursorPosition = cursorPos;
}