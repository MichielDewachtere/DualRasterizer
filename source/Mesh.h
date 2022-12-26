#pragma once

#include "Vector3.h"
#include "ColorRGB.h"
#include <vector>

using namespace dae;
class Effect;

struct Vertex
{
	Vector3 position;
	ColorRGB color = colors::White;
	Vector2 uv;
	Vector3 normal;
	Vector3 tangent;
};

class Mesh final
{
public:
	Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Effect* pEffect);
	~Mesh();

	Mesh(const Mesh& other) = delete;
	Mesh& operator=(const Mesh& rhs) = delete;
	Mesh(Mesh&& other) = delete;
	Mesh& operator=(Mesh&& rhs) = delete;

	void Render(ID3D11DeviceContext* pDeviceContext) const;

	void SetWorldViewProjectionMatrix(const Matrix& viewMatrix, const Matrix& projectionMatrix) const;

	void SetWorldMatrix(const Matrix& newMatrix) { m_WorldMatrix = newMatrix; }
	Matrix GetWorldMatrix() const { return m_WorldMatrix; }

private:
	ID3D11Buffer* m_pVertexBuffer{};
	Effect* m_pEffect;
	ID3D11InputLayout* m_pVertexLayout{};
	uint32_t m_AmountIndices;
	ID3D11Buffer* m_pIndexBuffer{};

	Matrix m_WorldMatrix{};
	std::vector<Vertex> m_Vertices;

	void DrawUsingDefaultTechnique(ID3D11DeviceContext* pDeviceContext) const;
};