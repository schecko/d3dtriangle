
#include <windows.h>
#include <stdint.h>
#include <iostream>
#include <d3d11.h>
#include <D3Dcompiler.h>


#define DEBUGGING true

enum ShaderType
{
    pixelShader,
    vertexShader

};

struct Vertex
{
    float x, y, z;
    float r, g, b, a;

};

struct MainMemory
{
    HINSTANCE exeHandle;
    HWND windowHandle;
    HWND consoleHandle;

    uint32_t clientWidth, clientHeight;

    IDXGISwapChain* dSwapChain;
    ID3D11Device* dDevice;
    ID3D11DeviceContext* dContext;
    ID3D11RenderTargetView *backBuffer;

    ID3D11PixelShader* pixelShader;
    ID3D11VertexShader* vertexShader;

    ID3D11Buffer* vertexBuffer;

    Vertex vertexData[3];
    ID3D11InputLayout* vertexLayout;

    union
    {
        bool running;
        bool initialized;
    };

};





void Assert(bool test, const char* message)
{
#if DEBUGGING == true
    char x;
    if (!test)
    {
        HWND consoleHandle = GetConsoleWindow();
        ShowWindow(consoleHandle, SW_SHOW);
        SetFocus(consoleHandle);
        std::cout << message << std::endl;
        std::cout << "Type anything and press enter if you wish to continue anyway; best to exit though." << std::endl;
        std::cin >> x;
        ShowWindow(consoleHandle, SW_HIDE);
    }
#endif
}

//handle the windows messages
LRESULT CALLBACK MessageHandler(HWND hwnd, UINT msg, WPARAM wP, LPARAM lP)
{

    MainMemory* mainMemory = (MainMemory*)GetWindowLongPtr(hwnd, 0);
    switch (msg)
    {
        case WM_CREATE:
        {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lP;
            SetWindowLongPtrW(hwnd, 0, (LONG_PTR)cs->lpCreateParams);
        }
        break;
        case WM_DESTROY:
        case WM_CLOSE:
        {
            mainMemory->running = false;
        }
        break;
        default:
        {
            return DefWindowProc(hwnd, msg, wP, lP);
        }
    }
    return 0;
}

//returns a handle to the created window.
HWND NewWindow(const char* name, const uint32_t clientWidth, const uint32_t clientHeight, void* pointer = nullptr)
{


    WNDCLASS wc = {};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MessageHandler;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = name;
    wc.lpszMenuName = name;
    wc.cbWndExtra = sizeof(void*);


    ATOM result = 0;
    result = RegisterClass(&wc);

    Assert(result != 0, "could not register windowclass");

    RECT wr = { 0, 0, (LONG)clientWidth, (LONG)clientHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    //TODO calculate windowrect from clientrect
    uint32_t windowWidth = wr.right - wr.left;
    uint32_t windowHeight = wr.bottom - wr.top;

    HWND windowHandle = CreateWindow(name,
                                     name,
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     windowWidth,
                                     windowHeight,
                                     nullptr,
                                     nullptr,
                                     wc.hInstance,
                                     pointer);

    Assert(windowHandle != nullptr, "could not create a windows window");

    ShowWindow(windowHandle, SW_SHOW);

    return windowHandle;
}

void InitD3D(const HWND windowHandle, IDXGISwapChain** dSwapChain, ID3D11Device** dDevice, ID3D11DeviceContext** dContext, uint32_t clientWidth, uint32_t clientHeight)
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = clientWidth;
    scd.BufferDesc.Height = clientHeight;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = windowHandle;
    scd.SampleDesc.Count = 4;
    scd.Windowed = true;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D11CreateDeviceAndSwapChain(nullptr,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  nullptr,
                                  0,
                                  nullptr,
                                  0,
                                  D3D11_SDK_VERSION,
                                  &scd,
                                  dSwapChain,
                                  dDevice,
                                  nullptr,
                                  dContext);

    Assert(dSwapChain != nullptr, "could not create swapchain");
    Assert(dDevice != nullptr, "could not create device");
    Assert(dContext != nullptr, "could not create context");




}
void QuitD3D(MainMemory* m)
{
    m->dSwapChain->SetFullscreenState(false, nullptr);

    m->vertexLayout->Release();

    m->vertexShader->Release();
    m->pixelShader->Release();

    m->dSwapChain->Release();
    m->backBuffer->Release();
    m->dDevice->Release();
    m->dContext->Release();
}

void SetRenderTarget(IDXGISwapChain* dSwapChain, ID3D11Device* dDevice, ID3D11DeviceContext* dContext, ID3D11RenderTargetView** backBuffer)
{
    ID3D11Texture2D* textureBuffer;
    dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&textureBuffer);
    dDevice->CreateRenderTargetView(textureBuffer, nullptr, backBuffer);
    textureBuffer->Release();

    dContext->OMSetRenderTargets(1, backBuffer, nullptr);
}

