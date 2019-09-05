#include "GraphicsApplication.h"
#include "Global.h"
#include "DrawingSystem.h"
#include "AnimationSystem.h"
#include "GLCDrawingSystem.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MeshRendererComponent.h"
#include "AnimationComponent.h"
#include "CameraComponent.h"
#include "MaterialComponent.h"
#include "GLCComponent.h"
#include "StandardEntity.h"
#include "ObjMesh.h"
#include "GLCMesh.h"
#include <iostream>

// This is the entry of the program

using namespace Engine;

void ConfigSetup();
void TestSetup(GraphicsApplication* pApp);

int main()
{
	if (gpGlobal == nullptr)
	{
		gpGlobal = new Global();
	}

	auto pApplication = std::make_shared<GraphicsApplication>();

	if (!pApplication)
	{
		return -1;
	}

	pApplication->SetApplicationID(0);
	pApplication->AddSetupFunction(TestSetup);

	gpGlobal->SetApplication(pApplication);

	ConfigSetup();

	try
	{
		pApplication->Initialize();
	}
	catch (const std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}

	while (!pApplication->ShouldQuit())
	{
		try
		{
			pApplication->Tick();
		}
		catch (const std::runtime_error& e)
		{
			std::cout << e.what() << std::endl;
			return -1;
		}
	}

	pApplication->ShutDown();

	return 0;
}

