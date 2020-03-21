#include <iostream>
#include "GraphicsApplication.h"
#include "Global.h"
#include "DrawingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "ECSSceneWriter.h"
#include "ECSSceneReader.h"

#include "AllComponents.h"
#include "ImageTexture.h"
#include "ExternalMesh.h"
#include "StandardEntity.h"
#include "ScriptSelector.h"

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
		system("pause");
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
			system("pause");
			return -1;
		}
	}

	pApplication->ShutDown();

	return 0;
}

void ConfigSetup()
{
	gpGlobal->CreateConfiguration<AppConfiguration>(EConfigurationType::App);
	gpGlobal->CreateConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics);

	// TODO: read setitings from file
	gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->SetAppName("Cactus Engine");
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetDeviceType(EGraphicsDeviceType::Vulkan);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetWindowSize(1600, 900);
	gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->SetVSync(true);
}

void TestSetup(GraphicsApplication* pApp)
{
	auto pWorld = pApp->GetECSWorld();

	// Alert: the registration sequence may correspond to execution sequence
	pWorld->RegisterSystem<InputSystem>(ESystemType::Input);
	//pWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation);
	pWorld->RegisterSystem<DrawingSystem>(ESystemType::Script);
	pWorld->RegisterSystem<ScriptSystem>(ESystemType::Drawing);

#ifndef ENABLE_ASYNC_COMPUTE_TEST_CE
	// Read scene from file
	ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	//ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");

	// Or manually add contents here
	// ...
#else
	// Camera
	auto pCameraComponent = pWorld->CreateComponent<CameraComponent>();
	pCameraComponent->SetClearColor(Color4(1.0f, 1.0f, 1.0f, 0.0f));

	auto pCameraTransformComp = pWorld->CreateComponent<TransformComponent>();
	pCameraTransformComp->SetPosition(Vector3(-4.0f, 2.5f, 3.4f));
	pCameraTransformComp->SetRotation(Vector3(-33, -45, 0));

	auto pCameraScriptComp = pWorld->CreateComponent<ScriptComponent>();

	auto pCamera = pWorld->CreateEntity<StandardEntity>();
	pCamera->AttachComponent(pCameraComponent);
	pCamera->AttachComponent(pCameraTransformComp);
	pCamera->AttachComponent(pCameraScriptComp);
	pCamera->SetEntityTag(EEntityTag::MainCamera);

	pCameraScriptComp->BindScript(SampleScript::GenerateScriptByID((SampleScript::EScriptID::Camera), pCamera));

	// 512 Objects
	auto pMesh = std::make_shared<ExternalMesh>("Assets/Models/Amago_0.obj");
	auto pAlbedoTexture = std::make_shared<ImageTexture>("Assets/Textures/Amago_0.bmp");
	auto pToneTexture = std::make_shared<ImageTexture>("Assets/Textures/XToon_BW.png");

	for (unsigned int i = 0; i < 8; i++)
	{
		for (unsigned int j = 0; j < 8; j++)
		{
			for (unsigned int k = 0; k < 8; k++)
			{
				auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
				pTransformComp->SetPosition(Vector3((float(i) - 4) * 0.5f, (float(k) - 4) * 0.5f, (float(j) - 4) * 0.5f));
				pTransformComp->SetScale(Vector3(1.5f));

				auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
				pMeshFilterComp->SetMesh(pMesh);

				auto pMaterial = std::make_shared<Material>();
				pMaterial->SetAlbedoColor(Color4(1.0f));
				pMaterial->SetShaderProgram(EBuiltInShaderProgramType::AnimeStyle);
				pMaterial->SetTexture(EMaterialTextureType::Albedo, pAlbedoTexture);
				pMaterial->SetTexture(EMaterialTextureType::Tone, pToneTexture);

				auto pMaterialComp = std::make_shared<MaterialComponent>();
				pMaterialComp->AddMaterial(0, pMaterial);

				auto pMeshRendererComp = std::make_shared<MeshRendererComponent>();
				pMeshRendererComp->SetRenderer(ERendererType::Forward);

				auto pSphere = pWorld->CreateEntity<StandardEntity>();
				pSphere->AttachComponent(pTransformComp);
				pSphere->AttachComponent(pMeshFilterComp);
				pSphere->AttachComponent(pMaterialComp);
				pSphere->AttachComponent(pMeshRendererComp);
			}
		}
	}
#endif

	// Write scene to file
	//WriteECSWorldToJson(pWorld, "Assets/Scene/NewScene.json");
}