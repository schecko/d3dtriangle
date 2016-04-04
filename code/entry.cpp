
#include <windows.h>
#include <stdint.h>
#include <iostream>
#include <d3d11.h>
#include <D3Dcompiler.h>


#define DEBUGGING true

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

    ID3D10Blob* pixelShader;
    ID3D10Blob* vertexShader;

    union
    {
        bool running;
        bool initialized;
    };

};

enum ShaderType
{
    pixelShader,
    vertexShader

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
void QuitD3D(IDXGISwapChain* dSwapChain, ID3D11Device* dDevice, ID3D11DeviceContext* dContext)
{
    dSwapChain->SetFullscreenState(false, nullptr);
    dSwapChain->Release();
    dDevice->Release();
    dContext->Release();
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

void Render(IDXGISwapChain* dSwapChain, ID3D11Device* dDevice, ID3D11DeviceContext* dContext, ID3D11RenderTargetView* backBuffer)
{
    FLOAT color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    dContext->ClearRenderTargetView(backBuffer, color);


    dSwapChain->Present(0, 0);
}

void LoadShader(ID3D11Device* dDevice, const char* name, ShaderType shaderType, void* shader)
{
    ID3D10Blob* data = nullptr;
    switch (shaderType)
    {
        case vertexShader:
        {
            D3DCompileFromFile((LPCWSTR)name, 0, 0, "VShader", "vs_4_0", 0, 0, &data, 0);
            Assert(data != nullptr, "could not load shader");
            dDevice->CreateVertexShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, (ID3D11VertexShader**)&shader);
        }
        break;
        case pixelShader:
        {
            D3DCompileFromFile((LPCWSTR)name, 0, 0, "VShader", "ps_4_0", 0, 0, &data, 0);
            Assert(data != nullptr, "could not load shader");
            dDevice->CreatePixelShader(data->GetBufferPointer(), data->GetBufferSize(), nullptr, (ID3D11PixelShader**)&shader);
        }
        break;
        default:
        {
            Assert(0, "loadshader does not have a proper shadertype passed to it.");
        }
    }
}

int main()
{
    MainMemory* m = new MainMemory();
    m->running = true;
    m->clientWidth = 1200;
    m->clientHeight = 800;
    m->windowHandle = NewWindow("d3dtriangle", m->clientWidth, m->clientHeight, m);

    InitD3D(m->windowHandle, &m->dSwapChain, &m->dDevice, &m->dContext, m->clientWidth, m->clientHeight);
    SetRenderTarget(m->dSwapChain, m->dDevice, m->dContext, &m->backBuffer);
    SetViewport(m->dContext, m->clientWidth, m->clientHeight);

    LoadShader(m->dDevice, "D:\\scottsdocs\\sourcecode\\d3dtriangle\\build\\shaders\\shaders.shaders", ShaderType::pixelShader, m->pixelShader);
    LoadShader(m->dDevice, "D:\\scottsdocs\\sourcecode\\d3dtriangle\\build\\shaders\\shaders.shaders", ShaderType::vertexShader, m->vertexShader);



    while (m->running)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Render(m->dSwapChain, m->dDevice, m->dContext, m->backBuffer);
    }

    QuitD3D(m->dSwapChain, m->dDevice, m->dContext);

    return 0;
}
