#pragma once
#include "Mesh.h"
#include <map>

struct SDL_Window;
struct SDL_Surface;

class Camera;
class Mesh;
class Effect;
class ShadedEffect;
class TransEffect;

namespace dae
{
	class Renderer final
	{
	public:
		enum class SamplerState
		{
			point,
			linear,
			anisotropic
		};
		enum class CullMode
		{
			back,
			front,
			none
		};

		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		void ToggleRotation();
		void ToggleFireFx();
		void ToggleSamplerState();
		void ToggleCullMode();
		void ToggleUniformClearColor();

	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };
		bool m_IsRotating{ false };
		bool m_DisplayFireFX{ true };

		SamplerState m_CurrentSamplerState{ SamplerState::point };
		CullMode m_CurrentCullMode{ CullMode::back };

		bool m_UseUniformClearColor{ true };
		ColorRGB m_BackGroundColor;

		Camera* m_pCamera;

		std::map<Mesh*, ShadedEffect*>* m_pMeshToShadedEffectMap;
		std::map<Mesh*, TransEffect*>* m_pMeshToTransEffectMap;

		void VehicleMeshInit() const;
		void CombustionMeshInit() const;
		
		//DIRECTX
		HRESULT InitializeDirectX();

		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;

		IDXGIFactory* m_pDXGIFactory;

		IDXGISwapChain* m_pSwapChain;

		ID3D11Resource* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;

		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;

		ID3D11SamplerState* m_pSamplerState;
		ID3D11RasterizerState* m_pRasterizerState;
	};
}
