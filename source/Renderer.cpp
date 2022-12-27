#include "pch.h"
#include "Renderer.h"

#include <ranges>

#include "BRDF.h"
#include "Camera.h"
#include "TransEffect.h"
#include "ShadedEffect.h"
#include "Utils.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[(int)(m_Width * m_Height)];

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

		if (m_UseHardware)
		{
			CombustionMeshInit();
		}

		m_pCamera = new Camera();
		m_pCamera->Initialize(45.f, Vector3{ 0.f,0.f,0.f }, m_Width / (float)m_Height);

		m_BackGroundColor = ColorRGB{ 99 / 255.f,150 / 255.f,237 / 255.f };
	}

	Renderer::~Renderer()
	{
		delete[] m_pDepthBufferPixels;

		delete m_pVehicleDiffuse;
		delete m_pVehicleNormalMap;
		delete m_pVehicleGlossinessMap;
		delete m_pVehicleSpecularMap;
		delete m_pFireFXDiffuse;

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
		constexpr float rotationSpeed{ 45 * TO_RADIANS };

		for (const auto& [mesh, shadedEffect] : *m_pMeshToShadedEffectMap)
		{
			if (m_IsRotating)
			{
				const Matrix newWorldMatrix = Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed()) * mesh->GetWorldMatrix();
				mesh->SetWorldMatrix(newWorldMatrix);

				if (m_UseHardware)
					shadedEffect->SetWorldMatrixVariable(newWorldMatrix);
			}

			mesh->SetWorldViewProjectionMatrix(viewMatrix, projectionMatrix);
			if (m_UseHardware)
				shadedEffect->SetInvViewMatrixVariable(invViewMatrix);
		}

		for (const auto& mesh : *m_pMeshToTransEffectMap | std::views::keys)
		{
			if (m_IsRotating)
			{
				const Matrix newWorldMatrix = Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed()) * mesh->GetWorldMatrix();
				mesh->SetWorldMatrix(newWorldMatrix);
			}

			mesh->SetWorldViewProjectionMatrix(viewMatrix, projectionMatrix);
		}
	}
	void Renderer::Render() const
	{
		if (m_UseHardware)
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
		else
		{
			SDL_LockSurface(m_pBackBuffer);

			std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

			ClearBackground();

			for (const auto& mesh : *m_pMeshToShadedEffectMap | std::views::keys)
			{
				VertexTransformationFunction(*mesh);

				RenderTriangleList(*mesh);
			}

			//@END
			//Update SDL Surface
			SDL_UnlockSurface(m_pBackBuffer);
			SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
			SDL_UpdateWindowSurface(m_pWindow);
		}
	}

	//SHARED
	void Renderer::ToggleRasterizerMode()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_UseHardware == true)
		{
			m_UseHardware = false;
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Rasterizer Mode = SOFTWARE\n";
			SetConsoleTextAttribute(h, 7);

			m_BackGroundColor = ColorRGB{ 100 / 255.f,100 / 255.f ,100 / 255.f };
		}
		else if (m_UseHardware == false)
		{
			m_UseHardware = true;
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) Rasterizer Mode = HARDWARE\n";
			SetConsoleTextAttribute(h, 7);

			m_BackGroundColor = ColorRGB{ 99 / 255.f,150 / 255.f,237 / 255.f };
		}
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
	void Renderer::CycleCullMode()
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

		D3D11_CULL_MODE cullMode{};

		switch (m_CurrentCullMode)
		{
		case CullMode::back:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = FRONT\n";
			SetConsoleTextAttribute(h, 7);

			cullMode = D3D11_CULL_BACK;

			m_CurrentCullMode = CullMode::front;
			break;
		case CullMode::front:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = NONE\n";
			SetConsoleTextAttribute(h, 7);

			cullMode = D3D11_CULL_FRONT;

			m_CurrentCullMode = CullMode::none;
			break;
		case CullMode::none:
			SetConsoleTextAttribute(h, 6);
			std::cout << "**(SHARED) CullMode = BACK\n";
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

			if (m_UseHardware == true)
				m_BackGroundColor = ColorRGB{ 99 / 255.f,150 / 255.f,237 / 255.f };
			else
				m_BackGroundColor = ColorRGB{ 100 / 255.f,100 / 255.f ,100 / 255.f };

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

	//SOFTWARE
	void Renderer::CycleShadingMode()
	{
		if (m_UseHardware) return;

		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		switch (m_CurrentShadingMode)
		{
		case ShadingMode::observedArea:
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) Shading Mode = DIFFUSE\n";
			SetConsoleTextAttribute(h, 7);

			m_CurrentShadingMode = ShadingMode::diffuse;
			break;
		case ShadingMode::diffuse:
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) Shading Mode = SPECULAR\n";
			SetConsoleTextAttribute(h, 7);

			m_CurrentShadingMode = ShadingMode::specular;

			break;
		case ShadingMode::specular:
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) Shading Mode = COMBINED\n";
			SetConsoleTextAttribute(h, 7);

			m_CurrentShadingMode = ShadingMode::combined;

			break;
		case ShadingMode::combined:
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) Shading Mode = OBSERVED AREA\n";
			SetConsoleTextAttribute(h, 7);

			m_CurrentShadingMode = ShadingMode::observedArea;
			break;
		}
	}
	void Renderer::ToggleNormalMap()
	{
		if (m_UseHardware) return;

		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_EnableNormalMap == true)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) NormalMap OFF\n";
			SetConsoleTextAttribute(h, 7);

			m_EnableNormalMap = false;
		}
		else if (m_EnableNormalMap == false)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) NormalMap ON\n";
			SetConsoleTextAttribute(h, 7);

			m_EnableNormalMap = true;
		}
	}
	void Renderer::ToggleDepthBufferVisualization()
	{
		if (m_UseHardware) return;

		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_DepthBufferVisualization == true)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) DepthBuffer Visualization OFF\n";
			SetConsoleTextAttribute(h, 7);

			m_DepthBufferVisualization = false;
		}
		else if (m_DepthBufferVisualization == false)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) DepthBuffer Visualization ON\n";
			SetConsoleTextAttribute(h, 7);

			m_DepthBufferVisualization = true;
		}
	}
	void Renderer::ToggleBoundingBoxVisualization()
	{
		if (m_UseHardware) return;

		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_BoundingBoxVisualization == true)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) DepthBuffer Visualization OFF\n";
			SetConsoleTextAttribute(h, 7);

			m_BoundingBoxVisualization = false;
		}
		else if (m_BoundingBoxVisualization == false)
		{
			SetConsoleTextAttribute(h, 5);
			std::cout << "**(SOFTWARE) DepthBuffer Visualization ON\n";
			SetConsoleTextAttribute(h, 7);

			m_BoundingBoxVisualization = true;
		}
	}

	//HARDWARE
	void Renderer::ToggleFireFx()
	{
		if (m_UseHardware == false) return;

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
	void Renderer::CycleSamplerState()
	{
		if (m_UseHardware == false) return;

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

#pragma region MeshInitialization
	void Renderer::VehicleMeshInit()
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("resources/vehicle.obj", vertices, indices) == false)
			std::cout << ".obj not found\n";

		m_pVehicleDiffuse = Texture::LoadFromFile("resources/vehicle_diffuse.png", m_pDevice);
		m_pVehicleNormalMap = Texture::LoadFromFile("resources/vehicle_normal.png", m_pDevice);
		m_pVehicleSpecularMap = Texture::LoadFromFile("resources/vehicle_specular.png", m_pDevice);
		m_pVehicleGlossinessMap = Texture::LoadFromFile("resources/vehicle_gloss.png", m_pDevice);

		const Vector3 position{ 0,0,50 };
		const Vector3 rotation{ 0,0,0 };
		const Vector3 scale{ 1,1,1 };
		const Matrix worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);

		auto pShadedEffect = new ShadedEffect{ m_pDevice, L"resources/Vehicle.fx" };
		pShadedEffect->SetDiffuseMap(m_pVehicleDiffuse);
		pShadedEffect->SetNormalMap(m_pVehicleNormalMap);
		pShadedEffect->SetSpecularMap(m_pVehicleSpecularMap);
		pShadedEffect->SetGlossinessMap(m_pVehicleGlossinessMap);

		pShadedEffect->SetWorldMatrixVariable(worldMatrix);

		auto pMesh = new Mesh{ m_pDevice, vertices, indices, pShadedEffect };
		pMesh->SetWorldMatrix(worldMatrix);

		std::pair<Mesh*, ShadedEffect*> pair(pMesh, pShadedEffect);

		m_pMeshToShadedEffectMap->insert(pair);
	}
	void Renderer::CombustionMeshInit()
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		if (Utils::ParseOBJ("resources/fireFX.obj", vertices, indices) == false)
			std::cout << ".obj not found\n";

		m_pFireFXDiffuse = Texture::LoadFromFile("resources/fireFX_diffuse.png", m_pDevice);

		auto pTransEffect = new TransEffect{ m_pDevice, L"resources/Fire.fx" };
		pTransEffect->SetDiffuseMap(m_pFireFXDiffuse);

		const Vector3 position{ 0,0,50 };
		const Vector3 rotation{ 0,0,0 };
		const Vector3 scale{ 1,1,1 };
		const Matrix worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);

		auto pMesh = new Mesh{ m_pDevice, vertices, indices, pTransEffect };
		pMesh->SetWorldMatrix(worldMatrix);

		m_pMeshToTransEffectMap->insert(std::make_pair(pMesh, pTransEffect));
	}