void SetViewport(ID3D11DeviceContext* dContext, uint32_t clientWidth, uint32_t clientHeight)
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)clientWidth;
    viewport.Height = (FLOAT)clientHeight;

    dContext->RSSetViewports(1, &viewport);
}

void Render(IDXGISwapChain* dSwapChain, ID3D11Device* dDevice, ID3D11DeviceContext* dContext, ID3D11RenderTargetView* backBuffer, void* vertexData, uint32_t vertexStride)
{
    FLOAT color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    dContext->ClearRenderTargetView(backBuffer, color);
    uint32_t offset = 0;
    dContext->IASetVertexBuffers(0, 1,(ID3D11Buffer**) &vertexData, &vertexStride, &offset);
    dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dContext->Draw(3, 0);


    dSwapChain->Present(0, 0);
}

ID3DBlob* LoadShader(ID3D11Device* dDevice, const wchar_t* name, ShaderType shaderType, void* shader)
{
    ID3DBlob* shaderData = nullptr;
    switch (shaderType)
    {
        case vertexShader:
        {
            D3DCompileFromFile(name, 0, 0, "VShader", "vs_4_0", 0, 0, &shaderData, 0);
            Assert(shaderData != nullptr, "could not load shader");
            dDevice->CreateVertexShader(shaderData->GetBufferPointer(), shaderData->GetBufferSize(), nullptr, (ID3D11VertexShader**)shader);
        }
        break;
        case pixelShader:
        {
            D3DCompileFromFile(name, 0, 0, "PShader", "ps_4_0", 0, 0, &shaderData, 0);
            Assert(shaderData != nullptr, "could not load shader");
            dDevice->CreatePixelShader(shaderData->GetBufferPointer(), shaderData->GetBufferSize(), nullptr, (ID3D11PixelShader**)shader);
        }
        break;
        default:
        {
            Assert(0, "loadshader does not have a proper shadertype passed to it.");
        }
    }
    return shaderData;
}

void MoveDataToGPU(ID3D11DeviceContext* dContext, ID3D11Device* dDevice, void* data, uint32_t dataSize, ID3D11Buffer* gpuBuffer)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = dataSize;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dDevice->CreateBuffer(&bd, nullptr, &gpuBuffer);

    D3D11_MAPPED_SUBRESOURCE ms;
    dContext->Map(gpuBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    memcpy(ms.pData, data, dataSize);
    dContext->Unmap(gpuBuffer, 0);
}

void InitPipeline(MainMemory* m)
{

    LoadShader(m->dDevice, L"D:\\scottsdocs\\sourcecode\\d3dtriangle\\build\\shaders\\shaders.shaders", ShaderType::pixelShader, &m->pixelShader);
    ID3DBlob *vertexShaderData  = LoadShader(m->dDevice, L"D:\\scottsdocs\\sourcecode\\d3dtriangle\\build\\shaders\\shaders.shaders", ShaderType::vertexShader, &m->vertexShader);
    m->dContext->PSSetShader(m->pixelShader, 0, 0);
    m->dContext->VSSetShader(m->vertexShader, 0, 0);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float[3]), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m->dDevice->CreateInputLayout(ied, 2, vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), &m->vertexLayout);
    m->dContext->IASetInputLayout(m->vertexLayout);
}

void Init(MainMemory* m)
{

    m->clientWidth = 1200;
    m->clientHeight = 800;
    m->vertexData[0] = { 0.0f, 0.5f, 0.0f, 0.1f, 0.4f, 0.6f, 1.0f };
    m->vertexData[1] = { 0.5f, -0.5f, 0.0f, 0.2f, 0.5f, 0.7f, 1.0f };
    m->vertexData[2] = { -0.5f, -0.5f, 0.0f, 0.3f, 0.6f, 0.8f, 1.0f };
    m->windowHandle = NewWindow("d3dtriangle", m->clientWidth, m->clientHeight, m);

    InitD3D(m->windowHandle, &m->dSwapChain, &m->dDevice, &m->dContext, m->clientWidth, m->clientHeight);
    SetRenderTarget(m->dSwapChain, m->dDevice, m->dContext, &m->backBuffer);
    SetViewport(m->dContext, m->clientWidth, m->clientHeight);
    MoveDataToGPU(m->dContext, m->dDevice, &m->vertexData, sizeof(m->vertexData), m->vertexBuffer);
    InitPipeline(m);

    m->initialized = true;
}

void Frame(MainMemory* m)
{
    Render(m->dSwapChain, m->dDevice, m->dContext, m->backBuffer, &m->vertexBuffer, sizeof(Vertex));
}

void Quit(MainMemory* m)
{
    QuitD3D(m);
    DestroyWindow(m->windowHandle);
}

int main()
{
    MainMemory* m = new MainMemory();
    Init(m);

    while (m->running)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Frame(m);

    }
    Quit(m);
    return 0;
}