void ConfigSetup()
{
	gpGlobal->CreateConfiguration<AppConfiguration>(eConfiguration_App);
	gpGlobal->CreateConfiguration<GraphicsConfiguration>(eConfiguration_Graphics);

	gpGlobal->GetConfiguration<AppConfiguration>(eConfiguration_App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->SetDeviceType(eDevice_OpenGL);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->SetWindowSize(1280, 720);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	pWorld->RegisterSystem<AnimationSystem>(eSystem_Animation);
	pWorld->RegisterSystem<GLCDrawingSystem>(eSystem_GLCDrawing);
	pWorld->RegisterSystem<DrawingSystem>(eSystem_Drawing);

	// Camera

	auto pCameraComponent = pWorld->CreateComponent<CameraComponent>();
	pCameraComponent->SetClearColor(Color4(0.2f, 0.2f, 0.3f, 1.0f));

	auto pCameraTransformComp = pWorld->CreateComponent<TransformComponent>();
	pCameraTransformComp->SetPosition(Vector3(0, 0, 5));

	auto pCamera = pWorld->CreateEntity<StandardEntity>();
	pCamera->AttachComponent(pCameraComponent);
	pCamera->AttachComponent(pCameraTransformComp);
	pCamera->SetEntityTag(eEntityTag_MainCamera);

	// Cube

	auto pCubeMesh = std::make_shared<ObjMesh>("Assets/Models/Cube.obj");

	auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
	pTransformComp->SetPosition(Vector3(2.0f, -1.5f, 0));

	auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp->SetMesh(pCubeMesh);

	auto pMaterialComp = std::make_shared<MaterialComponent>();
	pMaterialComp->SetAlbedoColor(Color4(1.0f, 1, 0.3f, 1));
	pMaterialComp->SetShaderProgram(eShaderProgram_Basic);

	auto pMeshRendererComp = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp->SetRenderer(eRenderer_Forward);

	auto pAnimationComp = std::make_shared<AnimationComponent>();
	pAnimationComp->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currRotation = pTransform->GetRotation();
		currRotation.y += 1.f;
		if (currRotation.y >= 360)
		{
			currRotation.y = 0;
		}
		pTransform->SetRotation(currRotation);
	});

	auto pCube = pWorld->CreateEntity<StandardEntity>();
	pCube->AttachComponent(pTransformComp);
	pCube->AttachComponent(pMeshFilterComp);
	pCube->AttachComponent(pMaterialComp);
	pCube->AttachComponent(pMeshRendererComp);
	pCube->AttachComponent(pAnimationComp);

	// Cube 2

	auto pTransformComp2 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp2->SetPosition(Vector3(-2.0f, -1.5f, 0));

	auto pMeshFilterComp2 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp2->SetMesh(pCubeMesh);

	auto pMaterialComp2 = std::make_shared<MaterialComponent>();
	pMaterialComp2->SetAlbedoColor(Color4(0.3f, 0.3f, 1.0f, 1));
	pMaterialComp2->SetShaderProgram(eShaderProgram_Basic);

	auto pMeshRendererComp2 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp2->SetRenderer(eRenderer_Forward);

	auto pCube2 = pWorld->CreateEntity<StandardEntity>();
	pCube2->AttachComponent(pTransformComp2);
	pCube2->AttachComponent(pMeshFilterComp2);
	pCube2->AttachComponent(pMaterialComp2);
	pCube2->AttachComponent(pMeshRendererComp2);

	// Cube 3

	auto pTransformComp3 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp3->SetPosition(Vector3(-2.0f, 1.5f, 0));

	auto pMeshFilterComp3 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp3->SetMesh(pCubeMesh);

	auto pMaterialComp3 = std::make_shared<MaterialComponent>();
	pMaterialComp3->SetAlbedoColor(Color4(1.0f, 0.3, 0.3f, 1));
	pMaterialComp3->SetShaderProgram(eShaderProgram_Basic);

	auto pMeshRendererComp3 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp3->SetRenderer(eRenderer_Forward);

	auto pAnimationComp3 = std::make_shared<AnimationComponent>();
	pAnimationComp3->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currScale = sinf(std::chrono::high_resolution_clock::now().time_since_epoch().count() * 0.01f) * Vector3(1, 1, 1);
		pTransform->SetScale(currScale);
	});

	auto pCube3 = pWorld->CreateEntity<StandardEntity>();
	pCube3->AttachComponent(pTransformComp3);
	pCube3->AttachComponent(pMeshFilterComp3);
	pCube3->AttachComponent(pMaterialComp3);
	pCube3->AttachComponent(pMeshRendererComp3);
	pCube3->AttachComponent(pAnimationComp3);

	// Cube 4

	auto pTransformComp4 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp4->SetPosition(Vector3(2.0f, 1.5f, 0));

	auto pMeshFilterComp4 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp4->SetMesh(pCubeMesh);

	auto pMaterialComp4 = std::make_shared<MaterialComponent>();
	pMaterialComp4->SetAlbedoColor(Color4(0.3f, 1, 0.3f, 1));
	pMaterialComp4->SetShaderProgram(eShaderProgram_Basic);

	auto pMeshRendererComp4 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp4->SetRenderer(eRenderer_Forward);

	auto pAnimationComp4 = std::make_shared<AnimationComponent>();
	pAnimationComp4->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos.x = sinf(std::chrono::high_resolution_clock::now().time_since_epoch().count() * 0.01f) + 1.5f;
		pTransform->SetPosition(currPos);
	});

	auto pCube4 = pWorld->CreateEntity<StandardEntity>();
	pCube4->AttachComponent(pTransformComp4);
	pCube4->AttachComponent(pMeshFilterComp4);
	pCube4->AttachComponent(pMaterialComp4);
	pCube4->AttachComponent(pMeshRendererComp4);
	pCube4->AttachComponent(pAnimationComp4);

	// General Linera Camera

	GLCDrawingSystem::m_sRenderResult = std::make_shared<RenderTexture>(256, 256, std::static_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice());

	auto pGLCComponent = pWorld->CreateComponent<GLCComponent>();

	auto pGLC = pWorld->CreateEntity<StandardEntity>();
	pGLC->AttachComponent(pGLCComponent);
	pGLC->SetEntityTag(eEntityTag_MainGLC);

	// GLC Cube

	auto pGLCCubeMesh = std::make_shared<GLCMesh>("Assets/Models/Cube.obj");

	auto pTransformCompGLC = pWorld->CreateComponent<TransformComponent>();
	pTransformCompGLC->SetPosition(Vector3(0.5f, 0.5f, 1.5f));
	pTransformCompGLC->SetScale(Vector3(0.25f, 0.25f, 0.25f));

	auto pMeshFilterCompGLC = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterCompGLC->SetMesh(pGLCCubeMesh);

	auto pMaterialCompGLC = std::make_shared<MaterialComponent>();
	pMaterialCompGLC->SetAlbedoColor(Color4(1.0f, 0.7f, 0.7f, 1));

	auto pAnimationCompGLC = std::make_shared<AnimationComponent>();
	pAnimationCompGLC->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currScale = sinf(std::chrono::high_resolution_clock::now().time_since_epoch().count() * 0.005f) * Vector3(0.25f, 0.25f, 0.25f);
		pTransform->SetScale(currScale);
	});

	auto pGLCCube = pWorld->CreateEntity<StandardEntity>();
	pGLCCube->AttachComponent(pTransformCompGLC);
	pGLCCube->AttachComponent(pMeshFilterCompGLC);
	pGLCCube->AttachComponent(pMaterialCompGLC);
	pGLCCube->AttachComponent(pAnimationCompGLC);
	pGLCCube->SetEntityTag(eEntityTag_GLCMesh);

	// GLC Cube 2

	auto pTransformCompGLC2 = pWorld->CreateComponent<TransformComponent>();
	pTransformCompGLC2->SetPosition(Vector3(0.75f, 0.75f, 1.5f));
	pTransformCompGLC2->SetRotation(Vector3(0, 0.0f, 0));
	pTransformCompGLC2->SetScale(Vector3(0.25f, 0.25f, 0.25f));

	auto pMeshFilterCompGLC2 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterCompGLC2->SetMesh(pGLCCubeMesh);

	auto pMaterialCompGLC2 = std::make_shared<MaterialComponent>();
	pMaterialCompGLC2->SetAlbedoColor(Color4(0.7f, 0.7f, 1.0f, 1));

	auto pAnimationCompGLC2 = std::make_shared<AnimationComponent>();
	pAnimationCompGLC2->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pMaterial = std::static_pointer_cast<MaterialComponent>(pEntity->GetComponent(eCompType_Material));
		Color3 currColor = abs(sinf(std::chrono::high_resolution_clock::now().time_since_epoch().count() * 0.01f)) * Color3(0.7f, 0.7f, 1.0f);
		pMaterial->SetAlbedoColor(Color4(currColor, 1.0f));
	});

	auto pGLCCube2 = pWorld->CreateEntity<StandardEntity>();
	pGLCCube2->AttachComponent(pTransformCompGLC2);
	pGLCCube2->AttachComponent(pMeshFilterCompGLC2);
	pGLCCube2->AttachComponent(pMaterialCompGLC2);
	pGLCCube2->AttachComponent(pAnimationCompGLC2);
	pGLCCube2->SetEntityTag(eEntityTag_GLCMesh);

	// GLC display cube (Cube 5)

	auto pTransformComp5 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp5->SetPosition(Vector3(0, 0, -1.0f));
	pTransformComp5->SetRotation(Vector3(0, 0, 180));
	pTransformComp5->SetScale(Vector3(6.0f, 6.0f, 0.5f));

	auto pMeshFilterComp5 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp5->SetMesh(pCubeMesh);

	auto pMaterialComp5 = std::make_shared<MaterialComponent>();
	pMaterialComp5->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterialComp5->SetAlbedoTexture(GLCDrawingSystem::m_sRenderResult);
	pMaterialComp5->SetShaderProgram(eShaderProgram_Basic);

	auto pMeshRendererComp5 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp5->SetRenderer(eRenderer_Forward);

	auto pCube5 = pWorld->CreateEntity<StandardEntity>();
	pCube5->AttachComponent(pTransformComp5);
	pCube5->AttachComponent(pMeshFilterComp5);
	pCube5->AttachComponent(pMaterialComp5);
	pCube5->AttachComponent(pMeshRendererComp5);
}