#pragma endregion
#pragma region SoftwareHelpers
	void Renderer::VertexTransformationFunction(Mesh& m) const
	{
		m.verticesOut.clear();
		m.verticesOut.reserve(m.vertices.size());

		const Matrix worldViewProjectionMatrix = m.GetWorldMatrix() * m_pCamera->GetViewMatrix() * m_pCamera->GetProjectionMatrix();

		for (const auto& v : m.vertices)
		{
			VertexOut vertexOut{};

			// to NDC-Space
			vertexOut.position = worldViewProjectionMatrix.TransformPoint(v.position.ToVector4());

			vertexOut.viewDirection = Vector3{ vertexOut.position.GetXYZ() };
			vertexOut.viewDirection.Normalize();

			vertexOut.position.x /= vertexOut.position.w;
			vertexOut.position.y /= vertexOut.position.w;
			vertexOut.position.z /= vertexOut.position.w;

			vertexOut.color = v.color;
			vertexOut.normal = m.GetWorldMatrix().TransformVector(v.normal).Normalized();
			vertexOut.uv = v.uv;
			vertexOut.tangent = m.GetWorldMatrix().TransformVector(v.tangent).Normalized();

			m.verticesOut.emplace_back(vertexOut);
		}
	}
	void Renderer::RenderTriangleList(Mesh& mesh) const
	{
		ColorRGB finalColor{ };

		for (size_t i{}; i < mesh.indices.size(); i += 3)
		{
			VertexOut vOut0 = mesh.verticesOut[mesh.indices[i]];
			VertexOut vOut1 = mesh.verticesOut[mesh.indices[i + 1]];
			VertexOut vOut2 = mesh.verticesOut[mesh.indices[i + 2]];

			// frustum culling check
			if (!IsInFrustum(vOut0)
				|| !IsInFrustum(vOut1)
				|| !IsInFrustum(vOut2))
				continue;

			// from NDC space to Raster space
			NDCToRaster(vOut0);
			NDCToRaster(vOut1);
			NDCToRaster(vOut2);

			const Vector2 v0 = { vOut0.position.x, vOut0.position.y };
			const Vector2 v1 = { vOut1.position.x, vOut1.position.y };
			const Vector2 v2 = { vOut2.position.x, vOut2.position.y };

			const Vector2 edge01 = v1 - v0;
			const Vector2 edge12 = v2 - v1;
			const Vector2 edge20 = v0 - v2;

			const float areaTriangle = Vector2::Cross(edge01, edge12);

			// create bounding box for triangle
			const INT top = std::max((INT)std::max(v0.y, v1.y), (INT)v2.y);
			const INT bottom = std::min((INT)std::min(v0.y, v1.y), (INT)v2.y);

			const INT left = std::min((INT)std::min(v0.x, v1.x), (INT)v2.x);
			const INT right = std::max((INT)std::max(v0.x, v1.x), (INT)v2.x);

			// check if bounding box is in screen
			if (left <= 0 || right >= m_Width - 1)
				continue;

			if (bottom <= 0 || top >= m_Height - 1)
				continue;

			constexpr INT offSet{ 1 };

			// iterate over every pixel in the bounding box, with an offset we enlarge the BB
			// in case of overlooked pixels
			for (INT px = left - offSet; px < right + offSet; ++px)
			{
				for (INT py = bottom - offSet; py < top + offSet; ++py)
				{
					if (m_BoundingBoxVisualization == false)
					{
						finalColor = colors::Black;

						Vector2 pixelPos = { (float)px,(float)py };

						const Vector2 directionV0 = pixelPos - v0;
						const Vector2 directionV1 = pixelPos - v1;
						const Vector2 directionV2 = pixelPos - v2;

						// weights are all negative => back-face culling
						// vs all positive => front-face culling
						float weightV2 = Vector2::Cross(edge01, directionV0);
						if (weightV2 < 0 && 
							(m_CurrentCullMode == CullMode::back || m_CurrentCullMode == CullMode::none))
							continue;
						if (weightV2 > 0 && m_CurrentCullMode == CullMode::front)
							continue;

						float weightV0 = Vector2::Cross(edge12, directionV1);
						if (weightV0 < 0 && 
							(m_CurrentCullMode == CullMode::back || m_CurrentCullMode == CullMode::none))

							continue;
						if (weightV0 > 0 && m_CurrentCullMode == CullMode::front)
							continue;

						float weightV1 = Vector2::Cross(edge20, directionV2);
						if (weightV1 < 0 && 
							(m_CurrentCullMode == CullMode::back || m_CurrentCullMode == CullMode::none))
							continue;
						if (weightV1 > 0 && m_CurrentCullMode == CullMode::front)
							continue;

						weightV0 /= areaTriangle;
						weightV1 /= areaTriangle;
						weightV2 /= areaTriangle;

						if (weightV0 + weightV1 + weightV2 < 1 - FLT_EPSILON
							&& weightV0 + weightV1 + weightV2 > 1 + FLT_EPSILON)
							continue;

						// This Z-BufferValue is the one we compare in the Depth Test and
						// the value we store in the Depth Buffer (uses position.z).
						float interpolatedZDepth = {
							1.f /
							((1 / vOut0.position.z) * weightV0 +
							(1 / vOut1.position.z) * weightV1 +
							(1 / vOut2.position.z) * weightV2)
						};

						if (interpolatedZDepth < 0 || interpolatedZDepth > 1)
							continue;

						if (interpolatedZDepth > m_pDepthBufferPixels[px + (py * m_Width)])
							continue;

						m_pDepthBufferPixels[px + (py * m_Width)] = interpolatedZDepth;

						if (m_DepthBufferVisualization == false)
						{
							// When we want to interpolate vertex attributes with a correct depth(color, uv, normals, etc.),
							// we still use the View Space depth(uses position.w)
							const float interpolatedWDepth = {
								1.f /
								((1 / vOut0.position.w) * weightV0 +
								(1 / vOut1.position.w) * weightV1 +
								(1 / vOut2.position.w) * weightV2)
							};

							const Vector2 interpolatedUV = {
								((vOut0.uv / vOut0.position.w) * weightV0 +
								(vOut1.uv / vOut1.position.w) * weightV1 +
								(vOut2.uv / vOut2.position.w) * weightV2) * interpolatedWDepth
							};

							const Vector3 interpolatedNormal = {
								((vOut0.normal / vOut0.position.w) * weightV0 +
								(vOut1.normal / vOut1.position.w) * weightV1 +
								(vOut2.normal / vOut2.position.w) * weightV2) * interpolatedWDepth
							};

							const Vector3 interpolatedTangent = {
								((vOut0.tangent / vOut0.position.w) * weightV0 +
								(vOut1.tangent / vOut1.position.w) * weightV1 +
								(vOut2.tangent / vOut2.position.w) * weightV2) * interpolatedWDepth
							};

							const Vector3 interpolatedViewDirection = {
								((vOut0.viewDirection / vOut0.position.w) * weightV0 +
								(vOut1.viewDirection / vOut1.position.w) * weightV1 +
								(vOut2.viewDirection / vOut2.position.w) * weightV2) * interpolatedWDepth
							};

							//Interpolated Vertex Attributes for Pixel
							VertexOut pixel;
							pixel.position = { pixelPos.x, pixelPos.y, interpolatedZDepth, interpolatedWDepth };
							pixel.color = finalColor;
							pixel.uv = interpolatedUV;
							pixel.normal = interpolatedNormal;
							pixel.tangent = interpolatedTangent;
							pixel.viewDirection = interpolatedViewDirection;

							PixelShading(pixel);

							finalColor = pixel.color;
						}
						else
						{
							const float depthBufferColor = Remap(m_pDepthBufferPixels[px + (py * m_Width)], 0.995f, 1.0f);

							finalColor = { depthBufferColor, depthBufferColor, depthBufferColor };
						}
					}
					else
					{
						finalColor = colors::White;
					}

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}
	void Renderer::PixelShading(VertexOut& v) const
	{
		ColorRGB tempColor{ colors::Black };

		const Vector3 directionToLight = -Vector3{ .577f,-.577f,.577f };
		constexpr float lightIntensity = 7.f;
		constexpr float specularShininess = 25.f;

		Vector3 normal;

		if (m_EnableNormalMap)
		{
			// Normal map
			const Vector3 biNormal = Vector3::Cross(v.normal, v.tangent);
			const Matrix tangentSpaceAxis = { v.tangent, biNormal, v.normal, Vector3::Zero };

			const ColorRGB normalColor = m_pVehicleNormalMap->Sample(v.uv);
			Vector3 sampledNormal = { normalColor.r, normalColor.g, normalColor.b }; // => range [0, 1]
			sampledNormal = 2.f * sampledNormal - Vector3{ 1, 1, 1 }; // => [0, 1] to [-1, 1]

			sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal);

			normal = sampledNormal.Normalized();
			////////////////////
		}
		else
			normal = v.normal;

		// Observed Area
		const float lambertCos = Vector3::Dot(normal, directionToLight);
		if (lambertCos < 0)
			return;

		const ColorRGB observedArea = { lambertCos, lambertCos, lambertCos };
		////////////////////

		ColorRGB diffuse;

		// phong specular
		const ColorRGB gloss = m_pVehicleGlossinessMap->Sample(v.uv);
		const float exponent = gloss.r * specularShininess;

		ColorRGB specular;
		////////////////////////

		switch (m_CurrentShadingMode)
		{
		case ShadingMode::observedArea:
			tempColor += observedArea;
			break;
		case ShadingMode::diffuse:
			diffuse = BRDF::Lambert(m_pVehicleDiffuse->Sample(v.uv));

			tempColor += diffuse * observedArea * lightIntensity;
			break;
		case ShadingMode::specular:
			specular = BRDF::Phong(m_pVehicleSpecularMap->Sample(v.uv), exponent, directionToLight, v.viewDirection, normal);

			tempColor += specular * observedArea;
			break;
		case ShadingMode::combined:
			specular = BRDF::Phong(m_pVehicleSpecularMap->Sample(v.uv), exponent, directionToLight, v.viewDirection, normal);
			diffuse = BRDF::Lambert(m_pVehicleDiffuse->Sample(v.uv));

			tempColor += diffuse * observedArea * lightIntensity + specular;
			break;
		}

		constexpr ColorRGB ambient = { .025f,.025f,.025f };

		tempColor += ambient;

		v.color = tempColor;

	}
	bool Renderer::IsInFrustum(const VertexOut& v) const
	{
		if (v.position.x < -1 || v.position.x > 1)
			return false;

		if (v.position.y < -1 || v.position.y > 1)
			return  false;

		if (v.position.z < 0 || v.position.z > 1)
			return  false;

		return true;
	}
	void Renderer::NDCToRaster(VertexOut& v) const
	{
		v.position.x = (v.position.x + 1) * 0.5f * (float)m_Width;
		v.position.y = (1 - v.position.y) * 0.5f * (float)m_Height;
	}
	void Renderer::ClearBackground() const
	{
		SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, (Uint8)(m_BackGroundColor.r * 255.f), (Uint8)(m_BackGroundColor.g * 255.f), (Uint8)(m_BackGroundColor.b * 255.f)));
	}
#pragma endregion
#pragma region HardwareHelpers
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
#pragma endregion
}