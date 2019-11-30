#include <iostream>
#include "GraphicsApplication.h"
#include "Global.h"
#include "DrawingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "AllComponents.h"
#include "StandardEntity.h"
#include "ExternalMesh.h"
#include "Timer.h"
#include "Plane.h"
#include "ImageTexture.h"
#include "CameraScript.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"

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
	pWorld->RegisterSystem<DrawingSystem>(eSystem_Script);
	pWorld->RegisterSystem<ScriptSystem>(eSystem_Drawing);

	// Read test
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/TestScene.json");
	//return;

	// Camera

	auto pCameraComponent = pWorld->CreateComponent<CameraComponent>();
	pCameraComponent->SetClearColor(Color4(0.2f, 0.2f, 0.3f, 1.0f));

	auto pCameraTransformComp = pWorld->CreateComponent<TransformComponent>();
	pCameraTransformComp->SetPosition(Vector3(0, 1.5f, 6.5f));
	pCameraTransformComp->SetRotation(Vector3(-17, -90, 0));

	auto pCameraScriptComp = pWorld->CreateComponent<ScriptComponent>();

	auto pCamera = pWorld->CreateEntity<StandardEntity>();
	pCamera->AttachComponent(pCameraComponent);
	pCamera->AttachComponent(pCameraTransformComp);
	pCamera->AttachComponent(pCameraScriptComp);
	pCamera->SetEntityTag(eEntityTag_MainCamera);

	pCameraScriptComp->BindScript(std::make_shared<SampleScript::CameraScript>(pCamera));

	// Cube

	auto pCubeMesh = std::make_shared<ExternalMesh>("Assets/Models/Cube.obj");
	auto pDefaultTexture = std::make_shared<ImageTexture>("Assets/Textures/Default.png");

	auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
	pTransformComp->SetPosition(Vector3(4.0f, 1.0f, 0));

	auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp->SetMesh(pCubeMesh);

	auto pMaterial = std::make_shared<Material>();
	pMaterial->SetAlbedoColor(Color4(1.0f, 1, 0.3f, 1));
	pMaterial->SetShaderProgram(eShaderProgram_Basic);
	pMaterial->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMaterialComp = std::make_shared<MaterialComponent>();
	pMaterialComp->AddMaterial(0, pMaterial);

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

	auto pMaterial2 = std::make_shared<Material>();
	
	pMaterial2->SetAlbedoColor(Color4(0.3f, 0.3f, 1.0f, 1));
	pMaterial2->SetShaderProgram(eShaderProgram_Basic);
	pMaterial2->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMaterialComp2 = std::make_shared<MaterialComponent>();
	pMaterialComp2->AddMaterial(0, pMaterial2);

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

	auto pMaterial3 = std::make_shared<Material>();
	pMaterial3->SetAlbedoColor(Color4(1.0f, 0.3, 0.3f, 1));
	pMaterial3->SetShaderProgram(eShaderProgram_Basic);
	pMaterial3->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMaterialComp3 = std::make_shared<MaterialComponent>();
	pMaterialComp3->AddMaterial(0, pMaterial3);

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

	auto pMaterial4 = std::make_shared<Material>();
	pMaterial4->SetAlbedoColor(Color4(0.3f, 1, 0.3f, 1));
	pMaterial4->SetShaderProgram(eShaderProgram_Basic);
	pMaterial4->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

	auto pMaterialComp4 = std::make_shared<MaterialComponent>();
	pMaterialComp4->AddMaterial(0, pMaterial4);

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

	auto pMaterial5 = std::make_shared<Material>();
	pMaterial5->SetAlbedoColor(Color4(1.0f, 0.7f, 0.65f, 0.85f));
	pMaterial5->SetShaderProgram(eShaderProgram_WaterBasic);
	pMaterial5->SetTexture(eMaterialTexture_Albedo, pWaterTexture);
	pMaterial5->SetTexture(eMaterialTexture_Noise, pNoiseTexture);
	pMaterial5->SetTransparent(true);

	auto pMaterialComp5 = std::make_shared<MaterialComponent>();
	pMaterialComp5->AddMaterial(0, pMaterial5);

	auto pMeshRendererComp5 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp5->SetRenderer(eRenderer_Forward);

	auto pPlane = pWorld->CreateEntity<StandardEntity>();
	pPlane->AttachComponent(pTransformComp5);
	pPlane->AttachComponent(pMeshFilterComp5);
	pPlane->AttachComponent(pMaterialComp5);
	pPlane->AttachComponent(pMeshRendererComp5);

	// Large island

	auto pRockMesh = std::make_shared<ExternalMesh>("Assets/Models/Rock_1.obj");
	auto pRockTexture = std::make_shared<ImageTexture>("Assets/Textures/Rock_1.tga");

	auto pTransformComp6 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp6->SetPosition(Vector3(0, -1.4f, 2.4f));
	pTransformComp6->SetScale(Vector3(0.007f, 0.007f, 0.004f));
	pTransformComp6->SetRotation(Vector3(-90, 0, 0));

	auto pMeshFilterComp6 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp6->SetMesh(pRockMesh);

	auto pMaterial6 = std::make_shared<Material>();
	pMaterial6->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterial6->SetShaderProgram(eShaderProgram_Basic);
	pMaterial6->SetTexture(eMaterialTexture_Albedo, pRockTexture);

	auto pMaterialComp6 = std::make_shared<MaterialComponent>();
	pMaterialComp6->AddMaterial(0, pMaterial6);

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

	auto pMaterial7 = std::make_shared<Material>();
	pMaterial7->SetAlbedoColor(Color4(1.0f, 1.0f, 0.8f, 1.0f));
	pMaterial7->SetShaderProgram(eShaderProgram_Basic);
	pMaterial7->SetTexture(eMaterialTexture_Albedo, pTexture2);

	auto pMaterialComp7 = std::make_shared<MaterialComponent>();
	pMaterialComp7->AddMaterial(0, pMaterial7);

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

	auto pMaterial8 = std::make_shared<Material>();
	pMaterial8->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterial8->SetShaderProgram(eShaderProgram_Basic);
	pMaterial8->SetTexture(eMaterialTexture_Albedo, pRockTexture);

	auto pMaterialComp8 = std::make_shared<MaterialComponent>();
	pMaterialComp8->AddMaterial(0, pMaterial8);

	auto pMeshRendererComp8 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp8->SetRenderer(eRenderer_Forward);

	auto pRock2 = pWorld->CreateEntity<StandardEntity>();
	pRock2->AttachComponent(pTransformComp8);
	pRock2->AttachComponent(pMeshFilterComp8);
	pRock2->AttachComponent(pMaterialComp8);
	pRock2->AttachComponent(pMeshRendererComp8);

	// Fish

	auto pFishMesh = std::make_shared<ExternalMesh>("Assets/Models/Amago_0.obj");
	auto pFishTexture = std::make_shared<ImageTexture>("Assets/Textures/Amago_0.bmp");

	auto pMaterial9 = std::make_shared<Material>();
	pMaterial9->SetAlbedoColor(Color4(0.8f, 1.0f, 0.8f, 1));
	pMaterial9->SetShaderProgram(eShaderProgram_Basic);
	pMaterial9->SetTexture(eMaterialTexture_Albedo, pFishTexture);

	for (int count = 0; count < 3; ++count)
	{
		auto pTransformComp9 = pWorld->CreateComponent<TransformComponent>();
		pTransformComp9->SetPosition(Vector3(1.8f, -1.25f, 2.8f + 0.4f * count));
		pTransformComp9->SetScale(Vector3(1.f, 1.f, 1.f));
		pTransformComp9->SetRotation(Vector3(0, 20, 0));

		auto pMeshFilterComp9 = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp9->SetMesh(pFishMesh);

		auto pMaterialComp9 = std::make_shared<MaterialComponent>();
		pMaterialComp9->AddMaterial(0, pMaterial9);

		auto pMeshRendererComp9 = std::make_shared<MeshRendererComponent>();
		pMeshRendererComp9->SetRenderer(eRenderer_Forward);

		auto pFish = pWorld->CreateEntity<StandardEntity>();
		pFish->AttachComponent(pTransformComp9);
		pFish->AttachComponent(pMeshFilterComp9);
		pFish->AttachComponent(pMaterialComp9);
		pFish->AttachComponent(pMeshRendererComp9);
	}

	// Unity Chan

	auto pCharMesh = std::make_shared<ExternalMesh>("Assets/Models/unitychan_v1.3ds");
	auto pToneTexture = std::make_shared<ImageTexture>("Assets/Textures/XToon_BW.png");

	std::vector<std::shared_ptr<ImageTexture>> pUnityChanTextures =
	{
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/hair_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/hair_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/hair_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/hair_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/hair_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/skin_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/cheek_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eyeline_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/face_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eyeline_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/face_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eye_iris_L_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eye_iris_R_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/face_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
	};

	auto pTransformComp10 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp10->SetPosition(Vector3(0, 0.5f, 5.3f));
	pTransformComp10->SetScale(Vector3(0.02f, 0.02f, 0.02f));
	pTransformComp10->SetRotation(Vector3(0, 0, 0));

	auto pMeshFilterComp10 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp10->SetMesh(pCharMesh);

	unsigned int submeshCount = pCharMesh->GetSubmeshCount();

	auto pMaterialComp10 = std::make_shared<MaterialComponent>();

	for (unsigned int i = 0; i < submeshCount; ++i)
	{
		auto pMaterial10 = std::make_shared<Material>();
		pMaterial10->SetShaderProgram(eShaderProgram_AnimeStyle);
		pMaterial10->SetTexture(eMaterialTexture_Albedo, pUnityChanTextures[i]);
		pMaterial10->SetTexture(eMaterialTexture_Tone, pToneTexture);

		pMaterialComp10->AddMaterial(i, pMaterial10);
	}

	auto pMeshRendererComp10 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp10->SetRenderer(eRenderer_Forward);

	auto pUnityChan = pWorld->CreateEntity<StandardEntity>();
	pUnityChan->AttachComponent(pTransformComp10);
	pUnityChan->AttachComponent(pMeshFilterComp10);
	pUnityChan->AttachComponent(pMaterialComp10);
	pUnityChan->AttachComponent(pMeshRendererComp10);

	// Sponza

	auto pSponzaMesh = std::make_shared<ExternalMesh>("Assets/Models/sponza.obj");

	auto pTransformComp11 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp11->SetPosition(Vector3(0, 0.0f, 0.0f));
	pTransformComp11->SetScale(Vector3(0.007f, 0.007f, 0.007f));

	auto pMeshFilterComp11 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp11->SetMesh(pSponzaMesh);

	submeshCount = pSponzaMesh->GetSubmeshCount();

	auto pMaterialComp11 = std::make_shared<MaterialComponent>();

	for (unsigned int i = 0; i < submeshCount; ++i)
	{
		auto pMaterial11 = std::make_shared<Material>();
		pMaterial11->SetShaderProgram(eShaderProgram_Basic);
		pMaterial11->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);

		pMaterialComp11->AddMaterial(i, pMaterial11);
	}

	auto pMeshRendererComp11 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp11->SetRenderer(eRenderer_Forward);

	auto pSponza = pWorld->CreateEntity<StandardEntity>();
	pSponza->AttachComponent(pTransformComp11);
	pSponza->AttachComponent(pMeshFilterComp11);
	pSponza->AttachComponent(pMaterialComp11);
	pSponza->AttachComponent(pMeshRendererComp11);

	WriteECSWorldToJson(pWorld, "Assets/Scene/TestScene.json");
}