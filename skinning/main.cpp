// AMDG
// DX setup and windowing code is ripped from directx 11 tutorials in the sdk.
// All skinning code is (c) John Lukasiewicz 2017
// Beerware license, you have to buy me a beer if you use this code and
// run into me at GDC or a Khronos meetup. 

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include "mesh.h"
#include "utility.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN

#define WIDTH 1024
#define HEIGHT 768

using namespace std;
using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct BlendVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
    XMFLOAT2 Tex;
    XMFLOAT4 BoneWeights;
    XMUINT4  BoneIndices;
};

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
    XMFLOAT2 Tex;
};

struct CBVSUniforms
{
    XMMATRIX mView;
    XMMATRIX mLightView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
    XMMATRIX mLightProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
};

struct CBPSUniforms
{
    XMFLOAT4 dirLightPos;
    XMFLOAT4 eyePos;
};

struct Camera
{
    float dx, dy;
    float speed;
    XMVECTOR position;
    XMVECTOR direction;
    XMVECTOR right;
    XMVECTOR up;
};

SimpleVertex ground[4]{ { { -500.0f, 0.0f, -500.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                        { { -500.0f, 0.0f, 500.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
                        { { 500.0f, 0.0f, 500.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                        { { 500.0f, 0.0f, -500.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } };

uint16_t groundIndices[6] = { 0, 1, 2, 2, 3, 0 };

char keys[256];

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                 g_hInst = nullptr;
HWND                      g_hWnd = nullptr; 
D3D_DRIVER_TYPE           g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL         g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*             g_pd3dDevice = nullptr;
ID3D11DeviceContext*      g_pImmediateContext = nullptr;
IDXGISwapChain*           g_pSwapChain = nullptr;
ID3D11RenderTargetView*   g_pRenderTargetView = nullptr;
ID3D11Texture2D*          g_pDepthStencil = nullptr;
ID3D11DepthStencilView*   g_pDepthStencilView = nullptr;
ID3D11Texture2D*          g_pShadowBuffer = nullptr;
ID3D11DepthStencilView*   g_pShadowBufferView = nullptr;
ID3D11ShaderResourceView* g_pShadowBufferSRV = nullptr;
ID3D11Texture2D*          g_pShadowMap = nullptr;
ID3D11DepthStencilView*   g_pShadowMapView = nullptr;
ID3D11VertexShader*       g_pBlendVertexShader = nullptr;
ID3D11VertexShader*       g_pSimpleVertexShader = nullptr;
ID3D11PixelShader*        g_pPixelShader = nullptr;
ID3D11PixelShader*        g_pDepthShader = nullptr;
ID3D11InputLayout*        g_pBlendVertexLayout = nullptr;
ID3D11InputLayout*        g_pSimpleVertexLayout = nullptr;
ID3D11Buffer*             g_pGroundVB = nullptr;
ID3D11Buffer*             g_pGroundIB = nullptr;
ID3D11Texture2D*          g_pGroundTexture = nullptr;
ID3D11ShaderResourceView* g_pGroundTextureSRV = nullptr;
ID3D11Buffer*             g_pCBVsUniforms = nullptr;
ID3D11Buffer*             g_pCBPsUniforms = nullptr;
ID3D11Buffer*             g_pCBChangeOnResize = nullptr;
ID3D11Buffer*             g_pCBChangesEveryFrame = nullptr;
ID3D11ShaderResourceView* g_pTextureRV = nullptr;
ID3D11SamplerState*       g_pSamplerLinear = nullptr;
ID3D11SamplerState*       g_pSamplerClamp = nullptr;
ID3D11RasterizerState*    g_pRasterState = nullptr;
ID3D11DepthStencilState*  g_pDSShadowMap = nullptr;
ID3D11DepthStencilState*  g_pDSState = nullptr;
ID3D11ShaderResourceView* g_nullSRV[1] = { nullptr };
DirectX::XMMATRIX         g_World;
DirectX::XMMATRIX         g_View;
DirectX::XMMATRIX         g_LightView;
DirectX::XMMATRIX         g_Projection;
DirectX::XMMATRIX         g_LightProjection;
DirectX::XMFLOAT4         g_vMeshColor(1.0f, 1.0f, 1.0f, 1.0f);
DirectX::XMFLOAT4         g_vLightPos(50.0f, 200.0f, -120.0f, 1.0f);
DirectX::XMFLOAT4         g_vEyePos(0.0f, 75.0f, -260.0f, 0.0f);
CBPSUniforms              g_cbPsUniforms{ g_vLightPos, g_vEyePos };
Camera                    g_camera = {};
DWORD                     g_timeStart = 0;
DWORD                     g_timeCurrent = 0;
float                     g_t = 0.0f;
XMFLOAT3                  sPosition(0.0f, 100.0f, -160.0f);
XMFLOAT3                  sDir(0.0f, 0.0f, 1.0f);
// Arrays of buffers and textures
vector<ID3D11Buffer*>             g_vBufferMgr;
vector<ID3D11Buffer*>             g_iBufferMgr;
vector<ID3D11Texture2D*>          g_textureMgr;
vector<ID3D11ShaderResourceView*> g_srvMgr;
ID3D11Buffer*                     g_pBoneBuffer;
ID3D11ShaderResourceView*         g_pBoneSRV = nullptr;

// Other resource stuff
uint32_t                  g_groundTextureIdx;

// Mesh data
unique_ptr<SkinnedMesh> g_skinnedMesh = nullptr;
Assimp::Importer importer; // note importer owns g_pScene
const aiScene* g_pScene = nullptr;

//--------------------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
HRESULT InitMesh();
XMMATRIX aiToXMMATRIX(aiMatrix4x4 in);
void onMouseMove(int x, int y, int deltaX, int deltaY);

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        if (pErrorBlob) pErrorBlob->Release();
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = "SkinningTest";
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, WIDTH, HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow("SkinningTest", "Skinning Test", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!g_hWnd)
        return E_FAIL;

    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

HRESULT CreateBuffer(ID3D11Buffer**  ppDest, 
                     D3D11_BIND_FLAG bindFlag, 
                     void*           pData, 
                     size_t          stride, 
                     uint32_t        bufferSize,
                     D3D11_USAGE     usage)
{
    HRESULT hr = S_OK;

    uint32_t cpuAccessFlags = 0;

    if (usage == D3D11_USAGE_DYNAMIC)
    {
        cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }

    D3D11_BUFFER_DESC bd = {};
    ID3D11Buffer* pDest;
    bd.Usage = usage;
    bd.ByteWidth = static_cast<uint32_t>(stride) * bufferSize;
    bd.BindFlags = bindFlag;
    bd.CPUAccessFlags = cpuAccessFlags;

    if (usage == D3D11_USAGE_DYNAMIC)
    {
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(DirectX::XMMATRIX);
        // note: dynamic buffer implies we will discard-map this buffer updating it later
        hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &pDest);
    }
    else
    {
        // only default usage path is tested
        assert(usage == D3D11_USAGE_DEFAULT);
        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem = pData;

        hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &pDest);
    }

    assert(hr == S_OK);

    *ppDest = pDest;

    return hr;
}

HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 4;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
    descDepth.SampleDesc.Count = 4;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
    if (FAILED(hr))
        return hr;

    // Shadow map 
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.SampleDesc.Count = 1;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pShadowBuffer);
    if (FAILED(hr))
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
    if (FAILED(hr))
        return hr;

    // Shadow map view 
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pShadowBuffer, &descDSV, &g_pShadowBufferView);
    if (FAILED(hr))
        return hr;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = true;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.StencilEnable = false;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Create depth stencil state
    g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDSState);

    // Create shadow map state
    dsDesc = {};
    dsDesc.DepthEnable = true;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.StencilEnable = false;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDSShadowMap);

    D3D11_SHADER_RESOURCE_VIEW_DESC shadowSRVDesc = {};
    shadowSRVDesc.Texture2D.MipLevels = 1;
    shadowSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shadowSRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pShadowBuffer, &shadowSRVDesc, &g_pShadowBufferSRV);
    if (FAILED(hr))
        return hr;

    //g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);


    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"skinning_vs.hlsl", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
        return hr;
    }

    // Create the vertex shaders
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pBlendVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC blendLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(blendLayout);

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout(blendLayout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &g_pBlendVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout(g_pBlendVertexLayout);

    hr = CompileShaderFromFile(L"simple_vs.hlsl", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
        return hr;
    }

    // Create the vertex shaders
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pSimpleVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC simpleLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    numElements = ARRAYSIZE(simpleLayout);

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout(simpleLayout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &g_pSimpleVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"lighting_ps.hlsl", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the depth output shader
    hr = CompileShaderFromFile(L"depth_ps.hlsl", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pDepthShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Create ground
    hr = CreateBuffer(&g_pGroundVB, D3D11_BIND_VERTEX_BUFFER, ground, sizeof(SimpleVertex), 6, D3D11_USAGE_DEFAULT);
    assert(S_OK == hr);
    hr = CreateBuffer(&g_pGroundIB, D3D11_BIND_INDEX_BUFFER, groundIndices, sizeof(uint16_t), 6, D3D11_USAGE_DEFAULT);
    assert(S_OK == hr);

    string path = "ground.png";
    hr = loadTexture(g_pd3dDevice, g_pImmediateContext, &g_pGroundTexture, path, D3D11_BIND_SHADER_RESOURCE, 0);
    assert(S_OK == hr);
    hr = g_pd3dDevice->CreateShaderResourceView(g_pGroundTexture, nullptr, &g_pGroundTextureSRV);
    assert(S_OK == hr);

    // Create the constant buffers
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBVSUniforms);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBVsUniforms);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBPSUniforms);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBPsUniforms);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangeOnResize);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame);
    if (FAILED(hr))
        return hr;

    // Create crazy hot pink border color in case uv gets messed up
    float borderC[] = { 1.0f, 0.0f, 1.0f, 1.0f };
    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.MaxAnisotropy = 16;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    memcpy_s(&sampDesc.BorderColor, sizeof(float) * 4, borderC, sizeof(float) * 4);
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
    assert(S_OK == hr);

    D3D11_SAMPLER_DESC pointSampDesc = {};
    pointSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    pointSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    pointSampDesc.MipLODBias = 0.0f;
    pointSampDesc.MaxAnisotropy = 1;
    pointSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState(&pointSampDesc, &g_pSamplerClamp);

    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FrontCounterClockwise = true;
    rastDesc.AntialiasedLineEnable = false;
    rastDesc.DepthBias = 0;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    
    hr = g_pd3dDevice->CreateRasterizerState(&rastDesc, &g_pRasterState);
    assert(S_OK == hr);

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(g_vEyePos.x, g_vEyePos.y, g_vEyePos.z, g_vEyePos.w);
    XMVECTOR At = XMVectorSet(0.0f, 75.0f, 0.0f, 0.0f);
    //XMVECTOR Eye = XMVectorSet(0.0f, -200.0f, -160.0f, 0.0f);
    //XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    //XMVECTOR Eye = XMVectorSet(0.0f, 15.0f, 40.0f, 0.0f);
    //XMVECTOR At = XMVectorSet(0.0f, 6.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    //g_View = XMMatrixLookAtLH(Eye, At, Up);

	////g_View = XMMatrixTranslation(g_camera.position.x, g_camera.position.y, g_camera.position.z);

    g_camera.position = XMVECTOR(XMLoadFloat3(&sPosition));
    g_camera.direction = XMVECTOR(XMLoadFloat3(&sDir));
    //g_camera.dx = 3.1415f;
    onMouseMove(0, 0, 0, 0); // just to init

    // Light view transformation matrix for shadow map
    XMVECTOR lightPos = XMVectorSet(g_vLightPos.x, g_vLightPos.y, g_vLightPos.z, g_vLightPos.w);
    g_LightView = XMMatrixLookAtRH(lightPos, At, Up);

    // Initialize the projection matrix
    g_Projection = XMMatrixPerspectiveFovRH(XM_PIDIV4, width / (FLOAT)height, 1.0f, 500.0f);
    g_LightProjection = XMMatrixPerspectiveFovRH(XM_PIDIV4, width / (FLOAT)height, 1.0f, 500.0f);

    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);
    cbChangesOnResize.mLightProjection = XMMatrixTranspose(g_LightProjection);

    g_pImmediateContext->UpdateSubresource(g_pCBChangeOnResize, 0, nullptr, &cbChangesOnResize, 0, 0);

    // Update light position
    g_pImmediateContext->UpdateSubresource(g_pCBPsUniforms, 0, nullptr, &g_cbPsUniforms, 0, 0);

    hr = InitMesh();
    assert(S_OK == hr);

    return hr;
}

