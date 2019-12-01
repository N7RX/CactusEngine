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
	pCameraComponent->SetClearColor(Color4(0.5f, 0.75f, 0.9f, 0.0f));

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
	auto pToneTexture = std::make_shared<ImageTexture>("Assets/Textures/XToon_BW.png");

	auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
	pTransformComp->SetPosition(Vector3(0.25f, 4.0f, -8.2f));

	auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp->SetMesh(pCubeMesh);

	auto pMaterial = std::make_shared<Material>();
	pMaterial->SetAlbedoColor(Color4(1.0f, 1, 0.3f, 1));
	pMaterial->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
	pMaterial->SetTexture(eMaterialTexture_Tone, pToneTexture);

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
	pTransformComp2->SetPosition(Vector3(-2.8f, 1.5f, -8.4f));

	auto pMeshFilterComp2 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp2->SetMesh(pCubeMesh);

	auto pMaterial2 = std::make_shared<Material>();
	
	pMaterial2->SetAlbedoColor(Color4(0.3f, 0.3f, 1.0f, 1));
	pMaterial2->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial2->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
	pMaterial2->SetTexture(eMaterialTexture_Tone, pToneTexture);

	auto pMaterialComp2 = std::make_shared<MaterialComponent>();
	pMaterialComp2->AddMaterial(0, pMaterial2);

	auto pMeshRendererComp2 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp2->SetRenderer(eRenderer_Forward);

	auto pAnimationComp2 = std::make_shared<AnimationComponent>();
	pAnimationComp2->SetAnimFunction(
		[](IEntity* pEntity)
		{
			static float startTime = Timer::Now();
			float deltaTime = Timer::Now() - startTime;
			auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
			Vector3 currScale = abs(sinf(deltaTime * 2.0f)) * Vector3(1, 1, 1);
			pTransform->SetScale(currScale);
		});

	auto pCube2 = pWorld->CreateEntity<StandardEntity>();
	pCube2->AttachComponent(pTransformComp2);
	pCube2->AttachComponent(pMeshFilterComp2);
	pCube2->AttachComponent(pMaterialComp2);
	pCube2->AttachComponent(pAnimationComp2);
	pCube2->AttachComponent(pMeshRendererComp2);

	// Cube 4

	auto pTransformComp4 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp4->SetPosition(Vector3(2.0f, 1.0f, -0.3f));

	auto pMeshFilterComp4 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp4->SetMesh(pCubeMesh);

	auto pMaterial4 = std::make_shared<Material>();
	pMaterial4->SetAlbedoColor(Color4(0.3f, 1, 0.3f, 1));
	pMaterial4->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial4->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
	pMaterial4->SetTexture(eMaterialTexture_Tone, pToneTexture);

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
		currPos.x = 2.5f * sinf(deltaTime * 2.0f) + 0.25f;
		pTransform->SetPosition(currPos);

		auto pMaterial = std::static_pointer_cast<MaterialComponent>(pEntity->GetComponent(eCompType_Material));
		auto pMaterialImpl = pMaterial->GetMaterialBySubmeshIndex(0);
		if (currPos.x < -2.2f)
		{
			pMaterialImpl->SetAlbedoColor(Color4(0.3f, 1.0f, 0.3f, 1));
		}
		if (currPos.x > 2.7f)
		{
			pMaterialImpl->SetAlbedoColor(Color4(1.0f, 0.3f, 0.3f, 1));
		}
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
	pTransformComp5->SetPosition(Vector3(1.3f, 0.2f, -4.2f));
	pTransformComp5->SetScale(Vector3(2.1, 2.1, 1));
	pTransformComp5->SetRotation(Vector3(-90, 0, 180));

	auto pMeshFilterComp5 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp5->SetMesh(pPlaneMesh);

	auto pMaterial5 = std::make_shared<Material>();
	pMaterial5->SetAlbedoColor(Color4(1.0f, 0.7f, 0.65f, 0.7f));
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
	pTransformComp6->SetPosition(Vector3(0.5f, 0.0f, -3.2f));
	pTransformComp6->SetScale(Vector3(0.0028f, 0.0028f, 0.0028f));
	pTransformComp6->SetRotation(Vector3(-90, 0, 0));

	auto pMeshFilterComp6 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp6->SetMesh(pRockMesh);

	auto pMaterial6 = std::make_shared<Material>();
	pMaterial6->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterial6->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial6->SetTexture(eMaterialTexture_Albedo, pRockTexture);
	pMaterial6->SetTexture(eMaterialTexture_Tone, pToneTexture);

	auto pMaterialComp6 = std::make_shared<MaterialComponent>();
	pMaterialComp6->AddMaterial(0, pMaterial6);

	auto pMeshRendererComp6 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp6->SetRenderer(eRenderer_Forward);

	auto pRock = pWorld->CreateEntity<StandardEntity>();
	pRock->AttachComponent(pTransformComp6);
	pRock->AttachComponent(pMeshFilterComp6);
	pRock->AttachComponent(pMaterialComp6);
	pRock->AttachComponent(pMeshRendererComp6);

	// Small island

	auto pTransformComp8 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp8->SetPosition(Vector3(0.0f, -0.1f, -2.9f));
	pTransformComp8->SetScale(Vector3(0.0028f, 0.0028f, 0.0016f));
	pTransformComp8->SetRotation(Vector3(-90, 0, 45));

	auto pMeshFilterComp8 = pWorld->CreateComponent<MeshFilterComponent>();
	pMeshFilterComp8->SetMesh(pRockMesh);

	auto pMaterial8 = std::make_shared<Material>();
	pMaterial8->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterial8->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial8->SetTexture(eMaterialTexture_Tone, pToneTexture);
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
		pTransformComp9->SetPosition(Vector3(-0.15f + 0.4f * count, 0.05f, -3.9f));
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

	auto pCharMesh = std::make_shared<ExternalMesh>("Assets/Models/unitychan_v2.3ds");

	std::vector<std::shared_ptr<ImageTexture>> pUnityChanTextures =
	{
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/skin_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eye_iris_R_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eye_iris_L_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/face_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/face_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eyeline_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/eyeline_00.tga"),
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
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/cheek_00.tga"),
		std::make_shared<ImageTexture>("Assets/Textures/unitychan/body_01.tga"),
	};

	auto pTransformComp10 = pWorld->CreateComponent<TransformComponent>();
	pTransformComp10->SetPosition(Vector3(0.25f, -0.02f, 5.3f));
	pTransformComp10->SetScale(Vector3(0.022f, 0.022f, 0.022f));
	pTransformComp10->SetRotation(Vector3(0, 180, 0));

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

		if (i == 20)
		{
			pMaterial10->SetShaderProgram(eShaderProgram_Basic_Transparent);
			pMaterial10->SetAlbedoColor(Color4(1.0f, 0.2f, 0.3f, 1));
			pMaterial10->SetTransparent(true);
		}
		if (i == 0 || i == 4 || i == 5 || i == 13)
		{
			pMaterial10->SetAlbedoColor(Color4(1.2f, 1, 1, 1));
		}
		if (i >= 8 && i <= 12)
		{
			pMaterial10->SetAnisotropy(-0.5f);
			pMaterial10->SetRoughness(0.2f);
		}

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

	std::vector<std::shared_ptr<ImageTexture>> pSponzaTextures =
	{
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_thorn_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/vase_round.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/vase_plant.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/background.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/spnza_bricks_a_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_arch_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_ceiling_a_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_column_a_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_floor_a_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_column_a_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_details_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_column_b_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/Default.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_flagpole_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_fabric_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_fabric_blue_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_fabric_green_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_curtain_green_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_curtain_blue_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_curtain_diff_blur.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/chain_diff.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/vase_hanging.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/vase_dif.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/lion.png"),
		std::make_shared<ImageTexture>("Assets/Textures/sponza/sponza_roof_diff.png"),
	};

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
		pMaterial11->SetShaderProgram(eShaderProgram_AnimeStyle);
		pMaterial11->SetAlbedoColor(Color4(1.0f, 0.9f, 0.8f, 1.0f));
		pMaterial11->SetTexture(eMaterialTexture_Albedo, pSponzaTextures[i]);
		pMaterial11->SetTexture(eMaterialTexture_Tone, pToneTexture);

		if (i >= 14 && i <= 19)
		{
			pMaterial11->SetAnisotropy(0.5f);
			pMaterial11->SetRoughness(0.3f);
		}

		pMaterialComp11->AddMaterial(i, pMaterial11);
	}

	auto pMeshRendererComp11 = std::make_shared<MeshRendererComponent>();
	pMeshRendererComp11->SetRenderer(eRenderer_Forward);

	auto pSponza = pWorld->CreateEntity<StandardEntity>();
	pSponza->AttachComponent(pTransformComp11);
	pSponza->AttachComponent(pMeshFilterComp11);
	pSponza->AttachComponent(pMaterialComp11);
	pSponza->AttachComponent(pMeshRendererComp11);

	// Water pool

	auto pMaterial12 = std::make_shared<Material>();
	pMaterial12->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1));
	pMaterial12->SetShaderProgram(eShaderProgram_AnimeStyle);
	pMaterial12->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
	pMaterial12->SetTexture(eMaterialTexture_Tone, pToneTexture);

	for (int i = 0; i < 4; ++i)
	{
		auto pMeshFilterComp12 = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp12->SetMesh(pCubeMesh);

		auto pMeshRendererComp12 = std::make_shared<MeshRendererComponent>();
		pMeshRendererComp12->SetRenderer(eRenderer_Forward);

		auto pMaterialComp12 = std::make_shared<MaterialComponent>();
		pMaterialComp12->AddMaterial(0, pMaterial12);

		auto pTransformComp12 = pWorld->CreateComponent<TransformComponent>();
		if (i == 0)
		{
			pTransformComp12->SetPosition(Vector3(-0.875f, 0.15f, -3.2f));
			pTransformComp12->SetScale(Vector3(2.5f, 0.3f, 0.25f));
		}
		if (i == 1)
		{
			pTransformComp12->SetPosition(Vector3(1.375f, 0.15f, -3.2f));
			pTransformComp12->SetScale(Vector3(2.5f, 0.3f, 0.25f));
		}
		if (i == 2)
		{
			pTransformComp12->SetPosition(Vector3(0.25f, 0.15f, -4.325f));
			pTransformComp12->SetScale(Vector3(0.25f, 0.3f, 2.0f));
		}
		if (i == 3)
		{
			pTransformComp12->SetPosition(Vector3(0.25f, 0.15f, -2.075f));
			pTransformComp12->SetScale(Vector3(0.25f, 0.3f, 2.0f));
		}

		auto pCube12 = pWorld->CreateEntity<StandardEntity>();
		pCube12->AttachComponent(pTransformComp12);
		pCube12->AttachComponent(pMeshFilterComp12);
		pCube12->AttachComponent(pMaterialComp12);
		pCube12->AttachComponent(pMeshRendererComp12);
	}

	// Bunny

	auto pBunnyMesh = std::make_shared<ExternalMesh>("Assets/Models/Bunny.obj");

	for (int i = 0; i < 2; ++i)
	{
		auto pTransformComp13 = pWorld->CreateComponent<TransformComponent>();
		pTransformComp13->SetPosition(Vector3(1.5f * i - 0.5f, 0.0f, 3.3f));
		pTransformComp13->SetScale(Vector3(0.3f));
		if (i == 0)
		{
			pTransformComp13->SetRotation(Vector3(0, 90, 0));
		}
		else
		{
			pTransformComp13->SetRotation(Vector3(0, -90, 0));
		}

		auto pMeshFilterComp13 = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp13->SetMesh(pBunnyMesh);

		auto pMaterial13 = std::make_shared<Material>();
		if (i == 0)
		{
			pMaterial13->SetAlbedoColor(Color4(1.0f, 0.3f, 0.3f, 1));
			pMaterial13->SetAnisotropy(-0.5f);
			pMaterial13->SetRoughness(0.2f);
		}
		else
		{
			pMaterial13->SetAlbedoColor(Color4(0.3f, 0.3f, 1.0f, 1));
		}
		pMaterial13->SetShaderProgram(eShaderProgram_AnimeStyle);
		pMaterial13->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
		pMaterial13->SetTexture(eMaterialTexture_Tone, pToneTexture);

		auto pMaterialComp13 = std::make_shared<MaterialComponent>();
		pMaterialComp13->AddMaterial(0, pMaterial13);

		auto pMeshRendererComp13 = std::make_shared<MeshRendererComponent>();
		pMeshRendererComp13->SetRenderer(eRenderer_Forward);

		auto pAnimationComp13 = std::make_shared<AnimationComponent>();
		if (i == 0)
		{
			pAnimationComp13->SetAnimFunction(
				[](IEntity* pEntity)
				{
					static float startTime = Timer::Now();
					float deltaTime = Timer::Now() - startTime;
					auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
					Vector3 currPos = pTransform->GetPosition();
					currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f), 0.0f, 1.0f);
					pTransform->SetPosition(currPos);
				});
		}
		else
		{
			pAnimationComp13->SetAnimFunction(
				[](IEntity* pEntity)
				{
					static float startTime = Timer::Now();
					float deltaTime = Timer::Now() - startTime;
					auto pTransform = std::static_pointer_cast<TransformComponent>(pEntity->GetComponent(eCompType_Transform));
					Vector3 currPos = pTransform->GetPosition();
					currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f + 3.1415926536f), 0.0f, 1.0f);
					pTransform->SetPosition(currPos);
				});
		}

		auto pBunny = pWorld->CreateEntity<StandardEntity>();
		pBunny->AttachComponent(pTransformComp13);
		pBunny->AttachComponent(pMeshFilterComp13);
		pBunny->AttachComponent(pMaterialComp13);
		pBunny->AttachComponent(pMeshRendererComp13);
		pBunny->AttachComponent(pAnimationComp13);
	}

	// Spheres

	auto pSphereMesh = std::make_shared<ExternalMesh>("Assets/Models/sphere.obj");

	for (int i = 0; i < 2; ++i)
	{
		auto pTransformComp14 = pWorld->CreateComponent<TransformComponent>();
		pTransformComp14->SetPosition(Vector3(-3.0f + 6.5f * i, 3.5f, 0.0f));
		pTransformComp14->SetScale(Vector3(0.03f));

		auto pMeshFilterComp14 = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp14->SetMesh(pSphereMesh);

		auto pMaterial14 = std::make_shared<Material>();
		pMaterial14->SetAlbedoColor(Color4(0.89f, 0.67f, 0.24f, 1));
		pMaterial14->SetShaderProgram(eShaderProgram_AnimeStyle);
		pMaterial14->SetTexture(eMaterialTexture_Albedo, pDefaultTexture);
		pMaterial14->SetTexture(eMaterialTexture_Tone, pToneTexture);

		if (i == 0)
		{
			pMaterial14->SetAnisotropy(-0.75f);
			pMaterial14->SetRoughness(0.2f);
		}

		auto pMaterialComp14 = std::make_shared<MaterialComponent>();
		pMaterialComp14->AddMaterial(0, pMaterial14);

		auto pMeshRendererComp14 = std::make_shared<MeshRendererComponent>();
		pMeshRendererComp14->SetRenderer(eRenderer_Forward);

		auto pSphere = pWorld->CreateEntity<StandardEntity>();
		pSphere->AttachComponent(pTransformComp14);
		pSphere->AttachComponent(pMeshFilterComp14);
		pSphere->AttachComponent(pMaterialComp14);
		pSphere->AttachComponent(pMeshRendererComp14);
	}

	WriteECSWorldToJson(pWorld, "Assets/Scene/TestScene.json");
}