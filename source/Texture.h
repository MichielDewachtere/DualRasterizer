#pragma once
#include <SDL_surface.h>
#include <string>

namespace dae
{
	struct Vector2;

	class Texture final
	{
	public:
		~Texture();
		Texture(const Texture& other) = default;
		Texture& operator=(const Texture& rhs) = default;
		Texture(Texture&& other) = default;
		Texture& operator=(Texture&& rhs) = default;

		static Texture* LoadFromFile(const std::string& path, ID3D11Device* pDevice);
		ColorRGB Sample(const Vector2& uv) const;

		ID3D11ShaderResourceView* GetShaderResourceView() const { return m_pSRV; }

	private:
		explicit Texture(SDL_Surface* pSurface, ID3D11Device* pDevice);

		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };

		ID3D11Texture2D* m_pResource{};
		ID3D11ShaderResourceView* m_pSRV{};
	};
}