#include "GraphicsApplication.h"
#include "Global.h"
#include "DrawingSystem.h"
#include "AnimationSystem.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MeshRendererComponent.h"
#include "AnimationComponent.h"
#include "CameraComponent.h"
#include "MaterialComponent.h"
#include "StandardEntity.h"
#include "ObjMesh.h"
#include "Timer.h"
#include "Plane.h"
#include "ImageTexture.h"
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
	auto pDefaultTexture = std::make_shared<ImageTexture>("Assets/Textures/Default.png");

	auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
	pTransformComp->SetPosition(Vector3(2.0f, -1.5f, 0));

	auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp->SetMesh(pCubeMesh);

	auto pMaterialComp = std::make_shared<MaterialComponent>();
	pMaterialComp->SetAlbedoColor(Color4(1.0f, 1, 0.3f, 1));
	pMaterialComp->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp->SetAlbedoTexture(pDefaultTexture);

	auto pMeshRendererComp = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp->SetRenderer(eRenderer_Forward);

	auto pAnimationComp = std::make_shared<AnimationComponent>();
	pAnimationComp->SetAnimFunction(
		[](IEntity* pEntity)
	{
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currRotation = pTransform->GetRotation();
		currRotation.y += Timer::GetFrameDeltaTime() * 200.0f;
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
	pMaterialComp2->SetAlbedoTexture(pDefaultTexture);

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
	pMaterialComp3->SetAlbedoTexture(pDefaultTexture);

	auto pMeshRendererComp3 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp3->SetRenderer(eRenderer_Forward);

	auto pAnimationComp3 = std::make_shared<AnimationComponent>();
	pAnimationComp3->SetAnimFunction(
		[](IEntity* pEntity)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currScale = sinf(deltaTime * 2.0f) * Vector3(1, 1, 1);
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
	pMaterialComp4->SetAlbedoTexture(pDefaultTexture);

	auto pMeshRendererComp4 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp4->SetRenderer(eRenderer_Forward);

	auto pAnimationComp4 = std::make_shared<AnimationComponent>();
	pAnimationComp4->SetAnimFunction(
		[](IEntity* pEntity)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currPos = pTransform->GetPosition();
		currPos.x = sinf(deltaTime * 2.0f) + 1.5f;
		pTransform->SetPosition(currPos);
	});

	auto pCube4 = pWorld->CreateEntity<StandardEntity>();
	pCube4->AttachComponent(pTransformComp4);
	pCube4->AttachComponent(pMeshFilterComp4);
	pCube4->AttachComponent(pMaterialComp4);
	pCube4->AttachComponent(pMeshRendererComp4);
	pCube4->AttachComponent(pAnimationComp4);

	// Plane

	auto pPlaneMesh = std::make_shared<Plane>(31, 31);
	auto pTexture = std::make_shared<ImageTexture>("Assets/Textures/Statue.jpg");

	auto pTransformComp5 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp5->SetPosition(Vector3(1.5f, 1.5f, 0));
	pTransformComp5->SetScale(Vector3(3, 3, 1));
	pTransformComp5->SetRotation(Vector3(0, 0, 180));

	auto pMeshFilterComp5 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp5->SetMesh(pPlaneMesh);

	auto pMaterialComp5 = std::make_shared<MaterialComponent>();
	pMaterialComp5->SetAlbedoColor(Color4(1.0f, 1, 1.0f, 1));
	pMaterialComp5->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp5->SetAlbedoTexture(pTexture);

	auto pMeshRendererComp5 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp5->SetRenderer(eRenderer_Forward);

	auto pPlane = pWorld->CreateEntity<StandardEntity>();
	pPlane->AttachComponent(pTransformComp5);
	pPlane->AttachComponent(pMeshFilterComp5);
	pPlane->AttachComponent(pMaterialComp5);
	pPlane->AttachComponent(pMeshRendererComp5);
}