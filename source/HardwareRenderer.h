#pragma once
#include "Renderer.h"

class HardwareRenderer final
{
public:
	HardwareRenderer();
	virtual ~HardwareRenderer();

	HardwareRenderer(const HardwareRenderer&) = delete;
	HardwareRenderer(HardwareRenderer&&) noexcept = delete;
	HardwareRenderer& operator=(const HardwareRenderer&) = delete;
	HardwareRenderer& operator=(HardwareRenderer&&) noexcept = delete;

	void Update(const Timer* pTimer);
	void Render() const;
private:




};

