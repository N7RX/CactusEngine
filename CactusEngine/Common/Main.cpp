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
#include "InputSystem.h"
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
	gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->SetVSync(true);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	// Alert: the registration sequence corresponds to execution sequence
	pWorld->RegisterSystem<InputSystem>(eSystem_Input);
	pWorld->RegisterSystem<AnimationSystem>(eSystem_Animation);
	pWorld->RegisterSystem<DrawingSystem>(eSystem_Drawing);

	// Camera

	auto pCameraComponent = pWorld->CreateComponent<CameraComponent>();
	pCameraComponent->SetClearColor(Color4(0.2f, 0.2f, 0.3f, 1.0f));

	auto pCameraTransformComp = pWorld->CreateComponent<TransformComponent>();
	pCameraTransformComp->SetPosition(Vector3(0, 2, 6.5f));
	pCameraTransformComp->SetRotation(Vector3(-30, 0, 0));

	auto pCamera = pWorld->CreateEntity<StandardEntity>();
	pCamera->AttachComponent(pCameraComponent);
	pCamera->AttachComponent(pCameraTransformComp);
	pCamera->SetEntityTag(eEntityTag_MainCamera);

	// Cube

	auto pCubeMesh = std::make_shared<ObjMesh>("Assets/Models/Cube.obj");
	auto pDefaultTexture = std::make_shared<ImageTexture>("Assets/Textures/Default.png");

	auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
	pTransformComp->SetPosition(Vector3(4.0f, 1.0f, 0));

	auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp->SetMesh(pCubeMesh);

	auto pMaterialComp = std::make_shared<MaterialComponent>();
	pMaterialComp->SetAlbedoColor(Color4(1.0f, 1, 0.3f, 1));
	pMaterialComp->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

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
	pTransformComp2->SetPosition(Vector3(-4.0f, 1.0f, 0));

	auto pMeshFilterComp2 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp2->SetMesh(pCubeMesh);

	auto pMaterialComp2 = std::make_shared<MaterialComponent>();
	pMaterialComp2->SetAlbedoColor(Color4(0.3f, 0.3f, 1.0f, 1));
	pMaterialComp2->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp2->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMeshRendererComp2 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp2->SetRenderer(eRenderer_Forward);

	auto pCube2 = pWorld->CreateEntity<StandardEntity>();
	pCube2->AttachComponent(pTransformComp2);
	pCube2->AttachComponent(pMeshFilterComp2);
	pCube2->AttachComponent(pMaterialComp2);
	pCube2->AttachComponent(pMeshRendererComp2);

	// Cube 3

	auto pTransformComp3 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp3->SetPosition(Vector3(-2.0f, 1.0f, 0));

	auto pMeshFilterComp3 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp3->SetMesh(pCubeMesh);

	auto pMaterialComp3 = std::make_shared<MaterialComponent>();
	pMaterialComp3->SetAlbedoColor(Color4(1.0f, 0.3, 0.3f, 1));
	pMaterialComp3->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp3->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMeshRendererComp3 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp3->SetRenderer(eRenderer_Forward);

	auto pAnimationComp3 = std::make_shared<AnimationComponent>();
	pAnimationComp3->SetAnimFunction(
		[](IEntity* pEntity)
	{
		static float startTime = Timer::Now();
		float deltaTime = Timer::Now() - startTime;
		auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
		Vector3 currScale = abs(sinf(deltaTime * 2.0f)) * Vector3(1, 1, 1);
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
	pTransformComp4->SetPosition(Vector3(2.0f, 1.0f, 0));

	auto pMeshFilterComp4 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp4->SetMesh(pCubeMesh);

	auto pMaterialComp4 = std::make_shared<MaterialComponent>();
	pMaterialComp4->SetAlbedoColor(Color4(0.3f, 1, 0.3f, 1));
	pMaterialComp4->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp4->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

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

	// Water Plane

	auto pPlaneMesh = std::make_shared<Plane>(63, 63);
	auto pWaterTexture = std::make_shared<ImageTexture>("Assets/Textures/Water_Color_1.jpg");
	auto pNoiseTexture = std::make_shared<ImageTexture>("Assets/Textures/Noise_1.png");

	auto pTransformComp5 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp5->SetPosition(Vector3(2.5f, -1.1f, 0));
	pTransformComp5->SetScale(Vector3(5, 5, 1));
	pTransformComp5->SetRotation(Vector3(-90, 0, 180));

	auto pMeshFilterComp5 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp5->SetMesh(pPlaneMesh);

	auto pMaterialComp5 = std::make_shared<MaterialComponent>();
	pMaterialComp5->SetAlbedoColor(Color4(1.0f, 0.7f, 0.65f, 0.85f));
	pMaterialComp5->SetShaderProgram(eShaderProgram_WaterBasic);
	pMaterialComp5->SetTexture(eMaterialTexture_Albedo, pWaterTexture);
	pMaterialComp5->SetTexture(eMaterialTexture_Noise, pNoiseTexture);
	pMaterialComp5->SetTransparent(true);

	auto pMeshRendererComp5 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp5->SetRenderer(eRenderer_Forward);

	auto pPlane = pWorld->CreateEntity<StandardEntity>();
	pPlane->AttachComponent(pTransformComp5);
	pPlane->AttachComponent(pMeshFilterComp5);
	pPlane->AttachComponent(pMaterialComp5);
	pPlane->AttachComponent(pMeshRendererComp5);

	// Large island

	auto pRockMesh = std::make_shared<ObjMesh>("Assets/Models/Rock_1.obj");
	auto pRockTexture = std::make_shared<ImageTexture>("Assets/Textures/Rock_1.tga");

	auto pTransformComp6 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp6->SetPosition(Vector3(0, -1.4f, 2.4f));
	pTransformComp6->SetScale(Vector3(0.007f, 0.007f, 0.004f));
	pTransformComp6->SetRotation(Vector3(-90, 0, 0));

	auto pMeshFilterComp6 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp6->SetMesh(pRockMesh);

	auto pMaterialComp6 = std::make_shared<MaterialComponent>();
	pMaterialComp6->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterialComp6->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp6->SetTexture(eMaterialTexture_Albedo, pRockTexture);

	auto pMeshRendererComp6 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp6->SetRenderer(eRenderer_Forward);

	auto pRock = pWorld->CreateEntity<StandardEntity>();
	pRock->AttachComponent(pTransformComp6);
	pRock->AttachComponent(pMeshFilterComp6);
	pRock->AttachComponent(pMaterialComp6);
	pRock->AttachComponent(pMeshRendererComp6);

	// Bottom

	auto pPlaneMesh2 = std::make_shared<Plane>(1, 1);
	auto pTexture2 = std::make_shared<ImageTexture>("Assets/Textures/Worktops_3_Low.jpg");

	auto pTransformComp7 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp7->SetPosition(Vector3(2.5f, -1.3f, 0));
	pTransformComp7->SetScale(Vector3(5, 5, 1));
	pTransformComp7->SetRotation(Vector3(-90, 0, 180));

	auto pMeshFilterComp7 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp7->SetMesh(pPlaneMesh2);

	auto pMaterialComp7 = std::make_shared<MaterialComponent>();
	pMaterialComp7->SetAlbedoColor(Color4(1.0f, 1.0f, 0.8f, 1.0f));
	pMaterialComp7->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp7->SetTexture(eMaterialTexture_Albedo, pTexture2);

	auto pMeshRendererComp7 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp7->SetRenderer(eRenderer_Forward);

	auto pPlane2 = pWorld->CreateEntity<StandardEntity>();
	pPlane2->AttachComponent(pTransformComp7);
	pPlane2->AttachComponent(pMeshFilterComp7);
	pPlane2->AttachComponent(pMaterialComp7);
	pPlane2->AttachComponent(pMeshRendererComp7);

	// Small island

	auto pTransformComp8 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp8->SetPosition(Vector3(-1.5f, -1.9f, 3.4f));
	pTransformComp8->SetScale(Vector3(0.007f, 0.007f, 0.004f));
	pTransformComp8->SetRotation(Vector3(-90, 0, 45));

	auto pMeshFilterComp8 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp8->SetMesh(pRockMesh);

	auto pMaterialComp8 = std::make_shared<MaterialComponent>();
	pMaterialComp8->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterialComp8->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp8->SetTexture(eMaterialTexture_Albedo, pRockTexture);

	auto pMeshRendererComp8 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp8->SetRenderer(eRenderer_Forward);

	auto pRock2 = pWorld->CreateEntity<StandardEntity>();
	pRock2->AttachComponent(pTransformComp8);
	pRock2->AttachComponent(pMeshFilterComp8);
	pRock2->AttachComponent(pMaterialComp8);
	pRock2->AttachComponent(pMeshRendererComp8);

	// Fish

	auto pFishMesh = std::make_shared<ObjMesh>("Assets/Models/Amago_0.obj");
	auto pFishTexture = std::make_shared<ImageTexture>("Assets/Textures/Amago_0.bmp");

	for (int count = 0; count < 3; ++count)
	{
		auto pTransformComp9 = pWorld->CreateComponent<TransformComponent>();
		pTransformComp9->SetPosition(Vector3(1.8f, -1.25f, 2.8f + 0.4f * count));
		pTransformComp9->SetScale(Vector3(1.f, 1.f, 1.f));
		pTransformComp9->SetRotation(Vector3(0, 20, 0));

		auto pMeshFilterComp9 = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp9->SetMesh(pFishMesh);

		auto pMaterialComp9 = std::make_shared<MaterialComponent>();
		pMaterialComp9->SetAlbedoColor(Color4(0.8f, 1.0f, 0.8f, 1));
		pMaterialComp9->SetShaderProgram(eShaderProgram_Basic);
		pMaterialComp9->SetTexture(eMaterialTexture_Albedo, pFishTexture);

		auto pMeshRendererComp9 = std::make_shared<MeshRendererComponent>();
		pMeshRendererComp9->SetRenderer(eRenderer_Forward);

		auto pFish = pWorld->CreateEntity<StandardEntity>();
		pFish->AttachComponent(pTransformComp9);
		pFish->AttachComponent(pMeshFilterComp9);
		pFish->AttachComponent(pMaterialComp9);
		pFish->AttachComponent(pMeshRendererComp9);
	}

	// Unity Chan

	auto pCharMesh = std::make_shared<ObjMesh>("Assets/Models/UnityChan.obj");

	auto pTransformComp10 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp10->SetPosition(Vector3(0, -0.5f, 2.5));
	pTransformComp10->SetScale(Vector3(0.02f, 0.02f, 0.02f));
	pTransformComp10->SetRotation(Vector3(0, 0, 0));

	auto pMeshFilterComp10 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp10->SetMesh(pCharMesh);

	auto pMaterialComp10 = std::make_shared<MaterialComponent>();
	pMaterialComp10->SetAlbedoColor(Color4(1.0f, 0.8f, 0.8f, 1));
	pMaterialComp10->SetShaderProgram(eShaderProgram_Basic);
	pMaterialComp10->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMeshRendererComp10 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp10->SetRenderer(eRenderer_Forward);

	auto pUnityChan = pWorld->CreateEntity<StandardEntity>();
	pUnityChan->AttachComponent(pTransformComp10);
	pUnityChan->AttachComponent(pMeshFilterComp10);
	pUnityChan->AttachComponent(pMaterialComp10);
	pUnityChan->AttachComponent(pMeshRendererComp10);
}