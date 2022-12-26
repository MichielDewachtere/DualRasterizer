#include "pch.h"
#include "Renderer.h"

#include <ranges>

#include "Camera.h"
#include "ShadedEffect.h"
#include "TransEffect.h"
#include "Utils.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		m_pMeshToShadedEffectMap = new std::map<Mesh*, ShadedEffect*>;
		m_pMeshToTransEffectMap = new std::map<Mesh*, TransEffect*>;

		VehicleMeshInit();
		CombustionMeshInit();

		m_pCamera = new Camera();
		m_pCamera->Initialize(45.f, Vector3{ 0.f,0.f,0.f }, m_Width / (float)m_Height);

		m_BackGroundColor = ColorRGB{ 99 / 255.f,150 / 255.f,237 / 255.f };
	}

	Renderer::~Renderer()
	{
		for (const auto& [mesh, shadedEffect] : *m_pMeshToShadedEffectMap)
		{
			delete shadedEffect;
			delete mesh;
		}
		delete m_pMeshToShadedEffectMap;

		for (const auto& [mesh, shadedEffect] : *m_pMeshToTransEffectMap)
		{
			delete shadedEffect;
			delete mesh;
		}
		delete m_pMeshToTransEffectMap;

		delete m_pCamera;

		//Release resources (to prevent resource leaks)
		if (m_pSamplerState) m_pSamplerState->Release();

		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
		}

		if (m_pRenderTargetBuffer)
		{
			m_pRenderTargetBuffer->Release();
		}

		if (m_pDepthStencilView)
			m_pDepthStencilView->Release();

		if (m_pDepthStencilBuffer)
			m_pDepthStencilBuffer->Release();

		if (m_pSwapChain)
			m_pSwapChain->Release();

		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}

		if (m_pDevice)
		{
			m_pDevice->Release();
		}

		if (m_pDXGIFactory)
			m_pDXGIFactory->Release();
	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_pCamera->Update(pTimer);

		const auto viewMatrix = m_pCamera->GetViewMatrix();
		const auto projectionMatrix = m_pCamera->GetProjectionMatrix();
		const auto invViewMatrix = m_pCamera->GetInvViewMatrix();

		for (const auto& [mesh, shadedEffect] : *m_pMeshToShadedEffectMap)
		{
			if (m_IsRotating)
			{
				constexpr float rotationSpeed{ 30 * TO_RADIANS };
				const Matrix newWorldMatrix = Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed()) * mesh->GetWorldMatrix();
				mesh->SetWorldMatrix(newWorldMatrix);
				shadedEffect->SetWorldMatrixVariable(newWorldMatrix);
			}

			mesh->SetWorldViewProjectionMatrix(viewMatrix, projectionMatrix);
			shadedEffect->SetInvViewMatrixVariable(invViewMatrix);
		}

		if (m_DisplayFireFX == true)
		{
			for (const auto& mesh : *m_pMeshToTransEffectMap | std::views::keys)
			{
				if (m_IsRotating)
				{
					constexpr float rotationSpeed{ 30 * TO_RADIANS };
					const Matrix newWorldMatrix = Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed()) * mesh->GetWorldMatrix();
					mesh->SetWorldMatrix(newWorldMatrix);
				}

				mesh->SetWorldViewProjectionMatrix(viewMatrix, projectionMatrix);
			}
		}
	}

	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		//Clear Buffers
		//constexpr auto clearColor = ColorRGB(0.f, 0.f, 0.3f);
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &m_BackGroundColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//Render
		for (const auto& mesh : *m_pMeshToShadedEffectMap | std::views::keys)
		{
			mesh->Render(m_pDeviceContext);
		}

		if (m_DisplayFireFX == true)
		{
			for (const auto& mesh : *m_pMeshToTransEffectMap | std::views::keys)
			{
				mesh->Render(m_pDeviceContext);
			}
		}

		//Present
		m_pSwapChain->Present(0, 0);
	}
	
	void Renderer::ToggleRotation()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_IsRotating == true)
		{
			m_IsRotating = false;
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Rotation OFF\n";
			SetConsoleTextAttribute(h, 7);
		}
		else if (m_IsRotating == false)
		{
			m_IsRotating = true;
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Rotation ON\n";
			SetConsoleTextAttribute(h, 7);
		}

	}
	void Renderer::ToggleFireFx()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_DisplayFireFX == true)
		{
			m_DisplayFireFX = false;
			SetConsoleTextAttribute(h, 2);
			std::cout << "**(HARDWARE) FireFX OFF\n";
			SetConsoleTextAttribute(h, 7);
		}
		else if (m_DisplayFireFX == false)
		{
			m_DisplayFireFX = true;
			SetConsoleTextAttribute(h, 2);
			std::cout << "**(HARDWARE) FireFX ON\n";
			SetConsoleTextAttribute(h, 7);
		}
	}
	void Renderer::ToggleSamplerState()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

		D3D11_FILTER filter{};

		switch (m_CurrentSamplerState)
		{
		case SamplerState::point:
			SetConsoleTextAttribute(h, 2);
			std::cout << "**(HARDWARE) Sampler Filter = POINT\n";
			SetConsoleTextAttribute(h, 7);

			filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;

			m_CurrentSamplerState = SamplerState::linear;
			break;
		case SamplerState::linear:
			SetConsoleTextAttribute(h, 2);
			std::cout << "**(HARDWARE) Sampler Filter = LINEAR\n";
			SetConsoleTextAttribute(h, 7);

			filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;

			m_CurrentSamplerState = SamplerState::anisotropic;
			break;
		case SamplerState::anisotropic:
			SetConsoleTextAttribute(h, 2);
			std::cout << "**(HARDWARE) Sampler Filter = ANISOTROPIC\n";
			SetConsoleTextAttribute(h, 7);

			filter = D3D11_FILTER_ANISOTROPIC;

			m_CurrentSamplerState = SamplerState::point;
			break;
		}

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MaxAnisotropy = 1;
		
		if (m_pSamplerState)
			m_pSamplerState->Release();

		HRESULT hr{ m_pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState) };
		if (FAILED(hr)) std::wcout << L"m_pSampleState failed to load\n";

		for (const auto& effect : *m_pMeshToShadedEffectMap | std::views::values)
		{
			effect->SetSamplerState(m_pSamplerState);
		}
		for (const auto& effect : *m_pMeshToTransEffectMap | std::views::values)
		{
			effect->SetSamplerState(m_pSamplerState);
		}
	}
	void Renderer::ToggleCullMode()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

		D3D11_CULL_MODE cullMode{};

		switch (m_CurrentCullMode)
		{
		case CullMode::back:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = BACK\n";
			SetConsoleTextAttribute(h, 7);

			cullMode = D3D11_CULL_BACK;

			m_CurrentCullMode = CullMode::front;
			break;
		case CullMode::front:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = FRONT\n";
			SetConsoleTextAttribute(h, 7);

			cullMode = D3D11_CULL_FRONT;

			m_CurrentCullMode = CullMode::none;
			break;
		case CullMode::none:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = NONE\n";
			SetConsoleTextAttribute(h, 7);

			cullMode = D3D11_CULL_NONE;

			m_CurrentCullMode = CullMode::back;
			break;
		}

		D3D11_RASTERIZER_DESC desc;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = cullMode;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthBiasClamp = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.ScissorEnable = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		if (m_pRasterizerState) m_pRasterizerState->Release();

		HRESULT hr{ m_pDevice->CreateRasterizerState(&desc, &m_pRasterizerState) };
		if (FAILED(hr)) std::wcout << L"m_pRasterizerState failed to load\n";

		for (const auto& effect : *m_pMeshToShadedEffectMap | std::views::values)
		{
			effect->SetRasterizerState(m_pRasterizerState);
		}
		for (const auto& effect : *m_pMeshToTransEffectMap | std::views::values)
		{
			effect->SetRasterizerState(m_pRasterizerState);
		}

	}
	void Renderer::ToggleUniformClearColor()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_UseUniformClearColor == true)
		{
			m_UseUniformClearColor = false;

			m_BackGroundColor = ColorRGB{ 99 / 255.f,150 / 255.f,237 / 255.f };

			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Uniform ClearColor OFF\n";
			SetConsoleTextAttribute(h, 7);
		}
		else if (m_UseUniformClearColor == false)
		{
			m_UseUniformClearColor = true;

			m_BackGroundColor = ColorRGB{ 26 / 255.f,26 / 255.f,26 / 255.f };

			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Uniform ClearColor ON\n";
			SetConsoleTextAttribute(h, 7);
		}
	}

	void Renderer::VehicleMeshInit() const
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("resources/vehicle.obj", vertices, indices) == false)
			std::cout << ".obj not found\n";

		auto pShadedEffect = new ShadedEffect{ m_pDevice, L"resources/Vehicle.fx" };
		pShadedEffect->SetDiffuseMap("resources/vehicle_diffuse.png", m_pDevice);
		pShadedEffect->SetNormalMap("resources/vehicle_normal.png", m_pDevice);
		pShadedEffect->SetSpecularMap("resources/vehicle_specular.png", m_pDevice);
		pShadedEffect->SetGlossinessMap("resources/vehicle_gloss.png", m_pDevice);
		
		const Vector3 position{ 0,0,50 };
		const Vector3 rotation{ 0,0,0 };
		const Vector3 scale{ 1,1,1 };
		const Matrix worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);

		pShadedEffect->SetWorldMatrixVariable(worldMatrix);

		auto pMesh = new Mesh{ m_pDevice, vertices, indices, pShadedEffect };
		pMesh->SetWorldMatrix(worldMatrix);

		std::pair<Mesh*, ShadedEffect*> pair(pMesh, pShadedEffect);

		m_pMeshToShadedEffectMap->insert(pair);
	}
	void Renderer::CombustionMeshInit() const
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("resources/fireFX.obj", vertices, indices) == false)
			std::cout << ".obj not found\n";

		auto pTransEffect = new TransEffect{ m_pDevice, L"resources/Fire.fx" };
		pTransEffect->SetDiffuseMap("resources/fireFX_diffuse.png", m_pDevice);

		const Vector3 position{ 0,0,50 };
		const Vector3 rotation{ 0,0,0 };
		const Vector3 scale{ 1,1,1 };
		const Matrix worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);

		auto pMesh = new Mesh{ m_pDevice, vertices, indices, pTransEffect };
		pMesh->SetWorldMatrix(worldMatrix);

		m_pMeshToTransEffectMap->insert(std::make_pair(pMesh, pTransEffect));
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//Create Device and DeviceContext, using hardware acceleration
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		uint32_t createDeviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(result))
			return result;

		//Create DXGI Factory to create SwapChain based on hardware
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&m_pDXGIFactory));
		if (FAILED(result)) return result;

		// Create the swapchain description
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle HWND from the SDL backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version)
			SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		//Create SwapChain and hook it into the handle of the SDL window
		result = m_pDXGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		//Create the Depth/Stencil Buffer and View
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		//Create the Stencil Buffer
		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;

		//Create the Stencil View
		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;

		//Create the RenderTargetView
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result))
			return result;

		//Bind the Views to the Output Merger Stage
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//Set the Viewport
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<FLOAT>(m_Width);
		viewport.Height = static_cast<FLOAT>(m_Height);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return S_OK;
	}
}