HRESULT InitMesh()
{
    HRESULT hr = S_OK;

    std::string filename = "..\\3d_models\\vanilla\\source\\happy1.fbx";
    std::string materialPath = "..\\3d_models\\vanilla\\source";
    //std::string filename = "..\\3d_models\\wobble_cube\\wobble3.fbx";
    //std::string materialPath = "..\\3d_models\\wobble_cube\\";

    //importer.SetPropertyBool("AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS", false);
    //assert(importer.GetPropertyBool("AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS") == false);
    g_pScene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Fast |
                                           aiProcess_ConvertToLeftHanded);

    g_skinnedMesh = make_unique<SkinnedMesh>(SkinnedMesh{});
    assert(g_pScene->mNumMeshes > 0);

    g_skinnedMesh->m_numMeshes = g_pScene->mNumMeshes;

    g_skinnedMesh->m_entries.resize(g_pScene->mNumMeshes);
    g_vBufferMgr.resize(g_pScene->mNumMeshes);
    g_iBufferMgr.resize(g_pScene->mNumMeshes);

    for (uint32_t i = 0; i < g_skinnedMesh->m_numMeshes; i++)
    {
        const aiMesh* pAiMesh = g_pScene->mMeshes[i];
        MeshEntry* pEntry = &g_skinnedMesh->m_entries[i];

        // Populate vertices and tex coordinates
        for (uint32_t j = 0; j < pAiMesh->mNumVertices; j++)
        {
            const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
            const aiVector3D* vtex = &(pAiMesh->mVertices[j]);
            const aiVector3D* norm = &(pAiMesh->mNormals[j]);
            const aiVector3D* texc = pAiMesh->HasTextureCoords(0) ? &(pAiMesh->mTextureCoords[0][j]) : &Zero3D;

            pEntry->positions.push_back(DirectX::XMFLOAT3(vtex->x, vtex->y, vtex->z));
            pEntry->normals.push_back(DirectX::XMFLOAT3(norm->x, norm->y, norm->z));
            pEntry->texCoords.push_back(DirectX::XMFLOAT2(texc->x, texc->y));
        }

        // Populate the index buffer based on faces
        for (uint32_t k = 0; k < pAiMesh->mNumFaces; k++)
        {
            const aiFace& face = pAiMesh->mFaces[k];
            assert(face.mNumIndices == 3);
            pEntry->indices.push_back(face.mIndices[0]);
            pEntry->indices.push_back(face.mIndices[1]);
            pEntry->indices.push_back(face.mIndices[2]);
        }

        uint32_t numIndices = static_cast<uint32_t>(pEntry->indices.size());
        unique_ptr<uint16_t[]> iBufferMem = make_unique<uint16_t[]>(sizeof(uint16_t) * numIndices); // 3 position and 2 tex
        uint16_t* iDataPtr = iBufferMem.get();
        pEntry->iBufIdx = i;

        for (uint32_t k = 0; k < numIndices; k++)
        {
            // make sure it's all 16 bit values
            assert((pEntry->indices[k] & 0xFFFF0000) == 0);
            iDataPtr[k] = static_cast<uint16_t>(pEntry->indices[k]);
        }

        // Create index buffer in video mem
        hr = CreateBuffer(&g_iBufferMgr[i], D3D11_BIND_INDEX_BUFFER, iBufferMem.get(), sizeof(uint16_t), numIndices, D3D11_USAGE_DEFAULT);
        assert(S_OK == hr);

        // Read in bone data per vertex
        pEntry->boneData.resize(pAiMesh->mNumVertices);

        for (uint32_t j = 0; j < pAiMesh->mNumBones; j++)
        {
            aiBone* pBone = pAiMesh->mBones[j];
            string boneName(pBone->mName.data);
            uint32_t boneIndex = 0;

            if (g_skinnedMesh->m_boneMap.find(boneName) == g_skinnedMesh->m_boneMap.end())
            {
                boneIndex = g_skinnedMesh->m_numBones;
                g_skinnedMesh->m_boneMap[boneName] = g_skinnedMesh->m_numBones;
                g_skinnedMesh->m_numBones++;

                BoneInfo bi = {};
                bi.boneOffset = aiToXMMATRIX(pBone->mOffsetMatrix);
                g_skinnedMesh->m_boneInfo.push_back(bi);
            }
            else
            {
                boneIndex = g_skinnedMesh->m_boneMap[boneName];
            }

            for (uint32_t k = 0; k < pBone->mNumWeights; k++)
            {
                uint32_t vid = pBone->mWeights[k].mVertexId;
                float weight = pBone->mWeights[k].mWeight;
                // TODO: record all vertices and normalize into 4
                if (weight < 0.0001)
                {
                    continue;
                }
                pEntry->boneData[vid].insertData(boneIndex, weight);
            }
        }

        // Interleave vertex data into array and create vbuffer in vid mem
        unique_ptr<BlendVertex[]> vBufferMem = make_unique<BlendVertex[]>(sizeof(BlendVertex) * pAiMesh->mNumVertices); // 3 position and 2 tex
        BlendVertex* dataPtr = vBufferMem.get();

        for (uint32_t j = 0; j < pAiMesh->mNumVertices; j++)
        {
            dataPtr[j].Pos = pEntry->positions[j];
            dataPtr[j].Normal = pEntry->normals[j];
            dataPtr[j].Tex = pEntry->texCoords[j];
            memcpy_s(&dataPtr[j].BoneIndices, sizeof(uint32_t) * 4, &pEntry->boneData[j].ids, sizeof(uint32_t) * 4);
            memcpy_s(&dataPtr[j].BoneWeights, sizeof(float) * 4, &pEntry->boneData[j].weights, sizeof(float) * 4);
        }

        // Create buffer in vid mem
        hr = CreateBuffer(&g_vBufferMgr[i], D3D11_BIND_VERTEX_BUFFER, vBufferMem.get(), sizeof(BlendVertex), pAiMesh->mNumVertices, D3D11_USAGE_DEFAULT);
        assert(S_OK == hr);

        // Set vertex buffer index for this mesh
        pEntry->vBufIdx = i;
        
        // Set material index
        pEntry->materialIdx = pAiMesh->mMaterialIndex;
    } // End looping through all meshes

    // Init bone constant buffer
    hr = CreateBuffer(&g_pBoneBuffer, D3D11_BIND_SHADER_RESOURCE, nullptr, sizeof(XMMATRIX), g_skinnedMesh->m_numBones, D3D11_USAGE_DYNAMIC);
    assert(S_OK == hr);

    D3D11_SHADER_RESOURCE_VIEW_DESC bufDesc = {};
    bufDesc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Buffer.ElementWidth = sizeof(DirectX::XMMATRIX);
    bufDesc.Buffer.NumElements = g_skinnedMesh->m_numBones;
    hr = g_pd3dDevice->CreateShaderResourceView(g_pBoneBuffer, &bufDesc, &g_pBoneSRV);
    assert(S_OK == hr);

    // Init materials and textures
    for (uint32_t i = 0; i < g_pScene->mNumMaterials; i++)
    {
        g_textureMgr.resize(g_pScene->mNumMaterials);
        g_srvMgr.resize(g_pScene->mNumMaterials);

        const aiMaterial* pMaterial = g_pScene->mMaterials[i];
        const uint32_t textureCount = pMaterial->GetTextureCount(aiTextureType_DIFFUSE);
        if (textureCount > 0)
        {
            assert(textureCount == 1); // assume 1 material = 1 texture
            aiString Path;
            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
            {
                string p(Path.data);

                int width  = 0;
                int height = 0;
                int comp   = 0;
                unsigned char *pData;

                if (p[0] == '*')
                {
                    int textureIdx = atoi(&p[1]);
                    pData = static_cast<unsigned char *>(
                                          stbi_load_from_memory((stbi_uc*)(g_pScene->mTextures[textureIdx]->pcData),
                                                                           g_pScene->mTextures[textureIdx]->mWidth,
                                                                           &width, 
                                                                           &height, 
                                                                           &comp, 
                                                                           0));
                }
                else
                {
                    //assert(!"untested path, might blow up further down");
                    string fullPath = materialPath + p;
                    pData = static_cast<unsigned char *>(stbi_load(fullPath.c_str(), &width, &height, &comp, 0));
                }

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
                desc.MipLevels = static_cast<UINT>(log2(width));
                desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;

                // Create texture resource
                g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_textureMgr[i]);

                // Upload data
                g_pImmediateContext->UpdateSubresource(g_textureMgr[i], 0, nullptr, pData, width * 4, 0);

                // Create SRV
                g_pd3dDevice->CreateShaderResourceView(g_textureMgr[i], nullptr, &g_srvMgr[i]);

                g_pImmediateContext->GenerateMips(g_srvMgr[i]);
            }
        }
    }
    return hr;
}

