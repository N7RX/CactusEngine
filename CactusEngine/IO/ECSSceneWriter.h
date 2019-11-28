#pragma once
#include <json/json.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ECSWorld.h"
#include "AllComponents.h"

namespace Engine
{
	static bool WriteECSWorldToJson(const std::shared_ptr<ECSWorld> pWorld, const char* fileAddress)
	{
		if (!pWorld)
		{
			return false;
		}

		// Initialize writers

		Json::StreamWriterBuilder builder;
		const std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());

		std::ofstream fileWriter;
		fileWriter.open(fileAddress);

		if (fileWriter.fail())
		{
			std::cout << "ECSSceneWriter: Cannot access " << fileAddress << std::endl;
			return false;
		}

		// Construct json hierarchy struture

		Json::Value root;

		auto pEntityList = pWorld->GetEntityList();
		unsigned int entityIndex = 0;
		for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
		{
			Json::Value entity;
			entity["tag"] = itr->second->GetEntityTag();
			
			// Write components of each entity

			auto componentList = itr->second->GetComponentList();
			for (auto componentEntry : componentList)
			{
				switch (componentEntry.first)
				{
				case eCompType_Transform:
				{
					auto pTransformComp = std::static_pointer_cast<TransformComponent>(componentEntry.second);
					Json::Value component;

					Vector3 position = pTransformComp->GetPosition();
					component["position"]["x"] = position.x;
					component["position"]["y"] = position.y;
					component["position"]["z"] = position.z;

					Vector3 rotation = pTransformComp->GetRotation();
					component["rotation"]["x"] = rotation.x;
					component["rotation"]["y"] = rotation.y;
					component["rotation"]["z"] = rotation.z;

					Vector3 scale = pTransformComp->GetScale();
					component["scale"]["x"] = scale.x;
					component["scale"]["y"] = scale.y;
					component["scale"]["z"] = scale.z;

					entity["transform"] = component;
					break;
				}
				case eCompType_MeshRenderer:
				{
					auto pMeshRendererComp = std::static_pointer_cast<MeshRendererComponent>(componentEntry.second);
					Json::Value component;

					component["rendererType"] = pMeshRendererComp->GetRendererType();

					entity["meshRenderer"] = component;
					break;
				}
				case eCompType_MeshFilter:
				{
					auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(componentEntry.second);
					Json::Value component;

					auto pMesh = pMeshFilterComp->GetMesh();

					component["type"] = pMesh->GetMeshType();

					switch (pMesh->GetMeshType())
					{
					case eBuiltInMesh_External:
						component["filePath"] = pMesh->GetFilePath();
						break;
					case eBuiltInMesh_Plane:
						auto planeDimension = pMesh->GetPlaneDimenstion();
						component["planeDimension"]["x"] = planeDimension.x;
						component["planeDimension"]["y"] = planeDimension.y;
						break;
					default:
						std::cout << "ECSSceneWriter: Unhandled mesh type: " << pMesh->GetMeshType() << std::endl;
						break;
					}

					entity["meshFilter"] = component;
					break;
				}
				case eCompType_Material:
				{
					auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(componentEntry.second);
					Json::Value component;

					component["shaderType"] = pMaterialComp->GetShaderProgramType();

					auto textureList = pMaterialComp->GetTextureList();
					for (auto textureEntry : textureList)
					{
						switch (textureEntry.first)
						{
						case eMaterialTexture_Albedo:
							component["albedoTexturePath"] = textureEntry.second->GetFilePath();
							break;
						case eMaterialTexture_Normal:
							component["normalTexturePath"] = textureEntry.second->GetFilePath();
							break;
						case eMaterialTexture_Roughness:
							component["roughnessTexturePath"] = textureEntry.second->GetFilePath();
							break;
						case eMaterialTexture_AO:
							component["aoTexturePath"] = textureEntry.second->GetFilePath();
							break;
						case eMaterialTexture_Noise:
							component["noiseTexturePath"] = textureEntry.second->GetFilePath();
							break;
						case eMaterialTexture_Tone:
							component["toneTexturePath"] = textureEntry.second->GetFilePath();
							break;
						default:
							std::cout << "ECSSceneWriter: Unhandled texture type: " << textureEntry.first << std::endl;
							break;
						}
					}

					Color4 albedoColor = pMaterialComp->GetAlbedoColor();
					component["albedoColor"]["r"] = albedoColor.x;
					component["albedoColor"]["g"] = albedoColor.y;
					component["albedoColor"]["b"] = albedoColor.z;
					component["albedoColor"]["a"] = albedoColor.w;

					component["transparent"] = pMaterialComp->IsTransparent();

					entity["material"] = component;
					break;
				}
				case eCompType_Camera:
				{
					auto pCameraComp = std::static_pointer_cast<CameraComponent>(componentEntry.second);
					Json::Value component;

					component["fov"] = pCameraComp->GetFOV();
					component["nearClip"] = pCameraComp->GetNearClip();
					component["farClip"] = pCameraComp->GetFarClip();
					component["projection"] = pCameraComp->GetProjectionType();

					Color4 clearColor = pCameraComp->GetClearColor();
					component["clearColor"]["r"] = clearColor.x;
					component["clearColor"]["g"] = clearColor.y;
					component["clearColor"]["b"] = clearColor.z;
					component["clearColor"]["a"] = clearColor.w;

					entity["camera"] = component;
					break;
				}
				case eCompType_Animation:
				{
					// Anmiation component is unhandled at this moment
					break;
				}
				case eCompType_Script:
				{
					auto pScriptComp = std::static_pointer_cast<ScriptComponent>(componentEntry.second);
					Json::Value component;

					component["scriptID"] = pScriptComp->GetScript()->GetScriptID();
					
					entity["script"] = component;
					break;
				}
				default:
					std::cout << "ECSSceneWriter: Unhandled component type: " << componentEntry.first << std::endl;
					break;
				}
			}
			
			std::stringstream entityName;
			entityName << "entity" << entityIndex;
			root[entityName.str()] = entity;
			entityIndex++;
		}

		root["entityCount"] = entityIndex;

		// Write to json file

		jsonWriter->write(root, &fileWriter);
		fileWriter.close();

		return true;
	}
}