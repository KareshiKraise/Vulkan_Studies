#include "AssetUtilities.h"
#include <iostream>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <stdexcept>
#include "Renderer.h"

static std::string directory;
static std::vector<Mesh> models;
static std::vector<Texture> textures_loaded;

void VK_CHECK_RESULT(VkResult ret, std::string msg)
{
	if (ret != VK_SUCCESS)
	{
		throw std::runtime_error(msg);
	}
}

std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, Vulkan_Backend& in_backend)
{
	std::vector<Texture> textures;
	for (int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true; break;
			}
		}
		if (!skip)
		{
			Texture texture;
			texture.type = typeName;
			texture.path = directory + "/" + std::string(str.C_Str());//str.C_Str();
			texture.setupTexture((in_backend));
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}

	return textures;
}

static Mesh processModel(aiMesh* mesh, const aiScene* scene, Vulkan_Backend& in_backend)
{
	std::vector<vertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<Texture> textures;

	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		vertex vert;
		glm::vec3 vec;
		vec.x = mesh->mVertices[i].x;
		vec.y = mesh->mVertices[i].y;
		vec.z = mesh->mVertices[i].z;
		vert.pos = vec;

		vec.x = mesh->mNormals[i].x;
		vec.y = mesh->mNormals[i].y;
		vec.z = mesh->mNormals[i].z;
		vert.normal = vec;

		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vert.uv = vec;
		}
		else {
			vert.uv = glm::vec2(0.0f, 0.0f);
			std::cout << "no uvs detected" << std::endl;
		}

		if (mesh->HasTangentsAndBitangents())
		{
			/*tangents and bitangents*/
			glm::vec3 t;
			t.x = mesh->mTangents[i].x;
			t.y = mesh->mTangents[i].y;
			t.z = mesh->mTangents[i].z;
			vert.tangent = t;
			glm::vec3 bt;
			bt.x = mesh->mBitangents[i].x;
			bt.y = mesh->mBitangents[i].y;
			bt.z = mesh->mBitangents[i].z;
			vert.bitangent = bt;
			vertices.push_back(vert);
		}
		else
		{
			std::cout << "Model doesnt have tangents" << std::endl;
		}
	}

	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", in_backend);
	//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	//std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	//
	//std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	//textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	//
	//std::vector<Texture> transparencyMaps = loadMaterialTextures(material, aiTextureType_OPACITY, "texture_alphamask");
	//textures.insert(textures.end(), transparencyMaps.begin(), transparencyMaps.end());
	//
	//std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	//textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
		
	Material diffMat;
	diffMat.diffuse = &diffuseMaps.front();	
	return Mesh(vertices, indices, diffMat);
}

static void processNode(aiNode* node, const aiScene* scene, Vulkan_Backend& in_backend)
{
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		models.push_back(processModel(mesh, scene, in_backend));
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, in_backend);
	}
}

std::vector<Mesh> utils::loadOBJ(std::string path, Vulkan_Backend& in_backend)
{
	
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	scene = importer.ApplyPostProcessing(aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "[ERROR:ASSIMP]: " << importer.GetErrorString() << std::endl;
		throw std::runtime_error("Assimp failed to load scene OBJ");
	}

	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene, in_backend);

	return models;
}

std::vector<char> utils::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule utils::createShaderModule(const std::vector<char>& code, const VkDevice& device)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}