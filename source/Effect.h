#pragma once

using namespace dae;

class Effect
{
public:
	Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
	virtual ~Effect();

	Effect(const Effect& other) = delete;
	Effect& operator=(const Effect& rhs) = delete;
	Effect(Effect&& other) = delete;
	Effect& operator=(Effect&& rhs) = delete;

	virtual ID3DX11Effect* GetEffect() const { return m_pEffect; }
	
	virtual ID3DX11EffectTechnique* GetDefaultTechnique() const { return m_pTechnique; }

	virtual void SetWorldViewProjectionMatrixVariable(const Matrix& newValue) const
	{
		m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&newValue));
	}

	virtual void SetDiffuseMap(const std::string& filePath, ID3D11Device* pDevice) const;
	virtual void SetSamplerState(ID3D11SamplerState* newSamplerState);
	virtual void SetRasterizerState(ID3D11RasterizerState* newRasterizerState);
protected:
	ID3DX11Effect* m_pEffect;

	ID3DX11EffectTechnique* m_pTechnique;
	ID3DX11EffectSamplerVariable* m_pSamplerVariable;
	ID3DX11EffectRasterizerVariable* m_pRasterizerVariable;
	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable;
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;

	virtual void InitMatrixVariables() {}
	virtual void InitShaderResourceVariables() {}

	static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
};