#include "GLCDrawingSystem.h"
#include "GLCComponent.h"

using namespace Engine;

GLCDrawingSystem::GLCDrawingSystem(ECSWorld* pWorld)
	: m_pECSWorld(pWorld)
{

}

void GLCDrawingSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t GLCDrawingSystem::GetSystemID() const
{
	return m_systemID;
}

void GLCDrawingSystem::Initialize()
{

}

void GLCDrawingSystem::ShutDown()
{

}

void GLCDrawingSystem::FrameBegin()
{

}

void GLCDrawingSystem::Tick()
{
	auto pGLCEntity = m_pECSWorld->FindEntityWithTag(eEntityTag_MainGLC);
	if (pGLCEntity)
	{
		auto pGLCComp = std::static_pointer_cast<GLCComponent>(pGLCEntity->GetComponent(eCompType_GLC));
		if (pGLCComp)
		{

		}
	}
}

void GLCDrawingSystem::FrameEnd()
{

}