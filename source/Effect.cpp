#include "pch.h"
#include "Effect.h"

#include "Texture.h"

#include <sstream>

using namespace dae;

Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	m_pEffect = LoadEffect(pDevice, assetFile);

	m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
	if (!m_pTechnique->IsValid()) std::wcout << L"DefaultTechnique not valid\n";

	m_pSamplerVariable = m_pEffect->GetVariableByName("gSamState")->AsSampler();
	if (m_pSamplerVariable->IsValid() == false)
		std::wcout << L"m_pSamplerVariable is not valid\n";

	m_pRasterizerVariable = m_pEffect->GetVariableByName("gRasterizerState")->AsRasterizer();
	if (m_pRasterizerVariable->IsValid() == false)
		std::wcout << L"m_pRasterizerVariable is not valid\n";

	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (m_pMatWorldViewProjVariable->IsValid() == false)
		std::wcout << L"m_pMatWorldViewProjVariable not valid!\n";

	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (!m_pDiffuseMapVariable->IsValid())
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
}

Effect::~Effect()
{
	if (m_pDiffuseMapVariable) m_pDiffuseMapVariable->Release();
	if (m_pMatWorldViewProjVariable) m_pMatWorldViewProjVariable->Release();
	if (m_pRasterizerVariable) m_pRasterizerVariable->Release();
	if (m_pSamplerVariable) m_pSamplerVariable->Release();
	
	if (m_pTechnique) m_pTechnique->Release();

	if (m_pEffect) m_pEffect->Release();
}

void Effect::SetDiffuseMap(const Texture* diffuseTexture) const
{
	if (m_pDiffuseMapVariable)
		m_pDiffuseMapVariable->SetResource(diffuseTexture->GetShaderResourceView());
}

void Effect::SetSamplerState(ID3D11SamplerState* newSamplerState)
{
	m_pSamplerVariable->SetSampler(0, newSamplerState);
}

void Effect::SetRasterizerState(ID3D11RasterizerState* newRasterizerState)
{
	m_pRasterizerVariable->SetRasterizerState(0, newRasterizerState);
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect{};

	DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile
	(
		assetFile.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob
	);

	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

			std::wstringstream ss;
			for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); ++i)
			{
				ss << pErrors[i];
			}

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << "\n";
		}
		else
		{
			std::wstringstream ss;
			ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
			std::wcout << ss.str() << "\n";
			return nullptr;
		}
	}

	return pEffect;
}
