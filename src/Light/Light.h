#pragma once
#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <DirectXMath.h>

#define MaxLights 16

struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
    float FalloffStart = 1.0f;                          // point/spot light only
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
    float FalloffEnd = 10.0f;                           // point/spot light only
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
    float SpotPower = 64.0f;                            // spot light only
};



class DirectionLight
{
public:
    DirectionLight();
    DirectionLight(float theta,float phi,float intensity, DirectX::XMFLOAT3 strength,float sin);
    ~DirectionLight();
    void SetLight(float theta, float phi, float intensity, DirectX::XMFLOAT3 strength, float sin);
	
    DirectX::XMVECTOR GetLightDir();
    DirectX::XMFLOAT3 GetLightStrength();

    void UpdateTheta(float factor);
    void UpdatePhi(float factor);

private:
    float m_SunTheta ;
    float m_SunPhi;
    float m_LightIntensity ;
    DirectX::XMFLOAT3 m_LightStrength ;
    float m_SinFactor;
};

