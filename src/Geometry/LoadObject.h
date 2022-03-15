#pragma once
#include"../Resources/FrameResource.h"
#include<string>
#include<vector>

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