DirectX::XMMATRIX aiToXMMATRIX(aiMatrix4x4 in)
{
    // aiMatrix is row major and so is directx
    return DirectX::XMMATRIX(in.a1, in.a2, in.a3, in.a4,
                             in.b1, in.b2, in.b3, in.b4,
                             in.c1, in.c2, in.c3, in.c4,
                             in.d1, in.d2, in.d3, in.d4);
}

void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if (g_pBoneSRV) g_pBoneSRV->Release();
    if (g_pBoneBuffer) g_pBoneBuffer->Release();
    if (g_pRasterState) g_pRasterState->Release();
    if (g_pSamplerLinear) g_pSamplerLinear->Release();
    if (g_pSamplerClamp) g_pSamplerClamp->Release();
    if (g_pTextureRV) g_pTextureRV->Release();
    if (g_pCBVsUniforms) g_pCBVsUniforms->Release();
    if (g_pCBPsUniforms) g_pCBPsUniforms->Release();
    if (g_pCBChangeOnResize) g_pCBChangeOnResize->Release();
    if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
    if (g_pGroundVB) g_pGroundVB->Release();
    if (g_pGroundIB) g_pGroundIB->Release();
    if (g_pGroundTextureSRV) g_pGroundTextureSRV->Release();
    if (g_pGroundTexture) g_pGroundTexture->Release();
    if (g_pBlendVertexLayout) g_pBlendVertexLayout->Release();
    if (g_pBlendVertexShader) g_pBlendVertexShader->Release();
    if (g_pSimpleVertexLayout) g_pSimpleVertexLayout->Release();
    if (g_pSimpleVertexShader) g_pSimpleVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pDepthStencil) g_pDepthStencil->Release();
    if (g_pDepthStencilView) g_pDepthStencilView->Release();
    if (g_pShadowBuffer) g_pShadowBuffer->Release();
    if (g_pShadowBufferView) g_pShadowBufferView->Release();
    if (g_pShadowBufferSRV) g_pShadowBufferSRV->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    for (uint32_t i = 0; i < g_vBufferMgr.size(); i++)
    {
        if (g_vBufferMgr[i])
        {
            g_vBufferMgr[i]->Release();
        }
    }

    for (uint32_t i = 0; i < g_iBufferMgr.size(); i++)
    {
        if (g_iBufferMgr[i])
        {
            g_iBufferMgr[i]->Release();
        }
    }

    for (uint32_t i = 0; i < g_textureMgr.size(); i++)
    {
        if (g_textureMgr[i])
        {
            g_textureMgr[i]->Release();
        }
    }

    for (uint32_t i = 0; i < g_srvMgr.size(); i++)
    {
        if (g_srvMgr[i])
        {
            g_srvMgr[i]->Release();
        }
    }
}

