#pragma once
#include "Effect.h"

class TransEffect final : public Effect
{
public:
	TransEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
		: Effect(pDevice, assetFile) {}
	virtual ~TransEffect() override = default;

	TransEffect(const TransEffect& other) = delete;
	TransEffect& operator=(const TransEffect& rhs) = delete;
	TransEffect(TransEffect&& other) = delete;
	TransEffect& operator=(TransEffect&& rhs) = delete;
};