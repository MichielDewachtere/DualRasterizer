#include "pch.h"
#include "Texture.h"
#include <SDL_image.h>

using namespace dae;

Texture::Texture(SDL_Surface* pSurface, ID3D11Device* pDevice)
	: m_pSurface{ pSurface }
	, m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
{
	constexpr DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = pSurface->w;
	desc.Height = pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

	HRESULT result = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	if (FAILED(result))
	{
		std::cout << "Failed to initialize texture\n";
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D10_1_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	result = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);

	if (FAILED(result))
	{
		std::cout << "Failed to initialize SRV\n";
		return;
	}
}


Texture::~Texture()
{
	if (m_pSurface)
	{
		SDL_FreeSurface(m_pSurface);
		m_pSurface = nullptr;
	}

	if (m_pSRV) m_pSRV->Release();
	if (m_pResource) m_pResource->Release();
}

Texture* Texture::LoadFromFile(const std::string& path, ID3D11Device* pDevice)
{
	//Load SDL_Surface using IMG_LOAD
	//Create & Return a new Texture Object (using SDL_Surface)

	return new Texture{ IMG_Load(path.c_str()), pDevice };
}

ColorRGB Texture::Sample(const Vector2& uv) const
{
	Uint8 r{ 0 }, g{ 0 }, b{ 0 };

	const int x = (int)(uv.x * m_pSurface->w);
	const int y = (int)(uv.y * m_pSurface->h);

	const int idx{ x + y * m_pSurface->w };

	SDL_GetRGB(m_pSurfacePixels[idx], m_pSurface->format, &r, &g, &b);

	return ColorRGB{ r / 255.f, g / 255.f, b / 255.f };
}
