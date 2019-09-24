#include "GLCDrawingSystem.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "GLCMesh.h"
#include <iostream>
#include <fstream>

using namespace Engine;

GLCDrawingSystem::GLCDrawingSystem(ECSWorld* pWorld)
	: m_pECSWorld(pWorld)
{
	// This is the default output texture size, equivalent to 65536 rays
	ConfigureDrawDimension(256, 256);
	SetBackgroundColor(Color3(0.2f, 0.2f, 0.3f));

	m_glcOrigin_Vert = 1;
	m_glcOrigin_Hori = 1;
}

void GLCDrawingSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t GLCDrawingSystem::GetSystemID() const
{
	return m_systemID;
}

void GLCDrawingSystem::Initialize()
{
	m_linearResult.resize(m_drawWidth * m_drawHeight * LINEAR_RESULT_WIDTH, 0.0f);
	if (!m_renderResult)
	{
		m_renderResult = std::make_shared<RenderTexture>(m_drawWidth, m_drawHeight);
	}
}

void GLCDrawingSystem::ShutDown()
{

}

void GLCDrawingSystem::FrameBegin()
{

}

void GLCDrawingSystem::Tick()
{
	BuildRenderTask();
	ConfigureRenderEnvironment();
	ExecuteRenderTask();
}

void GLCDrawingSystem::FrameEnd()
{

}

void GLCDrawingSystem::ConfigureDrawDimension(uint32_t width, uint32_t height)
{
	m_drawWidth = width;
	m_drawHeight = height;
}

void GLCDrawingSystem::SetBackgroundColor(Color3 color)
{
	m_backgroundColor = color;
}

void GLCDrawingSystem::BuildRenderTask()
{
	const EntityList* pEntityList = m_pECSWorld->GetEntityList();
	for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
	{
		if (itr->second->GetEntityTag() == eEntityTag_GLCMesh)
		{
			m_renderTaskList.emplace_back(itr->second);
		}
	}
}

void GLCDrawingSystem::ConfigureRenderEnvironment()
{
	auto pGLC = m_pECSWorld->FindEntityWithTag(eEntityTag_MainGLC);
	if (!pGLC)
	{
		std::cerr << "No main GLC assigned.\n";
		return;
	}

	auto pGLCComp = std::static_pointer_cast<GLCComponent>(pGLC->GetComponent(eCompType_GLC));

	auto pRayDef1 = pGLCComp->GetRayDefinition(0);
	auto pRayDef2 = pGLCComp->GetRayDefinition(1);
	auto pRayDef3 = pGLCComp->GetRayDefinition(2);
	pRayDef1->SetOrigin(Vector2(0, 0));
	pRayDef1->SetImagePlanePoint(Vector2(0, 0));
	pRayDef2->SetOrigin(Vector2(m_glcOrigin_Hori, 0));
	pRayDef2->SetImagePlanePoint(Vector2(m_drawWidth, 0));
	pRayDef3->SetOrigin(Vector2(0, m_glcOrigin_Vert));
	pRayDef3->SetImagePlanePoint(Vector2(0, m_drawHeight));

	pGLCComp->UpdateParameters();
}

void GLCDrawingSystem::ExecuteRenderTask()
{
	auto pGLC = m_pECSWorld->FindEntityWithTag(eEntityTag_MainGLC);
	if (!pGLC)
	{
		std::cerr << "No main GLC assigned.\n";
		return;
	}

	auto pGLCComp = std::static_pointer_cast<GLCComponent>(pGLC->GetComponent(eCompType_GLC));

	RayTraceDraw(pGLCComp, m_renderTaskList);
	
	//WriteLinearResultToPPM();
	WriteLinearResultToTexture();

	m_renderTaskList.clear();
}

void GLCDrawingSystem::RayTraceDraw(const std::shared_ptr<GLCComponent> pGLC, const std::vector<std::shared_ptr<IEntity>>& drawList)
{
	for (uint32_t column = 0; column < m_drawWidth; ++column)
	{
		for (uint32_t row = 0; row < m_drawHeight; ++row)
		{
			auto pRay = pGLC->GenerateRay(column, row);
			Color4 result = RayIntersect(pRay, drawList);
			
			m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH] = result.r;
			m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH + 1] = result.g;
			m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH + 2] = result.b;
			m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH + 3] = result.a;
		}
	}
}