const aiNodeAnim* FindAnimationNode(const aiAnimation* pAnimations, const string& name)
{
    const aiNodeAnim* pAnimNode = nullptr;
    for (uint32_t i = 0; i < pAnimations->mNumChannels; i++)
    {
        const size_t len = strlen(pAnimations->mChannels[i]->mNodeName.data);
        if (strncmp(pAnimations->mChannels[i]->mNodeName.data, name.c_str(), len) == 0)
        {
            pAnimNode = pAnimations->mChannels[i];
            break;
        }
    }

    return pAnimNode;
}

template <typename T>
uint32_t FindKeyframe(float time, uint32_t numKeys, T keys)
{
    uint32_t keyFrame = 0;
    for (uint32_t i = 0; i < numKeys - 1; i++)
    {
        if (time < keys[i+1].mTime)
        {
            keyFrame = i;
            break;
        }
    }

    return keyFrame;
}

void InterpolateScaling(aiVector3D& v, float time, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1)
    {
        v = pNodeAnim->mScalingKeys[0].mValue;
    }
    else
    {
        const uint32_t keyFrame = FindKeyframe(time, pNodeAnim->mNumScalingKeys, pNodeAnim->mScalingKeys);
        const uint32_t nextFrame = keyFrame + 1;
        assert(nextFrame < pNodeAnim->mNumScalingKeys);
        const float timeBetweenFrames = static_cast<float>(pNodeAnim->mScalingKeys[nextFrame].mTime - pNodeAnim->mScalingKeys[keyFrame].mTime);
        const float offset = static_cast<float>(time - pNodeAnim->mScalingKeys[keyFrame].mTime);
        const float t = offset / timeBetweenFrames;
        assert(t >= 0 && t <= 1.0);
        const aiVector3D& start = pNodeAnim->mScalingKeys[keyFrame].mValue;
        const aiVector3D& end = pNodeAnim->mScalingKeys[nextFrame].mValue;
        aiVector3D delta = end - start;
        v = start + t * delta;
    }
}

