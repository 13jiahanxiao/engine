#include"Light.h"
#include"../Utility/MathHelper.h"

DirectionLight::DirectionLight():
	m_SunTheta(0),
	m_SunPhi(0),
	m_LightIntensity(0),
	m_LightStrength({ 0,0,0 }),
	m_SinFactor(0)
{

}

DirectionLight::DirectionLight(float theta, float phi, float intensity, DirectX::XMFLOAT3 strength, float sin):
	m_SunTheta(theta),
	m_SunPhi(phi),
	m_LightIntensity(intensity),
	m_LightStrength(strength),
	m_SinFactor(sin)
{

}

DirectionLight::~DirectionLight()
{

}

void DirectionLight::SetLight(float theta, float phi, float intensity, DirectX::XMFLOAT3 strength, float sin)
{
	m_SunTheta = theta;
	m_SunPhi = phi;
	m_LightIntensity = intensity;
	m_LightStrength = strength;
	m_SinFactor = sin;
}

DirectX::XMVECTOR DirectionLight::GetLightDir() 
{
	return MathHelper::SphericalToCartesian(m_LightIntensity, m_SunTheta, m_SunPhi);
}

DirectX::XMFLOAT3 DirectionLight::GetLightStrength()
{
	return m_LightStrength;
}

void DirectionLight::UpdateTheta(float factor)
{
	m_SunTheta += factor;
}

void DirectionLight::UpdatePhi(float factor)
{
	m_SunPhi += factor;
	m_SunPhi = MathHelper::Clamp(m_SunPhi, 0.1f, DirectX::XM_PIDIV2);
}