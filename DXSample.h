#pragma once

#include"DXSampleHelper.h"
#include"Win32Application.h"

using Microsoft::WRL::ComPtr;

class DXSample
{
public:
	DXSample(uint width, uint height, std::wstring name);
	virtual ~DXSample();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual void OnKeyDown(UINT8 key) {}
	virtual void OnKeyUp(UINT8 key) {}
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

	uint GetWidth() const { return m_width; }
	uint GetHeight()const { return m_height; }
	const wchar_t* GetTitle() const { return m_title.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc)wchar_t* argv[], int argc);
protected:

	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerfermanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);
	uint m_width;
	uint m_height;
	float m_aspectRatio;
	bool m_useWarpDevice;
private:
	// Root assets path.
	std::wstring m_assetsPath;

	// Window title.
	std::wstring m_title;
};