void InterpolateRotation(aiQuaternion& q, float time, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumRotationKeys == 1)
    {
        q = pNodeAnim->mRotationKeys[0].mValue;
        q = q.Normalize();
    }
    else
    {
        uint32_t keyFrame = FindKeyframe(time, pNodeAnim->mNumRotationKeys, pNodeAnim->mRotationKeys);
        const uint32_t nextFrame = keyFrame + 1;
        assert(nextFrame < pNodeAnim->mNumRotationKeys);
        const float timeBetweenFrames = static_cast<float>(pNodeAnim->mRotationKeys[nextFrame].mTime - pNodeAnim->mRotationKeys[keyFrame].mTime);
        const float offset = static_cast<float>(time - pNodeAnim->mRotationKeys[keyFrame].mTime);
        const float t = offset / timeBetweenFrames;
        assert(t >= 0 && t <= 1.0);
        const aiQuaternion& start = pNodeAnim->mRotationKeys[keyFrame].mValue;
        const aiQuaternion& end = pNodeAnim->mRotationKeys[nextFrame].mValue;
        aiQuaternion::Interpolate(q, start, end, t);
        q = q.Normalize();
    }
}

void InterpolateTranslation(aiVector3D& v, float time, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1)
    {
        v = pNodeAnim->mPositionKeys[0].mValue;
    }
    else
    {
        uint32_t keyFrame = FindKeyframe(time, pNodeAnim->mNumPositionKeys, pNodeAnim->mPositionKeys);
        const uint32_t nextFrame = keyFrame + 1;
        assert(nextFrame < pNodeAnim->mNumPositionKeys);
        const float timeBetweenFrames = static_cast<float>(pNodeAnim->mPositionKeys[nextFrame].mTime - pNodeAnim->mPositionKeys[keyFrame].mTime);
        const float offset = static_cast<float>(time - pNodeAnim->mPositionKeys[keyFrame].mTime);
        const float t = offset / timeBetweenFrames;
        assert(t >= 0 && t <= 1.0);
        const aiVector3D& start = pNodeAnim->mPositionKeys[keyFrame].mValue;
        const aiVector3D& end = pNodeAnim->mPositionKeys[nextFrame].mValue;
        aiVector3D delta = end - start;
        v = start + t * delta;
    }
}

