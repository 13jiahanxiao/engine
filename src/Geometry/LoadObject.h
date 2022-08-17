#pragma once
#include"../Resources/FrameResource.h"
#include<string>
#include<vector>
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

struct SubMesh 
{
	std::string m_name;
	int m_startVertex;
	int m_startIndex;
	int m_indexSize;
	//图片类型名，和图片名
	std::unordered_map <std::string, std::string> m_Texture;
};

class LoadObjectByAssimp 
{
public:
	LoadObjectByAssimp();
	~LoadObjectByAssimp();
	void LoadOBJ(std::string geoName, std::string fileName);
	void LoadOBJFace();
	void GetTexture();
	std::vector<Vertex> m_vertices;
	std::vector<std::uint16_t> m_indices;
	std::vector<SubMesh> m_submesh;
private:
	int m_indiceSize;
};

class LoadObject
{
public:
	LoadObject();
	~LoadObject();
	void Split(const std::string& in, std::vector<std::string>& out, std::string token);
	std::string Tail(const std::string& in);
	std::string FirstToken(const std::string& in);

	template <class T>
	inline const T& GetElement(const std::vector<T>& elements, std::string& index);

	void IndicesCreate(std::string curline, std::vector <DirectX::XMFLOAT3> pos,
		std::vector <DirectX::XMFLOAT3> normals,
		std::vector <DirectX::XMFLOAT2> texC);
	void LoadOBJ(std::string fileName);
	int m_IndiceSize;
	std::vector<std::uint16_t> m_Indices;
	std::vector<Vertex> m_Vertices;
};
