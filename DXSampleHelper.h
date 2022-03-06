#pragma once
#include<stdexcept>
#include "Common/d3dUtil.h"
#include"Metalib.h"
inline std::string HrToString(HRESULT hr) 
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<uint>(hr));
	return std::string(s_str);
}

inline void GetAssetsPath(_Out_writes_(pathSize) wchar_t* path, uint pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}