#include"LoadObject.h"
#include"../Resources/FrameResource.h"
#include<string>
#include<vector>
#include <fstream>

using namespace REngine;

LoadObject::LoadObject()
{
	m_IndiceSize = 0;
}
LoadObject::~LoadObject()
{
}
void LoadObject::Split(const std::string& in, std::vector<std::string>& out, std::string token)
{
	out.clear();
	std::string temp;
	for (int i = 0; i < int(in.size()); i++)
	{
		std::string test = in.substr(i, token.size());

		if (test == token)
		{
			if (!temp.empty())
			{
				out.push_back(temp);
				temp.clear();
				i += (int)token.size() - 1;
			}
			else
			{
				out.push_back("");
			}
		}
		else if (i + token.size() >= in.size())
		{
			temp += in.substr(i, token.size());
			out.push_back(temp);
			break;
		}
		else
		{
			temp += in[i];
		}
	}
}
std::string LoadObject::Tail(const std::string& in)
{
	size_t token_start = in.find_first_not_of(" \t");
	size_t space_start = in.find_first_of(" \t", token_start);
	size_t tail_start = in.find_first_not_of(" \t", space_start);
	size_t tail_end = in.find_last_not_of(" \t");
	if (tail_start != std::string::npos && tail_end != std::string::npos)
	{
		return in.substr(tail_start, tail_end - tail_start + 1);
	}
	else if (tail_start != std::string::npos)
	{
		return in.substr(tail_start);
	}
	return "";
}
std::string LoadObject::FirstToken(const std::string& in)
{
	if (!in.empty())
	{
		size_t token_start = in.find_first_not_of(" \t");
		size_t token_end = in.find_first_of(" \t", token_start);
		if (token_start != std::string::npos && token_end != std::string::npos)
		{
			return in.substr(token_start, token_end - token_start);
		}
		else if (token_start != std::string::npos)
		{
			return in.substr(token_start);
		}
	}
	return "";
}
template <class T>
inline const T& LoadObject::GetElement(const std::vector<T>& elements, std::string& index)
{
	int idx = std::stoi(index);
	if (idx < 0)
		idx = int(elements.size()) + idx;
	else
		idx--;
	return elements[idx];
}
void LoadObject::IndicesCreate(std::string curline, std::vector <DirectX::XMFLOAT3> pos,
std::vector <DirectX::XMFLOAT3> normals,
std::vector <DirectX::XMFLOAT2> texC)
{
	//先把顶点数据设置好，然后将每个点的数据从0到n展开
	std::vector<std::string> sface, svert;
	Split(Tail(curline), sface, " ");
	for (int i = 0; i < int(sface.size()); i++)
	{
		//判断有几个/
		int vtype;
		
		Split(sface[i], svert, "/");

		if (svert.size() == 1)
		{
			vtype = 1;
		}
		if (svert.size() == 2)
		{
			vtype = 3;
		}
		//顺序应党是位置/贴图/法线，但有的不是
		if (svert.size() == 3)
		{
			if (svert[1] != "")
			{
				vtype = 4;
			}
			else
			{
				vtype = 2;
			}
		}

		switch (vtype)
		{
		case 1: 
		{
			Vertex ver;
			ver.Pos.x = pos[std::stof(svert[0]) - 1].x;
			ver.Pos.y = pos[std::stof(svert[0]) - 1].y;
			ver.Pos.z = pos[std::stof(svert[0]) - 1].z;
			m_Vertices.push_back(ver);
			break;
		}
		case 2: // P/T
		{
			Vertex ver;
			ver.Pos.x = pos[std::stof(svert[0]) - 1].x;
			ver.Pos.y = pos[std::stof(svert[0]) - 1].y;
			ver.Pos.z = pos[std::stof(svert[0]) - 1].z;
			ver.TexC.x = texC[std::stof(svert[1]) - 1].x;
			ver.TexC.y = texC[std::stof(svert[1]) - 1].y;
			m_Vertices.push_back(ver);
			break;
		}
		case 3: // P//N
		{
			Vertex ver;
			ver.Pos.x = pos[std::stof(svert[0]) - 1].x;
			ver.Pos.y = pos[std::stof(svert[0]) - 1].y;
			ver.Pos.z = pos[std::stof(svert[0]) - 1].z;
			ver.Normal.x = normals[std::stof(svert[1]) - 1].x;
			ver.Normal.y = normals[std::stof(svert[1]) - 1].y;
			ver.Normal.z = normals[std::stof(svert[1]) - 1].z;
			m_Vertices.push_back(ver);
			break;
		}
		case 4: // P/T/N
		{
			Vertex ver;
			ver.Pos.x = pos[std::stof(svert[0]) - 1].x;
			ver.Pos.y = pos[std::stof(svert[0]) - 1].y;
			ver.Pos.z = pos[std::stof(svert[0]) - 1].z;
			ver.TexC.x = texC[std::stof(svert[1]) - 1].x;
			ver.TexC.y = texC[std::stof(svert[1]) - 1].y;
			ver.Normal.x = normals[std::stof(svert[2]) - 1].x;
			ver.Normal.y = normals[std::stof(svert[2]) - 1].y;
			ver.Normal.z = normals[std::stof(svert[2]) - 1].z;
			m_Vertices.push_back(ver);
			break;
		}
		default:
		{
			break;
		}
		}
		m_Indices.push_back(m_IndiceSize);
		m_IndiceSize++;
	}

	//没有法线的时候可以计算面法线
	//if (noNormal)
	//{
	//	Vector3f A = oVerts[0].position_ - oVerts[1].position_;
	//	Vector3f B = oVerts[2].position_ - oVerts[1].position_;
	//	Vector3f normal = cross(A, B);
	//	for (int i = 0; i < int(oVerts.size()); i++)
	//	{
	//		oVerts[i].normal_ = normal;
	//	}
	//}
}
void LoadObject::LoadOBJ(std::string fileName)
{
	m_IndiceSize = 0;
	m_Indices.clear();
	m_Vertices.clear();
	std::ifstream file(fileName);

	if (!file.is_open())
	{
		MessageBox(0, d3dUtil::String2Wstring(fileName).c_str(), 0, 0);
		return;
	}

	std::vector <DirectX::XMFLOAT3> pos;
	std::vector <DirectX::XMFLOAT3> normals;
	std::vector <DirectX::XMFLOAT2> texC;

	std::string curline;
	while (std::getline(file, curline))
	{
		//顶点
		if (FirstToken(curline) == "v")
		{
			std::vector<std::string> spos;
			DirectX::XMFLOAT3 vpos;
			Split(Tail(curline), spos, " ");

			vpos.x= std::stof(spos[0]);
			vpos.y = std::stof(spos[1]);
			vpos.z = std::stof(spos[2]);

			pos.push_back(vpos);
		}
		//纹理映射
		if (FirstToken(curline) == "vt")
		{
			std::vector<std::string> stex;
			DirectX::XMFLOAT2 vtex;
			Split(Tail(curline), stex, " ");

			vtex.x= std::stof(stex[0]);
			vtex.y = std::stof(stex[1]);

			texC.push_back(vtex);
		}
		//法线
		if (FirstToken(curline) == "vn")
		{
			std::vector<std::string> snor;
			DirectX::XMFLOAT3 vnor;
			Split(Tail(curline), snor, " ");

			vnor.x = std::stof(snor[0]);
			vnor.y = std::stof(snor[1]);
			vnor.z = std::stof(snor[2]);

			normals.push_back(vnor);
		}
		if (FirstToken(curline) == "f")
		{
			std::vector<Vertex> vVerts;
			IndicesCreate(curline,pos,normals,texC);
		}
	}

}