void UpdateNodeHierarchy(float time, const aiNode* pNode, const aiAnimation* pAnimation, const DirectX::XMMATRIX& parentTansformMatrix)
{
    string name(pNode->mName.data);

    // World transformation we are trying to calculate
    DirectX::XMMATRIX currentNodeTransformation = aiToXMMATRIX(pNode->mTransformation);

    const aiNodeAnim* pAnimationNode = FindAnimationNode(pAnimation, name);

    // If we skip this we will get bind pose
    if (pAnimationNode)
    {
        // interpolate scaling
        aiVector3D aiScale;
        InterpolateScaling(aiScale, time, pAnimationNode);
        aiMatrix4x4 scaleM;
        aiMatrix4x4::Scaling(aiScale, scaleM);

        // interpolate rotation
        aiQuaternion aiRotationQ;
        InterpolateRotation(aiRotationQ, time, pAnimationNode);
        aiMatrix4x4 rotationM = aiMatrix4x4(aiRotationQ.GetMatrix());

        // interpolate translation
        aiVector3D aiTranslation;
        InterpolateTranslation(aiTranslation, time, pAnimationNode);
        aiMatrix4x4 translationM;
        aiMatrix4x4::Translation(aiTranslation, translationM);

        aiMatrix4x4 aiCurrentNodeTransformation = translationM * rotationM * scaleM;
        currentNodeTransformation = aiToXMMATRIX(aiCurrentNodeTransformation);
    }

    DirectX::XMMATRIX globalTransformMatrix = parentTansformMatrix * currentNodeTransformation;

    aiMatrix4x4 aiGlobalInverseTransform = g_pScene->mRootNode->mTransformation;
    aiGlobalInverseTransform = aiGlobalInverseTransform.Inverse();
    DirectX::XMMATRIX globalInverseTransform = aiToXMMATRIX(aiGlobalInverseTransform);

    // note: Not all nodes are bones, but we still have to calculate the transformation in order of the hierarchy
    //       If assimp runs into certain transformations, it will insert helper nodes and break out the 
    //       scale/translate/rotate transforms out of the bone node into separate nodes, and apply animation to those. 
    if (g_skinnedMesh->m_boneMap.find(name) != g_skinnedMesh->m_boneMap.end())
    {
        uint32_t boneIdx = g_skinnedMesh->m_boneMap[name];
        g_skinnedMesh->m_boneInfo[boneIdx].finalTransformation =
            globalInverseTransform * globalTransformMatrix * g_skinnedMesh->m_boneInfo[boneIdx].boneOffset;
    }

    for (uint32_t i = 0; i < pNode->mNumChildren; i++)
    {
        UpdateNodeHierarchy(time, pNode->mChildren[i], pAnimation, globalTransformMatrix);
    }
}

