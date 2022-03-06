//#pragma once
//#include"../Common/Metalib.h"
//#include <memory>
//#include <DirectXMath.h>
//#include <string>
//#include <vector>
//#include <span>
//#include <d3d12.h>
//namespace utility
//{
//	struct VarTypeData
//	{
//		enum class ScaleType :vbyte
//		{
//			FLOAT,
//			INT,
//			UINT,
//		};
//
//		ScaleType scale;
//		vbyte dimension;
//		uint semanticIndex;
//		std::string semantic;
//		size_t GetSize()const;
//	};
//
//	class VarTypeBase;
//	class TypeStruct
//	{
//		friend class  VarTypeBase;
//		std::vector<VarTypeData> variables;
//	public:
//		std::span<VarTypeData const> Variables() const { return variables; }
//		size_t structSize = 0;
//		TypeStruct();
//		TypeStruct(TypeStruct const&) = delete;
//		TypeStruct(TypeStruct&&) = default;
//		void GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const;
//	};
//
//	class VarTypeBase
//	{
//	public:
//		size_t Offset()const { return offset; }
//	protected:
//		VarTypeBase(VarTypeData&& varData);
//	private:
//		size_t offset;
//	};
//
//	template<typename T>
//	class VarType :public VarTypeBase
//	{
//	public:
//		T const& Get(void const* structPtr)const
//		{
//			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
//			return *reinterpret_cast<T*>(ptrNum + Offset());
//		}
//		T& Get(void* structPtr) const {
//			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
//			return *reinterpret_cast<T*>(ptrNum + Offset());
//		}
//	protected:
//		VarType(VarTypeData&& varData) : VarTypeBase(std::move(varData)) {}
//	};
//
//	template<typename T>
//	struct Var {};
//
//	//Ä£°åÌØ»¯
//	template<>
//	struct Var<int32> :public VarType<int32>
//	{
//		Var(char const* semantic) :VarType<int32>(
//			VarTypeData{ VarTypeData::ScaleType::INT, vbyte(1), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<uint> :public VarType<uint>
//	{
//		Var(char const* semantic) : VarType<uint>(VarTypeData{ VarTypeData::ScaleType::UINT, vbyte(1), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<float> :public VarType<float>
//	{
//		Var(char const* semantic) : VarType<uint>(VarTypeData{ VarTypeData::ScaleType::FLOAT, vbyte(1), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMFLOAT2> :public VarType<DirectX::XMFLOAT2>
//	{
//		Var(char const* semantic) : VarType<DirectX::XMFLOAT2>(VarTypeData{ VarTypeData::ScaleType::FLOAT, vbyte(2), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMFLOAT3> :public VarType<DirectX::XMFLOAT3>
//	{
//		Var(char const* semantic) : VarType<DirectX::XMFLOAT3>(VarTypeData{ VarTypeData::ScaleType::FLOAT, vbyte(3), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMFLOAT4> :public VarType<DirectX::XMFLOAT4>
//	{
//		Var(char const* semantic) : VarType<DirectX::XMFLOAT4>(VarTypeData{ VarTypeData::ScaleType::FLOAT, vbyte(4), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMUINT2> : public VarType<DirectX::XMUINT2> {
//		Var(char const* semantic) : VarType<DirectX::XMUINT2>(VarTypeData{ VarTypeData::ScaleType::UINT, vbyte(2), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMUINT3> : public VarType<DirectX::XMUINT3> {
//		Var(char const* semantic) : VarType<DirectX::XMUINT3>(VarTypeData{ VarTypeData::ScaleType::UINT, vbyte(3), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMUINT4> : public VarType<DirectX::XMUINT4> {
//		Var(char const* semantic) : VarType<DirectX::XMUINT4>(VarTypeData{ VarTypeData::ScaleType::UINT, vbyte(4), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMINT2> : public VarType<DirectX::XMINT2> {
//		Var(char const* semantic) : VarType<DirectX::XMINT2>(VarTypeData{ VarTypeData::ScaleType::INT, vbyte(2), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMINT3> : public VarType<DirectX::XMINT3> {
//		Var(char const* semantic) : VarType<DirectX::XMINT3>(VarTypeData{ VarTypeData::ScaleType::INT, vbyte(3), uint(0), std::string(semantic) }) {}
//	};
//
//	template<>
//	struct Var<DirectX::XMINT4> : public VarType<DirectX::XMINT4> {
//		Var(char const* semantic) : VarType<DirectX::XMINT4>(VarTypeData{ VarTypeData::ScaleType::INT, vbyte(4), uint(0), std::string(semantic) }) {}
//	};
//}