Color4 GLCDrawingSystem::RayIntersect(const std::shared_ptr<GLCRay> pRay, const std::vector<std::shared_ptr<IEntity>>& drawList)
{
	for (auto& entity : drawList)
	{
		auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(eCompType_Transform));
		auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(eCompType_MeshFilter));

		if (!pTransformComp || !pMeshFilterComp)
		{
			continue;
		}

		auto pMesh = std::static_pointer_cast<GLCMesh>(pMeshFilterComp->GetMesh());

		if (!pMesh)
		{
			continue;
		}

		Matrix4x4 modelMat = pTransformComp->GetModelMatrix();

		for (auto& triangle : pMesh->GetTriangles())
		{
			Vector4 v1 = modelMat * Vector4(triangle.v1, 1.0f);
			Vector4 v2 = modelMat * Vector4(triangle.v2, 1.0f);
			Vector4 v3 = modelMat * Vector4(triangle.v3, 1.0f);
			
			if (RayTriangleIntersect(pRay, Vector3(v1), Vector3(v2), Vector3(v3)))
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(eCompType_Material));
				if (pMaterialComp)
				{
					return pMaterialComp->GetAlbedoColor();
				}
				return Vector4(1, 1, 1, 1);
			}
		}
	}

	return Vector4(m_backgroundColor, 1.0f);
}

bool GLCDrawingSystem::RayTriangleIntersect(const std::shared_ptr<GLCRay> pRay, const Vector3 v1, const Vector3 v2, Vector3 v3)
{
	// Algorithm adapted from scratchapixel.com

	Vector3 normal = glm::cross(v2 - v1, v3 - v1);

	Vector3 rayDir = Vector3(pRay->GetDirection().x / float(m_drawWidth), pRay->GetDirection().y / float(m_drawHeight), pRay->GetDirection().z);
	Vector3 rayOri = Vector3(pRay->GetOrigin().x / float(m_drawWidth - m_glcOrigin_Hori + 1), pRay->GetOrigin().y / float(m_drawHeight - m_glcOrigin_Vert + 1), pRay->GetOrigin().z);
	// Error: this origin correction does not fit the middle results

	float NormalDotRayDir = glm::dot(normal, rayDir);
	if (fabs(NormalDotRayDir) < FLT_EPSILON) // Triangle is parallel to the ray
	{
		return false;
	}

	float planeDistance = glm::dot(normal, v1);
	float t = (glm::dot(normal, rayOri) + planeDistance) / NormalDotRayDir;
	if (t < 0) // Triangle is behind
	{
		return false;
	}

	// Test for each edge

	Vector3 pointOnRay = rayOri + t * rayDir;
	Vector3 vertDir;

	Vector3 edge = v2 - v1;
	Vector3 vp = pointOnRay - v1;
	vertDir = glm::cross(edge, vp); // The signed would be inverted depending on the side
	if (glm::dot(normal, vertDir) < 0)
	{
		return false;
	}

	edge = v3 - v2;
	vp = pointOnRay - v2;
	vertDir = glm::cross(edge, vp);
	if (glm::dot(normal, vertDir) < 0)
	{
		return false;
	}

	edge = v1 - v3;
	vp = pointOnRay - v3;
	vertDir = glm::cross(edge, vp);
	if (glm::dot(normal, vertDir) < 0)
	{
		return false;
	}

	return true;
}

void GLCDrawingSystem::WriteLinearResultToTexture()
{
	m_renderResult->FlushData(m_linearResult.data(), eDataType_Float, eFormat_RGBA32F);
}

void GLCDrawingSystem::WriteLinearResultToPPM()
{
	std::ofstream fileWriter;
	fileWriter.open("GLCOutput.ppm");

	if (fileWriter.fail())
	{
		std::cerr << "Cannot open GLCOutput.ppm" << std::endl;
		return;
	}

	fileWriter << "P3\n" << m_drawWidth << " " << m_drawHeight << "\n255\n";

	for (uint32_t row = 0; row < m_drawHeight; ++row)
	{
		for (uint32_t column = 0; column < m_drawWidth; ++column)
		{
			int ir = int(255.9 * m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH]);
			int ig = int(255.9 * m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH + 1]);
			int ib = int(255.9 * m_linearResult[(row * m_drawWidth + column) * LINEAR_RESULT_WIDTH + 2]);

			fileWriter << ir << " " << ig << " " << ib << "\n";
		}
	}

	fileWriter.close();
}