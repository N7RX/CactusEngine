#pragma once
#include "Mesh.h"
#include "ImageTexture.h"

#include <unordered_map>

namespace Engine
{
	namespace ResourceManagement
	{
		typedef std::unordered_map<std::string, Texture2D*> FileTextureList; // TODO: store hashed strings instead of strings
		typedef std::unordered_map<std::string, Mesh*> FileMeshList;

		// Record already external resources to prevent duplicate loading
		static FileTextureList LoadedImageTextures;
		static FileMeshList LoadedMeshes;
	}
}