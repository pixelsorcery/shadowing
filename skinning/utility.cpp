// AMDG

#include "utility.h"
#include <memory>
#include <cassert>

using namespace std;

HRESULT loadTexture(ID3D11Device* pDevice, 
    ID3D11DeviceContext* pContext,
    ID3D11Texture2D** ppTexture,
    const std::string& path,
    UINT bindFlags,
    UINT cpuAccessFlags)
{
    HRESULT hr = S_OK;

    int width = 0;
    int height = 0;
    int comp = 0;
    unsigned char *pData;
    pData = static_cast<unsigned char *>(stbi_load(path.c_str(), &width, &height, &comp, 0));

    // need to convert to 4 component data
    unique_ptr<unsigned char[]> p4cData;

    if (comp == 3)
    {
        // need to convert to 4 component data
        p4cData = make_unique<unsigned char[]>(width * height * 4);

        unsigned char* ptr = p4cData.get();
        unsigned char* dataPtr = pData;
        for (uint32_t j = 0; j < (uint32_t)width * (uint32_t)height; j++)
        {
            *ptr = *dataPtr;
            ptr++, dataPtr++;
            *ptr = *dataPtr;
            ptr++, dataPtr++;
            *ptr = *dataPtr;
            ptr++, dataPtr++;
            *ptr = 0;
            ptr++;
        }

        pData = p4cData.get();
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    // Create texture resource
    hr = pDevice->CreateTexture2D(&desc, nullptr, ppTexture);
    assert(S_OK == hr);

    // Upload data
    pContext->UpdateSubresource(*ppTexture, 0, nullptr, pData, width * 4, 0);

    return hr;
}

void debugPrint(char* str)
{
    wchar_t wc[1024];
    const size_t cSize = strlen(str) + 1;
    assert(cSize < 1024);
    size_t outSize;
    mbstowcs_s(&outSize, wc, cSize, str, cSize - 1);
    OutputDebugStringW(wc);
}