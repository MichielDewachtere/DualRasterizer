#pragma once
#include "Effect.h"

class ShadedEffect final : public Effect
{
public:
	ShadedEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
	virtual ~ShadedEffect() override;

	ShadedEffect(const ShadedEffect& other) = delete;
	ShadedEffect& operator=(const ShadedEffect& rhs) = delete;
	ShadedEffect(ShadedEffect&& other) = delete;
	ShadedEffect& operator=(ShadedEffect&& rhs) = delete;

	void SetWorldMatrixVariable(const Matrix& newValue) const
	{
		m_pMatWorldVariable->SetMatrix(reinterpret_cast<const float*>(&newValue));
	}
	void SetInvViewMatrixVariable(const Matrix& newValue) const
	{
		m_pMatInvViewVariable->SetMatrix(reinterpret_cast<const float*>(&newValue));
	}

	void SetNormalMap(const std::string& filePath, ID3D11Device* pDevice) const;
	void SetSpecularMap(const std::string& filePath, ID3D11Device* pDevice) const;
	void SetGlossinessMap(const std::string& filePath, ID3D11Device* pDevice) const;

private:
	ID3DX11EffectMatrixVariable* m_pMatWorldVariable;
	ID3DX11EffectMatrixVariable* m_pMatInvViewVariable;

	ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pGlossinessMapVariable;

	virtual void InitMatrixVariables() override;
	virtual void InitShaderResourceVariables() override;
};

