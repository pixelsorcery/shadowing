#pragma once

#include <vector>
#include <unordered_map>
#include <DirectXMath.h>

static constexpr uint32_t MAX_BONES = 4;

struct BoneInfo
{
    DirectX::XMMATRIX boneOffset;
    DirectX::XMMATRIX finalTransformation;
};

struct BoneData
{
    uint32_t ids[MAX_BONES];
    float weights[MAX_BONES];
    unsigned char idx;

    BoneData() :idx(0), ids{ 0,0,0,0 }, weights{ 0.0f,0.0f,0.0f,0.0f } {};

    void insertData(uint32_t id, float weight)
    {
        //assert(idx < MAX_BONES);
        if (idx >= MAX_BONES) return;
        ids[idx] = id;
        weights[idx] = weight;
        idx++;
    };
};

struct MeshEntry
{
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texCoords;
    std::vector<BoneData> boneData;
    std::vector<uint16_t> indices;
    uint32_t materialIdx;
    uint32_t vBufIdx;
    uint32_t iBufIdx;
};

class SkinnedMesh
{
public:
    SkinnedMesh() :m_numMeshes(0), m_numBones(0) {};
    uint32_t m_numMeshes;
    std::vector<MeshEntry> m_entries;
    std::unordered_map<std::string, uint32_t> m_boneMap;
    uint32_t m_numBones;
    std::vector<BoneInfo> m_boneInfo;
};