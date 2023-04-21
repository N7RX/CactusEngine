#include "GraphicsApplication.h"
#include "Global.h"
#include "MemoryAllocator.h"
#include "RenderingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"
#include "LogUtility.h"

// Temporary includes
#include "AllComponents.h"
#include "ExternalMesh.h"
#include "ImageTexture.h"
#include "StandardEntity.h"
#include "ScriptSelector.h"
#include "SampleScript/LightScript.h"
#include "Timer.h"

// This is the entry of the program

using namespace Engine;

void ConfigSetup();
void TestSetup(GraphicsApplication* pApp);

int32_t main(uint32_t intParam, char** charParam)
{
	LOG_SYSTEM_INITIALIZE();

	if (gpGlobal == nullptr)
	{
		CE_NEW(gpGlobal, Global);
	}

	GraphicsApplication* pApplication;
	CE_NEW(pApplication, GraphicsApplication);

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
		LOG_ERROR(e.what());
		system("pause");
		return -1;
	}

	while (!pApplication->ShouldQuit())
	{
		try
		{
			pApplication->Tick();

			// For memory leak debugging
			//LOG_MESSAGE("Active allocation count: " + std::to_string(gAllocationTrackerInstance.GetActiveAllocationCount()));
		}
		catch (const std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			system("pause");
			return -1;
		}
	}

	pApplication->ShutDown();

	CE_DELETE(pApplication);

	CE_DELETE(gpGlobal);
	LOG_SYSTEM_SHUTDOWN();

	return 0;
}

void ConfigSetup()
{
	gpGlobal->CreateConfiguration<AppConfiguration>(EConfigurationType::App);
	gpGlobal->CreateConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics);

	// TODO: read settings from file

	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppVersion("0.0.1");

	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetGraphicsAPIType(EGraphicsAPIType::Vulkan);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetPreferredGPUType(EGPUType::Discrete);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetWindowSize(1920, 1080);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetMaxFramesInFlight(3);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetVSync(false);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetPrebuildShadersAndPipelines(false);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetActiveRenderer(ERendererType::Advanced);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetTextureAnisotropyLevel(ESamplerAnisotropyLevel::AFx4);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetRenderScale(1.0f);
}

void TestAddLights(ECSWorld* pWorld)
{
	// 121 point lights for deferred lighting test

	LightComponent::Profile lightProfile{};
	lightProfile.sourceType = LightComponent::SourceType::Point;
	lightProfile.lightColor = Color3(1.0f);
	lightProfile.lightIntensity = 10.0f;
	lightProfile.radius = 1.0f;
	CE_NEW(lightProfile.pVolumeMesh, ExternalMesh, "Assets/Models/light_sphere.obj");

	Color3 colors[7] = {
		Color3(1.0f),
		Color3(1.0f, 0.0f, 0.0f),
		Color3(0.0f, 1.0f, 0.0f),
		Color3(0.0f, 0.0f, 1.0f),
		Color3(1.0f, 0.0f, 1.0f),
		Color3(0.0f, 1.0f, 1.0f),
		Color3(1.0f, 1.0f, 0.0f)
	};
	uint32_t colorIndex = 0;

	for (int32_t i = -5; i < 6; i++)
	{
		for (int32_t j = -5; j < 6; j++)
		{
			auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
			auto pLightComp = pWorld->CreateComponent<LightComponent>();
			auto pScriptComp = pWorld->CreateComponent<ScriptComponent>();

			auto pLight = pWorld->CreateEntity<StandardEntity>();
			pLight->AttachComponent(pTransformComp);
			pLight->AttachComponent(pLightComp);
			pLight->AttachComponent(pScriptComp);

			pTransformComp->SetPosition(Vector3(4.0f * (i / 5.0f), 0.1f, 8.0f * (j / 5.0f)));
			lightProfile.lightColor = colors[(colorIndex++) % 7];
			pLightComp->UpdateProfile(lightProfile);

			SampleScript::LightScript* pSript;
			CE_NEW(pSript, SampleScript::LightScript, pLight);
			pScriptComp->BindScript(pSript);
		}
	}
}

