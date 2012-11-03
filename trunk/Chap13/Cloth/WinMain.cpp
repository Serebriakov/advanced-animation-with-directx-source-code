#include <stdio.h>
#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "Cloth.h"

// Hero's body and cape (plus cloth object)
D3DXMESHCONTAINER_EX *g_Hero = NULL;
D3DXMESHCONTAINER_EX *g_Cape = NULL;
cClothMesh            g_Cloth;

// Collision objects and transformation matrix
cCollision g_Collisions;
D3DXMATRIX g_matCollision;

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Background vertex structure, fvf, vertex buffer, and texture
typedef struct {
  float x, y, z, rhw;
  float u, v;
} sBackdropVertex;
#define BACKDROPFVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
IDirect3DVertexBuffer9 *g_BackdropVB = NULL;
IDirect3DTexture9      *g_BackdropTexture = NULL;

// Window class and caption text
char g_szClass[]   = "ClothClass";
char g_szCaption[] = "Cloth Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd);
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
  wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
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

BOOL DoInit(HWND hWnd)
{
  // Initialize Direct3D
  InitD3D(&g_pD3D, &g_pD3DDevice, hWnd);

  // Load the hero mesh
  LoadMesh(&g_Hero, g_pD3DDevice, "..\\Data\\hero.x", "..\\Data\\");

  // Load the cape mesh
  LoadMesh(&g_Cape, g_pD3DDevice, "..\\Data\\cape.x", "..\\Data\\");

  // Create cloth mesh data
  g_Cloth.Create(g_Cape->MeshData.pMesh, "..\\Data\\cape.x");

  // Add a collision object
  g_Collisions.AddPlane(&D3DXPLANE(0.0f, 1.0f, 0.0f, 0.0f));

  // Clear collision object transformation
  D3DXMatrixIdentity(&g_matCollision);

  // Create the backdrop
  sBackdropVertex BackdropVerts[4] = {
    {   0.0f,   0.0, 1.0, 1.0f, 0.0f, 0.0f },
    { 640.0f,   0.0, 1.0, 1.0f, 1.0f, 0.0f },
    {   0.0f, 480.0, 1.0, 1.0f, 0.0f, 1.0f },
    { 640.0f, 480.0, 1.0, 1.0f, 1.0f, 1.0f }
  };
  g_pD3DDevice->CreateVertexBuffer(sizeof(BackdropVerts), D3DUSAGE_WRITEONLY, BACKDROPFVF, D3DPOOL_DEFAULT, &g_BackdropVB, NULL);
  char *Ptr;
  g_BackdropVB->Lock(0,0, (void**)&Ptr, 0);
  memcpy(Ptr, BackdropVerts, sizeof(BackdropVerts));
  g_BackdropVB->Unlock();
  D3DXCreateTextureFromFile(g_pD3DDevice, "..\\Data\\Sky.bmp", &g_BackdropTexture);

  // Create and enable a directional light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(Light));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  D3DXVECTOR3 vecLight = D3DXVECTOR3(-1.0f, 0.1f, 1.0f);
  D3DXVec3Normalize(&vecLight, &vecLight);
  Light.Direction = vecLight;
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  return TRUE;
}

void DoShutdown()
{
  // Release Backdrop data
  ReleaseCOM(g_BackdropVB);
  ReleaseCOM(g_BackdropTexture);

  // Free collisions
  g_Collisions.Free();

  // Free the cloth mesh
  g_Cloth.Free();

  // Free meshes
  delete g_Cape;
  delete g_Hero;

  // Shutdown Direct3D
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD LastTime = timeGetTime()-1;
  DWORD ThisTime = timeGetTime();
  static BOOL First = TRUE;

  // Calculate elapsed time
  DWORD Elapsed = ThisTime - LastTime;
  LastTime = ThisTime;

  // Calculate a view transformation matrix
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(0.0f, 200.0f, -3000.0f),
                     &D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Compute an angle to move the hero and cape
  float Angle = (float)timeGetTime() / 1000.0f;

  // Position the cloth
  D3DXMATRIX matWorld;
  D3DXMatrixRotationY(&matWorld, -Angle + 3.14f);
  matWorld._41 = (float)cos(Angle) * 1024.0f;
  matWorld._42 = 1.0f;
  matWorld._43 = (float)sin(Angle) * 1024.0f;

  // Set gravity and wind forces
  D3DXVECTOR3 vecGravity = D3DXVECTOR3(0.0f, -9.8f, 0.0f);
  D3DXVECTOR3 vecWind = D3DXVECTOR3(0.0f, 20.0f, 0.0f);

  // Set the cloth's forces for simulation
  g_Cloth.SetForces(-0.05f, &vecGravity, &vecWind, &matWorld, First);
  First = FALSE;

  // Process the forces
  g_Cloth.ProcessForces((float)Elapsed / 1000.0f * 4.0f);

  // Process the collisions
  g_Cloth.ProcessCollisions(&g_Collisions, &g_matCollision);

  // Rebuild the cloth mesh
  g_Cloth.RebuildMesh(g_Cape->MeshData.pMesh);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Draw the backdrop
    g_pD3DDevice->SetFVF(BACKDROPFVF);
    g_pD3DDevice->SetStreamSource(0, g_BackdropVB, 0, sizeof(sBackdropVertex));
    g_pD3DDevice->SetTexture(0, g_BackdropTexture);
    g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // Turn on lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Render the hero
    D3DXMatrixRotationY(&matWorld, -Angle + 3.14f);
    matWorld._41 = (float)cos(Angle) * 1024.0f;
    matWorld._42 = 0.0f;
    matWorld._43 = (float)sin(Angle) * 1024.0f;
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_Hero);

    // Set two sided rendering
    g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Render the cloth mesh
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_Cape);

    // Restore culling
    g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

    // Turn off lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
