#pragma once
#include <json/json.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ResourceManager.h"
#include "ECSWorld.h"
#include "AllComponents.h"
#include "StandardEntity.h"
#include "ScriptSelector.h"
#include "ObjMesh.h"
#include "Plane.h"

namespace Engine
{
	bool ReadECSWorldFromJson(std::shared_ptr<ECSWorld> pWorld, const char* fileAddress)
	{
		if (!pWorld)
		{
			return false;
		}

		// Initialize readers

		std::ifstream fileReader;
		fileReader.open(fileAddress);

		if (fileReader.fail())
		{
			std::cout << "ECSSceneReader: Cannot open " << fileAddress << std::endl;
			return false;
		}

		Json::CharReaderBuilder builder;
		Json::Value root;

		JSONCPP_STRING errs;
		if (!parseFromStream(builder, fileReader, &root, &errs))
		{
			std::cerr << errs << std::endl;
			return false;
		}

		// Resolve the json structure and construct ECS world

		int entityCount = root["entityCount"].asInt();

		for (int i = 0; i < entityCount; ++i)
		{
			std::stringstream entityName;
			entityName << "entity" << i;
			Json::Value entity = root[entityName.str()];

			// Check and configure components

			std::stack<std::shared_ptr<BaseComponent>> components;
			// TODO: find a better solution
			int scriptID = -1;
			std::shared_ptr<ScriptComponent> pScriptComp = nullptr;

			if (entity["transform"])
			{
				Json::Value component = entity["transform"];

				Vector3 position(0);
				position.x = component["position"]["x"].asFloat();
				position.y = component["position"]["y"].asFloat();
				position.z = component["position"]["z"].asFloat();

				Vector3 rotation(0);
				rotation.x = component["rotation"]["x"].asFloat();
				rotation.y = component["rotation"]["y"].asFloat();
				rotation.z = component["rotation"]["z"].asFloat();

				Vector3 scale(0);
				scale.x = component["scale"]["x"].asFloat();
				scale.y = component["scale"]["y"].asFloat();
				scale.z = component["scale"]["z"].asFloat();

				auto pTransformComp = pWorld->CreateComponent<TransformComponent>();
				pTransformComp->SetPosition(position);
				pTransformComp->SetRotation(rotation);
				pTransformComp->SetScale(scale);

				components.push(pTransformComp);
			}
			if (entity["meshRenderer"])
			{
				Json::Value component = entity["meshRenderer"];

				auto pMeshRendererComp = std::make_shared<MeshRendererComponent>();
				pMeshRendererComp->SetRenderer((ERendererType)(component["rendererType"].asInt()));

				components.push(pMeshRendererComp);
			}
			if (entity["meshFilter"])
			{
				Json::Value component = entity["meshFilter"];

				bool attachMesh = true;
				std::shared_ptr<Mesh> pMesh = nullptr;
				switch ((EBuiltInMeshType)(component["type"].asInt()))
				{
				case eBuiltInMesh_External:
					//if (ResourceManagement::LoadedMeshes.find(component["filePath"].asCString()) == ResourceManagement::LoadedMeshes.end())
					{
						pMesh = std::make_shared<ObjMesh>(component["filePath"].asCString());
						//ResourceManagement::LoadedMeshes.emplace(component["filePath"].asCString(), pMesh);
					}
					//else
					//{
					//	pMesh = ResourceManagement::LoadedMeshes.at(component["filePath"].asCString());
					//}
					break;
				case eBuiltInMesh_Plane:
					pMesh = std::make_shared<Plane>(component["planeDimension"]["x"].asUInt(), component["planeDimension"]["y"].asUInt());
					break;
				default:
					std::cout << "ECSSceneReader: Unhandled mesh type: " << component["type"].asInt() << std::endl;
					attachMesh = false;
					break;
				}

				if (attachMesh && pMesh != nullptr)
				{
					auto pMeshFilterComp = pWorld->CreateComponent<MeshFilterComponent>();
					pMeshFilterComp->SetMesh(pMesh);

					components.push(pMeshFilterComp);
				}
			}
			if (entity["material"])
			{
				Json::Value component = entity["material"];

				auto pMaterialComp = std::make_shared<MaterialComponent>();

				pMaterialComp->SetShaderProgram((EBuiltInShaderProgramType)(component["shaderType"].asInt()));

				static std::string pathTypes[EMATERIALTEXTURETYPE_COUNT] =
				{
					"albedoTexturePath",
					"normalTexturePath",
					"roughnessTexturePath",
					"aoTexturePath",
					"noiseTexturePath",
					"toneTexturePath"
				};

				for (int i = 0; i < EMATERIALTEXTURETYPE_COUNT; ++i)
				{
					if (component[pathTypes[i]])
					{
						std::shared_ptr<Texture2D> pTexture = nullptr;
						//if (ResourceManagement::LoadedImageTextures.find(component[pathTypes[i]].asCString()) == ResourceManagement::LoadedImageTextures.end())
						{
							pTexture = std::make_shared<ImageTexture>(component[pathTypes[i]].asCString());
							//ResourceManagement::LoadedImageTextures.emplace(component[pathTypes[i]].asCString(), pTexture);
						}
						//else
						//{
						//	pTexture = ResourceManagement::LoadedImageTextures.at(component[pathTypes[i]].asCString());
						//}
						pMaterialComp->SetTexture((EMaterialTextureType)(eMaterialTexture_Albedo + i), pTexture); // Alert: here the check sequence must be consistent with enum sequence
					}
				}

				Color4 albedoColor(1);
				albedoColor.x = component["albedoColor"]["r"].asFloat();
				albedoColor.y = component["albedoColor"]["g"].asFloat();
				albedoColor.z = component["albedoColor"]["b"].asFloat();
				albedoColor.w = component["albedoColor"]["a"].asFloat();

				pMaterialComp->SetAlbedoColor(albedoColor);

				pMaterialComp->SetTransparent(component["transparent"].asBool());

				components.push(pMaterialComp);
			}
			if (entity["camera"])
			{
				Json::Value component = entity["camera"];

				auto pCameraComp = pWorld->CreateComponent<CameraComponent>();

				pCameraComp->SetFOV(component["fov"].asFloat());
				pCameraComp->SetClipDistance(component["nearClip"].asFloat(), component["farClip"].asFloat());
				pCameraComp->SetProjectionType((ECameraProjectionType)(component["projection"].asInt()));

				Color4 clearColor(0);
				clearColor.x = component["clearColor"]["r"].asFloat();
				clearColor.y = component["clearColor"]["g"].asFloat();
				clearColor.z = component["clearColor"]["b"].asFloat();
				clearColor.w = component["clearColor"]["a"].asFloat();

				pCameraComp->SetClearColor(clearColor);

				components.push(pCameraComp);
			}
			if (entity["animation"])
			{
				// Anmiation component is unhandled at this moment
			}
			if (entity["script"])
			{
				Json::Value component = entity["script"];

				pScriptComp = pWorld->CreateComponent<ScriptComponent>();

				scriptID = component["scriptID"].asInt();

				components.push(pScriptComp);
			}

			// Instantiate entity and attach components

			auto pEntity = pWorld->CreateEntity<StandardEntity>();
			
			if (scriptID >= 0)
			{
				pScriptComp->BindScript(SampleScript::GenerateScriptByID((SampleScript::EScriptID)(scriptID), pEntity));
			}

			while (!components.empty())
			{
				pEntity->AttachComponent(components.top());
				components.pop();
			}

			pEntity->SetEntityTag((EEntityTag)(entity["tag"].asInt()));
		}

		return true;
	}
}