void UpdateAnimation(float time, uint32_t animIdx)
{
    // Get animation node
    const aiAnimation* pAnimation = g_pScene->mAnimations[animIdx];

    // Animation time
    const float ticksPerSecond = static_cast<float>(pAnimation->mTicksPerSecond ? pAnimation->mTicksPerSecond : 25.0f);
    const float timeInTicks = time * ticksPerSecond;
    const float animationTime = fmod(timeInTicks, static_cast<float>(pAnimation->mDuration));

    UpdateNodeHierarchy(animationTime, g_pScene->mRootNode, pAnimation, DirectX::XMMatrixIdentity());
}

void ProcessInput()
{
    float speed = 0.1f;
    if (keys['W']) g_camera.position += g_camera.direction * 1.0f * speed * g_t;
    if (keys['S'] )g_camera.position -= g_camera.direction * 1.0f * speed * g_t;
    if (keys['A']) g_camera.position -= g_camera.right * 1.0f * speed * g_t;
    if (keys['D']) g_camera.position += g_camera.right * 1.0f * speed * g_t;
}

void updateView()
{

}

void Render()
{
    HRESULT hr = S_OK;
    // Update our time
    g_t = 0.0f;
    g_timeCurrent = GetTickCount();
    if (g_timeStart == 0)
    {
        g_timeStart = g_timeCurrent;
    }
    g_t = (g_timeCurrent - g_timeStart) / 1000.0f;

    //
    // Process Input
    //
    ProcessInput();

    //
    // Update animation
    //
    UpdateAnimation(g_t, 0);

    // 
    // Update bone data
    //
    D3D11_MAPPED_SUBRESOURCE mapInfo = {};
    hr = g_pImmediateContext->Map(g_pBoneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapInfo);
    assert(S_OK == hr);

    for (uint32_t i = 0; i < g_skinnedMesh->m_numBones; i++)
    {
        static_cast<DirectX::XMMATRIX*>(mapInfo.pData)[i] = g_skinnedMesh->m_boneInfo[i].finalTransformation;
    }

    g_pImmediateContext->Unmap(g_pBoneBuffer, 0);

    //g_View = XMMatrixTranslation(-(g_camera.position.x), -(g_camera.position.y), -(g_camera.position.z));
    g_View = XMMatrixLookAtRH(g_camera.position, g_camera.position + g_camera.direction, g_camera.up);

    //g_World = DirectX::XMMatrixRotationY(t);
    g_World = DirectX::XMMatrixIdentity();

    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.525f, 0.7f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    g_pImmediateContext->ClearDepthStencilView(g_pShadowBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    //
    // Update variables that change once per frame
    //
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose(g_World);
    cb.vMeshColor = g_vMeshColor;
    g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

    uint32_t offset = 0;
    uint32_t vStride = sizeof(SimpleVertex);
    uint32_t iStride = sizeof(uint16_t);

    //
    // Shadow pass
    //
    g_pImmediateContext->PSSetShaderResources(0, 1, g_nullSRV);
    g_pImmediateContext->PSSetShaderResources(1, 1, g_nullSRV);
    g_pImmediateContext->OMSetRenderTargets(0, nullptr, g_pShadowBufferView);

    CBVSUniforms cbVsUniforms;
    cbVsUniforms.mView = XMMatrixTranspose(g_LightView);
    cbVsUniforms.mLightView = XMMatrixTranspose(g_LightView);
    g_pImmediateContext->UpdateSubresource(g_pCBVsUniforms, 0, nullptr, &cbVsUniforms, 0, 0);

    g_pImmediateContext->VSSetShader(g_pSimpleVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pDepthShader, nullptr, 0);
    g_pImmediateContext->OMSetDepthStencilState(g_pDSState, 0x1);

    // Draw Ground
    g_pImmediateContext->VSSetShader(g_pSimpleVertexShader, nullptr, 0);
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pGroundVB, &vStride, &offset);
    g_pImmediateContext->IASetIndexBuffer(g_pGroundIB, DXGI_FORMAT_R16_UINT, 0);
    g_pImmediateContext->DrawIndexed(6, 0, 0);

    // Draw Model
    g_pImmediateContext->VSSetShader(g_pBlendVertexShader, nullptr, 0);

    for (uint32_t i = 0; i < g_skinnedMesh->m_numMeshes; i++)
    {
        uint32_t vIdx = g_skinnedMesh->m_entries[i].vBufIdx;
        uint32_t iIdx = g_skinnedMesh->m_entries[i].iBufIdx;
        uint32_t mIdx = g_skinnedMesh->m_entries[i].materialIdx;
        uint32_t numIndices = static_cast<uint32_t>(g_skinnedMesh->m_entries[i].indices.size());
        uint32_t vStride = sizeof(BlendVertex);
        uint32_t iStride = sizeof(uint16_t);

        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_vBufferMgr[vIdx], &vStride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_iBufferMgr[iIdx], DXGI_FORMAT_R16_UINT, 0);
        g_pImmediateContext->DrawIndexed(numIndices, 0, 0);
    }

    // Actual render pass

    // 
    // Render the ground
    //
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    cbVsUniforms.mView = XMMatrixTranspose(g_View);
    cbVsUniforms.mLightView = XMMatrixTranspose(g_LightView);
    g_pImmediateContext->UpdateSubresource(g_pCBVsUniforms, 0, nullptr, &cbVsUniforms, 0, 0);

    g_pImmediateContext->VSSetShader(g_pSimpleVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBVsUniforms);
    g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
    g_pImmediateContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_pImmediateContext->PSSetSamplers(1, 1, &g_pSamplerClamp);
    g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBPsUniforms);
    g_pImmediateContext->RSSetState(g_pRasterState);
    g_pImmediateContext->PSSetShaderResources(0, 1, &g_pGroundTextureSRV);
    g_pImmediateContext->PSSetShaderResources(1, 1, &g_pShadowBufferSRV);

    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pGroundVB, &vStride, &offset);
    g_pImmediateContext->IASetIndexBuffer(g_pGroundIB, DXGI_FORMAT_R16_UINT, 0);
    g_pImmediateContext->DrawIndexed(6, 0, 0);

    //
    // Render the model
    //
    g_pImmediateContext->VSSetShader(g_pBlendVertexShader, nullptr, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBVsUniforms);
    g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
    g_pImmediateContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_pImmediateContext->VSSetShaderResources(0, 1, &g_pBoneSRV);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_pImmediateContext->PSSetSamplers(1, 1, &g_pSamplerClamp);
    g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBPsUniforms);
    g_pImmediateContext->RSSetState(g_pRasterState);

    for (uint32_t i = 0; i < g_skinnedMesh->m_numMeshes; i++)
    {
        uint32_t vIdx = g_skinnedMesh->m_entries[i].vBufIdx;
        uint32_t iIdx = g_skinnedMesh->m_entries[i].iBufIdx;
        uint32_t mIdx = g_skinnedMesh->m_entries[i].materialIdx;
        uint32_t numIndices = static_cast<uint32_t>(g_skinnedMesh->m_entries[i].indices.size());
        uint32_t vStride = sizeof(BlendVertex);
        uint32_t iStride = sizeof(uint16_t);

        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_vBufferMgr[vIdx], &vStride, &offset);
        g_pImmediateContext->IASetIndexBuffer(g_iBufferMgr[iIdx], DXGI_FORMAT_R16_UINT, 0);
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_srvMgr[mIdx]);
        g_pImmediateContext->PSSetShaderResources(1, 1, &g_pShadowBufferSRV);
        g_pImmediateContext->DrawIndexed(numIndices, 0, 0);
    }

    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present(0, 0);
}

