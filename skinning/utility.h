// AMDG
#pragma once

#include "stb_image.h"
#include <string>
#include <d3d11.h>

HRESULT loadTexture(ID3D11Device* pDevice,
    ID3D11DeviceContext* pContext,
    ID3D11Texture2D** ppTexture,
    const std::string& path,
    UINT bindFlags,
    UINT cpuAccessFlags);

void debugPrint(char* str);