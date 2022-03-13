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

	void IndicesCreate(std::string curline);
	void LoadOBJ(std::string fileName);
	std::vector<std::uint16_t> m_VertIndices;
	std::vector<std::uint16_t> m_TexIndices;
	std::vector<std::uint16_t> m_NorIndices;
	std::vector<Vertex> m_Vertices;
};
