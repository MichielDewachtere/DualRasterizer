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
class HardwareRenderer;

namespace dae
{
	class Texture;

	class Renderer
	{
	public:
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
		void CycleSamplerState();
		void CycleCullMode();
		void ToggleUniformClearColor();
		void ToggleRasterizerMode();
		void CycleShadingMode();
		void ToggleNormalMap();
		void ToggleDepthBufferVisualization();
		void ToggleBoundingBoxVisualization();

	private:
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
		enum class ShadingMode
		{
			observedArea,
			diffuse,
			specular,
			combined
		};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//KeyBind Variables
		bool m_UseHardware{ true };
		bool m_IsRotating{ false };

		CullMode m_CurrentCullMode{ CullMode::back };

		bool m_UseUniformClearColor{ true };
		ColorRGB m_BackGroundColor;

		ShadingMode m_CurrentShadingMode{ ShadingMode::combined };

		bool m_EnableNormalMap{ true };
		bool m_DepthBufferVisualization{ false };
		bool m_BoundingBoxVisualization{ false };

		SamplerState m_CurrentSamplerState{ SamplerState::point };

		bool m_DisplayFireFX{ true };
		//////////////
		
		Camera* m_pCamera;

		std::map<Mesh*, ShadedEffect*>* m_pMeshToShadedEffectMap;
		std::map<Mesh*, TransEffect*>* m_pMeshToTransEffectMap;

		Texture* m_pVehicleDiffuse;
		Texture* m_pVehicleNormalMap;
		Texture* m_pVehicleGlossinessMap;
		Texture* m_pVehicleSpecularMap;

		Texture* m_pFireFXDiffuse;

		void VehicleMeshInit();
		void CombustionMeshInit();

		//SOFTWARE
		void RenderTriangleList(Mesh& mesh) const;
		void VertexTransformationFunction(Mesh& meshes) const;
		void PixelShading(VertexOut& v) const;

		bool IsInFrustum(const VertexOut& v) const;
		void NDCToRaster(VertexOut& v) const;

		void ClearBackground() const;

		//DIRECTX - HARDWARE
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


	//private:
	//	HardwareRenderer* m_pHardwareRenderer;
	};
}