void TestBuildCornellBox(ECSWorld* pWorld)
{
	// Camera

	{
		auto pCameraComponent = pWorld->CreateComponent<CameraComponent>();
		pCameraComponent->SetClearColor(Color4(0.2f, 0.2f, 0.3f, 1.0f));

		auto pCameraTransformComp = pWorld->CreateComponent<TransformComponent>();
		pCameraTransformComp->SetPosition(Vector3(0, 0, 7.0f));

		auto pCamera = pWorld->CreateEntity<StandardEntity>();
		pCamera->AttachComponent(pCameraComponent);
		pCamera->AttachComponent(pCameraTransformComp);
		pCamera->SetEntityTag(EEntityTag::MainCamera);

		auto pScriptComp = pWorld->CreateComponent<ScriptComponent>();
		pScriptComp->BindScript(SampleScript::GenerateScriptByID(SampleScript::EScriptID::Camera, pCamera));
		pCamera->AttachComponent(pScriptComp);
	}

	ExternalMesh* pCubeMesh;
	CE_NEW(pCubeMesh, ExternalMesh, "Assets/Models/Cube.obj");

	ImageTexture* pDefaultTexture;
	CE_NEW(pDefaultTexture, ImageTexture, "Assets/Textures/Default.png");

	ImageTexture* pToneTexture;
	CE_NEW(pToneTexture, ImageTexture, "Assets/Textures/XToon_BW.png");

	// Red wall
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(-3.0f, 0, 0));
		pTransformComp->SetScale(Vector3(5.0f, 5.0f, 1.0f));

		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pCubeMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(1.0f, 0, 0, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::Basic);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pCube = pWorld->CreateEntity<StandardEntity>();
		pCube->AttachComponent(pTransformComp);
		pCube->AttachComponent(pMeshFilterComp);
		pCube->AttachComponent(pMaterialComp);
	}

	// Green wall
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(3.0f, 0, 0));
		pTransformComp->SetScale(Vector3(5.0f, 5.0f, 1.0f));

		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pCubeMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(0, 1.0f, 0, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::Basic);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pCube = pWorld->CreateEntity<StandardEntity>();
		pCube->AttachComponent(pTransformComp);
		pCube->AttachComponent(pMeshFilterComp);
		pCube->AttachComponent(pMaterialComp);
	}

	// White wall - back
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(0, 0, -3.0f));
		pTransformComp->SetScale(Vector3(1.0f, 5.0f, 5.0f));

		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pCubeMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::Basic);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pCube = pWorld->CreateEntity<StandardEntity>();
		pCube->AttachComponent(pTransformComp);
		pCube->AttachComponent(pMeshFilterComp);
		pCube->AttachComponent(pMaterialComp);
	}

	// White wall - top
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(0.0f, 3.0f, 0));
		pTransformComp->SetScale(Vector3(5.0f, 1.0f, 5.0f));

		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pCubeMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::Basic);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pCube = pWorld->CreateEntity<StandardEntity>();
		pCube->AttachComponent(pTransformComp);
		pCube->AttachComponent(pMeshFilterComp);
		pCube->AttachComponent(pMaterialComp);
	}

	// White wall - down
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(0.0f, -3.0f, 0));
		pTransformComp->SetScale(Vector3(5.0f, 1.0f, 5.0f));

		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pCubeMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::Basic);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pCube = pWorld->CreateEntity<StandardEntity>();
		pCube->AttachComponent(pTransformComp);
		pCube->AttachComponent(pMeshFilterComp);
		pCube->AttachComponent(pMaterialComp);
	}

	// Statue
	{
		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		pTransformComp->SetPosition(Vector3(0.0f, -2.5f, 0.0f));
		pTransformComp->SetRotation(Vector3(0, 90, 0));
		pTransformComp->SetScale(Vector3(0.005f, 0.005f, 0.005f));

		ExternalMesh* pStatueMesh;
		CE_NEW(pStatueMesh, ExternalMesh, "Assets/Models/Lucy.obj");
		auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
		pMeshFilterComp->SetMesh(pStatueMesh);

		auto pMaterialComp = pWorld->CreateComponent<MaterialComponent>();
		Material* pMaterial;
		CE_NEW(pMaterial, Material);
		pMaterial->SetAlbedoColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
		pMaterial->SetShaderProgram(EBuiltInShaderProgramType::AnimeStyle);
		pMaterial->SetTexture(EMaterialTextureType::Albedo, pDefaultTexture);
		pMaterial->SetTexture(EMaterialTextureType::Tone, pToneTexture);
		pMaterialComp->AddMaterial(0, pMaterial);

		auto pAnimationComp = pWorld->CreateComponent<AnimationComponent>();
		pAnimationComp->SetAnimFunction(
			[](BaseEntity* pEntity)
			{
				auto pTransform = (TransformComponent*)pEntity->GetComponent(EComponentType::Transform);
				Vector3 currRotation = pTransform->GetRotation();
				currRotation.y += Timer::GetFrameDeltaTime() * 100.0f;
				if (currRotation.y >= 360)
				{
					currRotation.y = 0;
				}
				pTransform->SetRotation(currRotation);
			});

		auto pStatue = pWorld->CreateEntity<StandardEntity>();
		pStatue->AttachComponent(pTransformComp);
		pStatue->AttachComponent(pMeshFilterComp);
		pStatue->AttachComponent(pMaterialComp);
		pStatue->AttachComponent(pAnimationComp);
	}

	// Light
	{
		LightComponent::Profile lightProfile{};
		lightProfile.sourceType = LightComponent::SourceType::Point;
		lightProfile.lightColor = Color3(1.0f);
		lightProfile.lightIntensity = 15.0f;
		lightProfile.radius = 10.0f;
		CE_NEW(lightProfile.pVolumeMesh, ExternalMesh, "Assets/Models/light_sphere.obj");

		auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
		auto pLightComp = pWorld->CreateComponent<LightComponent>();

		pTransformComp->SetPosition(Vector3(0, 1.95f, 0));
		lightProfile.lightColor = Color3(1.0f, 1.0f, 1.0f);

		auto pLight = pWorld->CreateEntity<StandardEntity>();
		pLight->AttachComponent(pTransformComp);
		pLight->AttachComponent(pLightComp);

		pLightComp->UpdateProfile(lightProfile);
	}
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	pWorld->RegisterSystem<RenderingSystem>(ESystemType::Rendering, 2);
	pWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation, 1);
	pWorld->RegisterSystem<InputSystem>(ESystemType::Input, 1);
	pWorld->RegisterSystem<ScriptSystem>(ESystemType::Script, 0);
	pWorld->SortSystems();

	// Read scene from file
	ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");

	// Or manually add contents here
	// ...
	//TestBuildCornellBox(pWorld);
	TestAddLights(pWorld);

	// Save scene to file
	//WriteECSWorldToJson(pWorld, "Assets/Scene/NewScene.json");
}