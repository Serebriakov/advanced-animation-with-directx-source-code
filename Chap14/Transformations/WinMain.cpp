#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Sky vertex structure, fvf, vertex buffer, and texture
typedef struct {
  float x, y, z, rhw;
  float u, v;
} sSkyVertex;
#define SKYFVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
IDirect3DVertexBuffer9 *g_SkyVB = NULL;
IDirect3DTexture9      *g_SkyTexture = NULL;

// Land and water meshes
D3DXMESHCONTAINER_EX *g_WaterMesh = NULL;
D3DXMESHCONTAINER_EX *g_LandMesh  = NULL;

// Window class and caption text
char g_szClass[]   = "TextureTransformationClass";
char g_szCaption[] = "Texture Transformation Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Matrix4x4To3x3(D3DXMATRIX *matOut, D3DXMATRIX *matIn);

BOOL DoInit(HWND hWnd, BOOL Windowed = TRUE);
void DoShutdown();
void DoFrame();

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
  WNDCLASSEX wcex;
  MSG        Msg;
  HWND       hWnd;

  // Initialize the COM system
  CoInitialize(NULL);

  // Create the window class here and register it
  wcex.cbSize        = sizeof(wcex);
  wcex.style         = CS_CLASSDC;
  wcex.lpfnWndProc   = WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInst;
  wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;
  wcex.lpszMenuName  = NULL;
  wcex.lpszClassName = g_szClass;
  wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
  if(!RegisterClassEx(&wcex))
    return FALSE;

  // Create the main window
  hWnd = CreateWindow(g_szClass, g_szCaption,
              WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
              0, 0, 640, 480,
              NULL, NULL, hInst, NULL);
  if(!hWnd)
    return FALSE;
  ShowWindow(hWnd, SW_NORMAL);
  UpdateWindow(hWnd);

  // Call init function and enter message pump
  if(DoInit(hWnd) == TRUE) {

    // Start message pump, waiting for user to exit
    ZeroMemory(&Msg, sizeof(MSG));
    while(Msg.message != WM_QUIT) {
      if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
      }

      // Render a single frame
      DoFrame();
    }
  }

  // Call shutdown
  DoShutdown();

  // Unregister the window class
  UnregisterClass(g_szClass, hInst);

  // Shut down the COM system
  CoUninitialize();

  return 0;
}

long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg,              \
                           WPARAM wParam, LPARAM lParam)
{
  // Only handle window destruction messages
  switch(uMsg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

BOOL DoInit(HWND hWnd, BOOL Windowed)
{
  // Initialize Direct3D
  InitD3D(&g_pD3D, &g_pD3DDevice, hWnd);

  // Load the land and water meshes
  LoadMesh(&g_WaterMesh, g_pD3DDevice, "..\\Data\\Water.x", "..\\Data\\");
  LoadMesh(&g_LandMesh,  g_pD3DDevice, "..\\Data\\Land.x",  "..\\Data\\");

  // Create the sky backdrop
  sSkyVertex SkyVerts[4] = {
    {   0.0f,   0.0, 1.0, 1.0f, 0.0f, 0.0f },
    { 640.0f,   0.0, 1.0, 1.0f, 1.0f, 0.0f },
    {   0.0f, 480.0, 1.0, 1.0f, 0.0f, 1.0f },
    { 640.0f, 480.0, 1.0, 1.0f, 1.0f, 1.0f }
  };
  g_pD3DDevice->CreateVertexBuffer(sizeof(SkyVerts), D3DUSAGE_WRITEONLY, SKYFVF, D3DPOOL_DEFAULT, &g_SkyVB, NULL);
  char *Ptr;
  g_SkyVB->Lock(0,0, (void**)&Ptr, 0);
  memcpy(Ptr, SkyVerts, sizeof(SkyVerts));
  g_SkyVB->Unlock();
  D3DXCreateTextureFromFile(g_pD3DDevice, "..\\Data\\Sky.bmp", &g_SkyTexture);

  // Setup a light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(Light));
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Type = D3DLIGHT_DIRECTIONAL;
  D3DXVECTOR3 vecLight = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
  D3DXVec3Normalize(&vecLight, &vecLight);
  Light.Direction = vecLight;
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  // Start playing a waterfall sound
  PlaySound("..\\Data\\Waterfall.wav", NULL, SND_ASYNC | SND_LOOP);

  return TRUE;
}

void DoShutdown()
{
  // Stop playing an ocean sound
  PlaySound(NULL, NULL, 0);

  // Free meshes
  delete g_WaterMesh; g_WaterMesh = NULL;
  delete g_LandMesh;  g_LandMesh  = NULL;

  // Release sky data
  ReleaseCOM(g_SkyVB);
  ReleaseCOM(g_SkyTexture);

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  // Create and set the view transformation
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(360.0f, -170.0f, -430.0f),
                               &D3DXVECTOR3(65.0f,    70.0f,  -15.0f), 
                               &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Set identity matrix for world transformation
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Draw the sky
    g_pD3DDevice->SetFVF(SKYFVF);
    g_pD3DDevice->SetStreamSource(0, g_SkyVB, 0, sizeof(sSkyVertex));
    g_pD3DDevice->SetTexture(0, g_SkyTexture);
    g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // Enable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Draw the land meshes
    DrawMeshes(g_LandMesh);

    // Setup the texture transformation
    float TimeFactor = (float)(timeGetTime() / 500.0f);
    D3DXMATRIX matTexture;
    D3DXMatrixTranslation(&matTexture, 0.0f, -TimeFactor, 0.0f);
    Matrix4x4To3x3(&matTexture, &matTexture);
    g_pD3DDevice->SetTransform(D3DTS_TEXTURE0, &matTexture);
    g_pD3DDevice->SetTextureStageState(0, 
                         D3DTSS_TEXTURETRANSFORMFLAGS, 
                         D3DTTFF_COUNT2);

    // Draw the water (using alpha blending)
    DrawMeshes(g_WaterMesh);
    g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Disable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Turn off texture transformations
    g_pD3DDevice->SetTextureStageState(0, 
                         D3DTSS_TEXTURETRANSFORMFLAGS, 
                         D3DTTFF_DISABLE);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void Matrix4x4To3x3(D3DXMATRIX *matOut, D3DXMATRIX *matIn)
{
  matOut->_11 = matIn->_11;   // Copy over 1st row
  matOut->_12 = matIn->_12; 
  matOut->_13 = matIn->_13; 
  matOut->_14 = 0.0f;

  matOut->_21 = matIn->_21;   // Copy over 2nd row
  matOut->_22 = matIn->_22; 
  matOut->_23 = matIn->_23; 
  matOut->_24 = 0.0f;

  matOut->_31 = matIn->_41;   // Copy bottom row
  matOut->_32 = matIn->_42;   // used for translation
  matOut->_33 = matIn->_43; 
  matOut->_34 = 0.0f;

  matOut->_41 = 0.0f;  // Clear the bottom row
  matOut->_42 = 0.0f;
  matOut->_43 = 0.0f;
  matOut->_44 = 1.0f;
}
