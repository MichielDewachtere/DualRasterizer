#include "pch.h"
#include "Mesh.h"
#include "ShadedEffect.h"

Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& _vertices, const std::vector<uint32_t>& _indices, Effect* pEffect)
	: vertices(_vertices)
	, indices(_indices)
	, m_pEffect{ pEffect }
{
	// Create Vertex Layout
	static constexpr uint32_t numElements{ 5 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "COLOR";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "TEXCOORD";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 24;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "NORMAL";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;


	vertexDesc[4].SemanticName = "TANGENT";
	vertexDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[4].AlignedByteOffset = 44;
	vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pEffect->GetDefaultTechnique()->GetPassByIndex(0)->GetDesc(&passDesc);
	HRESULT result{ pDevice->CreateInputLayout
		(
			vertexDesc,
			numElements,
			passDesc.pIAInputSignature,
			passDesc.IAInputSignatureSize,
			&m_pVertexLayout
		) };
	if (FAILED(result)) return;

	// Create vertex buffer
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices.data();

	result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result)) return;

	// Create index buffer
	m_AmountIndices = static_cast<uint32_t>(indices.size());
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(uint32_t) * m_AmountIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = indices.data();

	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
	if (FAILED(result)) return;
}

Mesh::~Mesh()
{
	if (m_pIndexBuffer) m_pIndexBuffer->Release();
	if (m_pVertexBuffer) m_pVertexBuffer->Release();

	if (m_pVertexLayout) m_pVertexLayout->Release();
}

void Mesh::Render(ID3D11DeviceContext* pDeviceContext) const
{
	// Set primitive topology
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set input layout
	pDeviceContext->IASetInputLayout(m_pVertexLayout);

	// Set vertex buffer
	constexpr UINT stride{ sizeof(Vertex) };
	constexpr UINT offset{};
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set index buffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	DrawUsingDefaultTechnique(pDeviceContext);
}

void Mesh::SetWorldViewProjectionMatrix(const Matrix& viewMatrix, const Matrix& projectionMatrix) const
{
	const Matrix worldViewProjection = m_WorldMatrix * viewMatrix * projectionMatrix;
	m_pEffect->SetWorldViewProjectionMatrixVariable(worldViewProjection);
}

void Mesh::DrawUsingDefaultTechnique(ID3D11DeviceContext* pDeviceContext) const
{
	D3DX11_TECHNIQUE_DESC techniqueDesc{};
	m_pEffect->GetDefaultTechnique()->GetDesc(&techniqueDesc);
	for (UINT p{}; p < techniqueDesc.Passes; ++p)
	{
		m_pEffect->GetDefaultTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
		pDeviceContext->DrawIndexed(m_AmountIndices, 0, 0);
	}

}
