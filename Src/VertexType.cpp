#include "VertexType.h"

const D3D12_INPUT_ELEMENT_DESC VertexPositionNormalTangentTexture::InputElements[] =
{
    { "SV_Position",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",         0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",       0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static_assert(sizeof(VertexPositionNormalTangentTexture) == 44, "Vertex struct/layout mismatch");

const D3D12_INPUT_LAYOUT_DESC VertexPositionNormalTangentTexture::InputLayout =
{
    VertexPositionNormalTangentTexture::InputElements,
    VertexPositionNormalTangentTexture::InputElementCount
};