void onMouseMove(int x, int y, int deltaX, int deltaY)
{
    bool invertMouse = false;
    float mouseSensitivity = 0.001f;

    // note: this is here because SetCursorPos will generate a WM_MOUSEMOVE message, 
    // so we want to ignore that one
    static bool changed = false;
    if (changed = !changed) 
    {
        g_camera.dx += mouseSensitivity * g_t * (WIDTH / 2 - x);
        g_camera.dy += (invertMouse ? -1 : 1) * mouseSensitivity * g_t * (HEIGHT / 2 - y);

        XMFLOAT3 direction = XMFLOAT3(cos(g_camera.dy) * sin(g_camera.dx), sin(g_camera.dy), cos(g_camera.dy) * cos(g_camera.dx));
        g_camera.direction = DirectX::XMLoadFloat3(&direction);
 
        //XMFLOAT3 up(0.0f, 1.0f, 0.0f);
        //XMVECTOR upv = XMLoadFloat3(&up);

        //g_camera.right = XMVector3Cross(upv, g_camera.direction);

        XMFLOAT3 right = XMFLOAT3(sin(g_camera.dx - 3.1415f / 2.0f), 0, cos(g_camera.dx - 3.1415f / 2.0f));
        g_camera.right = DirectX::XMLoadFloat3(&right);

        g_camera.up = XMVector3Cross(g_camera.right, g_camera.direction);

#if _DEBUG
        char str[1024];
        wchar_t wc[1024];
        sprintf_s((char*)str, 1024, "dx: %f, dy: %f\n", g_camera.dx, g_camera.dy);
        const size_t cSize = strlen(str) + 1;
        size_t outSize;
        mbstowcs_s(&outSize, wc, cSize, str, cSize-1);
        OutputDebugStringW(wc);
#endif

        POINT point = { WIDTH / 2, HEIGHT / 2 };
        ClientToScreen(g_hWnd, &point);
        SetCursorPos(point.x, point.y);
        ShowCursor(false);
    }
}

void onKey(const uint32_t key, bool pressed)
{
    if (pressed)
    {
        keys[key] = 1;
    }
    else
    {
        keys[key] = 0;
    }
}

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_MOUSEMOVE:
        static int lastX, lastY;
        int x, y;
        x = GETX(lParam);
        y = GETY(lParam);
        onMouseMove(x, y, x - lastX, y - lastY);
        lastX = x;
        lastY = y;
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else
        {
            onKey((unsigned int)wParam, true);
        }
        
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        onKey((unsigned int)wParam, false);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}