#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <iostream>
#include <string>
#include <fstream>
#include <strstream>
#include <vector>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

using namespace DirectX;

const char shader_code[] = R"(
    cbuffer CBuf : register(b0) { float4x4 mvp; }
    struct Vin { float4 pos : POSITION; float4 color : COLOR; };
    struct Vout { float4 pos : SV_POSITION; float4 color : COLOR; };
    Vout vs(Vin input) {
        Vout output;
        output.pos = mul(input.pos, mvp);
        output.color = input.color;
        return output;
    }
    float4 ps(Vout input) : SV_TARGET { return input.color; }
)";

struct Vertex { float x, y, z; float r, g, b; };

bool LoadFromObjectFile(std::string fileName, Vertex** vertsModel, unsigned short** indsModel, int* vertsModelCnt, int* indsModelCnt)
{
    std::ifstream f(fileName);
	if (!f.is_open()) return false;

	std::vector<Vertex> verts;
	std::vector<unsigned short> inds;

    while (!f.eof())
    {
        char line[128];
        f.getline(line, 128);

        std::strstream s;
        s << line;
        char junk;

        if (line[0] == 'v')
        {
            Vertex v;
            s >> junk >> v.x >> v.y >> v.z;
            v.r = rand() % 2; v.g = rand() % 2; v.b = rand() % 2;
            verts.push_back(v);
        }

        else if (line[0] == 'f')
        {
            s >> junk;
            std::string token;
            std::vector<unsigned short> faceInds;

            while(s >> token)
            {
				int pos = token.find('/');
                if (pos != std::string::npos)
                    token = token.substr(0, pos);
				faceInds.push_back((unsigned short)(std::stoi(token) - 1));
			}
            if (faceInds.size() == 4)
            {
                // triangulo 1
				inds.push_back(faceInds[0]);
				inds.push_back(faceInds[1]);
                inds.push_back(faceInds[2]);
				// triangulo 2
				inds.push_back(faceInds[0]);
                inds.push_back(faceInds[2]);
				inds.push_back(faceInds[3]);
            }
            else if (faceInds.size() == 3)
            {
				inds.push_back(faceInds[0]);
				inds.push_back(faceInds[1]);
				inds.push_back(faceInds[2]);
            }
        }
    }

	*vertsModelCnt = (int)verts.size();
	*indsModelCnt = (int)inds.size();

	*vertsModel = new Vertex[*vertsModelCnt];
	*indsModel = new unsigned short[*indsModelCnt];

    for(int i = 0; i < *vertsModelCnt; i++)
		(*vertsModel)[i] = verts[i];

	for (int i = 0; i < *indsModelCnt; i++)
		(*indsModel)[i] = inds[i];
    
    f.close();

    return true;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	int height = 600, width = 800;
    HWND window = CreateWindowExA(0, "STATIC", "Sistemas Multimidia - DirectX", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
        width, height, 0, 0, 0, 0);

    IDXGISwapChain* swapchain;
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11RenderTargetView* rtv;
    ID3D11DepthStencilView* dsv;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = window;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_DEBUG, 0, 0, D3D11_SDK_VERSION, &scd, &swapchain, &device, 0, &context);

    ID3D11Texture2D* backbuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
    device->CreateRenderTargetView(backbuffer, 0, &rtv);

    D3D11_TEXTURE2D_DESC dsd = {};
    backbuffer->GetDesc(&dsd);
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* depthbuffer;
    device->CreateTexture2D(&dsd, 0, &depthbuffer);
    device->CreateDepthStencilView(depthbuffer, 0, &dsv);
    backbuffer->Release();
    depthbuffer->Release();

    ID3DBlob* vs_blob, * ps_blob;
    D3DCompile(shader_code, sizeof(shader_code), 0, 0, 0, "vs", "vs_5_0", 0, 0, &vs_blob, 0);
    D3DCompile(shader_code, sizeof(shader_code), 0, 0, 0, "ps", "ps_5_0", 0, 0, &ps_blob, 0);
    ID3D11VertexShader* vs; device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), 0, &vs);
    ID3D11PixelShader* ps; device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), 0, &ps);

    Vertex* vertices = nullptr;
    unsigned short* indices = nullptr;
	int vertsCnt = 0, indsCnt = 0;

	LoadFromObjectFile("Jet.obj", &vertices, &indices, &vertsCnt, &indsCnt);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = sizeof(Vertex) * vertsCnt;
    D3D11_SUBRESOURCE_DATA sd = { vertices };
    ID3D11Buffer* vbuf; device->CreateBuffer(&bd, &sd, &vbuf);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = sizeof(unsigned short) * indsCnt;
    sd.pSysMem = indices;
    ID3D11Buffer* ibuf; device->CreateBuffer(&bd, &sd, &ibuf);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = 64;
    ID3D11Buffer* cbuf; device->CreateBuffer(&bd, 0, &cbuf);
    D3D11_INPUT_ELEMENT_DESC ied[] = { {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}, {"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0} };
    ID3D11InputLayout* il; device->CreateInputLayout(ied, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &il);

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_BACK;
    ID3D11RasterizerState* rs;
    device->CreateRasterizerState(&rd, &rs);

    UINT stride = sizeof(Vertex), offset = 0;
    D3D11_VIEWPORT vp = { 0, 0, (float)dsd.Width, (float)dsd.Height, 0, 1 };
    context->IASetInputLayout(il);
    context->IASetVertexBuffers(0, 1, &vbuf, &stride, &offset);
    context->IASetIndexBuffer(ibuf, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->RSSetState(rs);
    context->VSSetShader(vs, 0, 0);
    context->VSSetConstantBuffers(0, 1, &cbuf);
    context->RSSetViewports(1, &vp);
    context->PSSetShader(ps, 0, 0);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        else {
            float t = (float)GetTickCount64() / 1000.0f;
            XMMATRIX model = XMMatrixScaling(0.25f, 0.25f, 0.25f) * XMMatrixRotationY(t) * XMMatrixRotationX(t / 2.0f);
            XMMATRIX view = XMMatrixLookAtLH({ 0, 1, -25 }, { 0, 0, 0 }, { 0, 1, 0 });
            XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)dsd.Width / dsd.Height, 0.1f, 50.0f);
            XMMATRIX mvp = model * view * proj;
            mvp = XMMatrixTranspose(mvp);

            D3D11_MAPPED_SUBRESOURCE ms;
            context->Map(cbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
            memcpy(ms.pData, &mvp, sizeof(mvp));
            context->Unmap(cbuf, 0);

            context->OMSetRenderTargets(1, &rtv, dsv);

            float color[] = { 0.1f, 0.1f, 0.2f, 1.0f };
            context->ClearRenderTargetView(rtv, color);
            context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
            context->DrawIndexed((UINT)indsCnt, 0, 0);
            swapchain->Present(1, 0);
        }
    }
    return 0;
}