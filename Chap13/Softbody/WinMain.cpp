#include <stdio.h>
#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "Cloth.h"
#include "softbody.h"

// Karate master's meshes and softbody object
D3DXMESHCONTAINER_EX *g_Master = NULL;
D3DXMESHCONTAINER_EX *g_Chest  = NULL;
cSoftbodyMesh         g_Softbody;
D3DXVECTOR3           g_vecPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
D3DXVECTOR3           g_vecOrientation = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

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
char g_szClass[]   = "SoftbodyClass";
char g_szCaption[] = "Softbody Demo by Jim Adams";

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

  // Load the master's mesh
  LoadMesh(&g_Master, g_pD3DDevice, "..\\Data\\karatemaster.x", "..\\Data\\");

  // Load the chest mesh
  LoadMesh(&g_Chest, g_pD3DDevice, "..\\Data\\chest.x", "..\\Data\\");

  // Create softbody mesh data
  g_Softbody.Create(g_Chest->MeshData.pMesh, "..\\Data\\chest.x");

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
  D3DXCreateTextureFromFile(g_pD3DDevice, "..\\Data\\dojo.bmp", &g_BackdropTexture);

  // Create and enable a directional light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(Light));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  D3DXVECTOR3 vecLight = D3DXVECTOR3(0.0f, -0.5f, 1.0f);
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

  // Free the softbody mesh
  g_Softbody.Free();

  // Free meshes
  delete g_Chest;
  delete g_Master;

  // Shutdown Direct3D
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD LastTime = timeGetTime()-1;
  DWORD ThisTime = timeGetTime();
  BOOL DontResolve;
  D3DXMATRIX matWorld;
  D3DXVECTOR3 vecGravity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  static DWORD Cycle = 0;
  static BOOL PlayYAHSound = FALSE;  // Flag to play a YAH! sound
  static BOOL PlayHitSound = FALSE;  // Flag to play a hit sound
  
  // Calculate elapsed time
  DWORD Elapsed = ThisTime - LastTime;
  LastTime = ThisTime;

  // The following bit of code is a little sloppy. Basically it 
  // calculates the position and orientation of the meshes based
  // on the amount of time elapsed (kept count in Cycle). The
  // Cycle value is used to keyframe the position and orientations,
  // as well as set a flag to transform the softbody mesh and
  // apply a bit of gravity.

  // Position and orient meshes based on cycled time
  
  // 0 to 999 milliseconds
  if(Cycle < 1000) {

    // Reset the softbody mesh at start of cycle animation
    if(!Cycle)
      g_Softbody.Reset();

    // Stand still - don't move or rotate
    // Make sure softbody mesh transforms with mesh
    g_vecPosition.z = 10.0f;
    g_vecOrientation.y = -1.57f;
    DontResolve = TRUE;

    // Clear flag to play sound coming up
    PlayYAHSound = FALSE;
    PlayHitSound = FALSE;

  } else

  // 1000 to 1499 milliseconds
  if(Cycle < 1500) {
    
    // Play a sound the first time this cycle is processed
    if(PlayYAHSound == FALSE) {
      PlayYAHSound = TRUE;
      PlaySound("..\\Data\\YAH.wav", NULL, SND_FILENAME | SND_ASYNC);
    }

    // Come towards camera - move and rotation
    // Make sure softbody mesh transforms with mesh
    // Apply a small bit of gravity to help softbody animation
    g_vecPosition.z = 10.0f - (((float)Cycle - 1000.0f) / 50.0f);
    g_vecOrientation.y = -1.57f + (float)(Cycle-1000) * 0.00314f;
    DontResolve = TRUE;
    vecGravity = D3DXVECTOR3(0.0f, -0.1f, 0.0f);

  } else

  // 1500 to 2999 milliseconds
  if(Cycle < 3000) {

    // Play a sound the first time this cycle is processed
    if(PlayHitSound == FALSE) {
      PlayHitSound = TRUE;
      PlaySound("..\\Data\\Hit.wav", NULL, SND_FILENAME | SND_ASYNC);
    }
    // Stand still - don't move or rotate
    // Make sure softbody mesh transform on its own
    g_vecPosition.z = 0.0f;
    g_vecOrientation.y = 0.0f;
    DontResolve = FALSE;

  } else

  // 3000 to 3999 milliseconds
  if(Cycle < 4000) {

    // Move back to starting position - move and rotate
    // Make sure softbody mesh transforms with mesh
    g_vecPosition.z = (((float)Cycle - 3000.0f) / 100.0f);
    g_vecOrientation.y = -(float)(Cycle-3000) * 0.00157f;
    DontResolve = TRUE;

  } 

  // Increase cycle time elased
  Cycle += Elapsed;

  // Reset animation cycle if past 4000 ms
  if(Cycle >= 4000)
    Cycle = 0;

  // Calculate a view transformation matrix
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(0.0f, 6.0f, -8.0f),
                     &D3DXVECTOR3(0.0f, 6.0f, 0.0f),
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Set the Softbody's forces for simulation (during last cycle)
  // Only use rotation here and draw it later on using only translation
  D3DXMatrixRotationYawPitchRoll(&matWorld, g_vecOrientation.y, g_vecOrientation.x, g_vecOrientation.z);
  g_Softbody.SetForces(-0.05f, &vecGravity, NULL, &matWorld, DontResolve);

  // Revert the softbody mesh
  g_Softbody.Revert(0.3f, &matWorld);

  // Process the forces
  g_Softbody.ProcessForces((float)Elapsed / 1000.0f * 4.0f);

  // Rebuild the Softbody mesh
  g_Softbody.RebuildMesh(g_Chest->MeshData.pMesh);

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

    // Render the master
    D3DXMatrixRotationYawPitchRoll(&matWorld, g_vecOrientation.y, g_vecOrientation.x, g_vecOrientation.z);
    matWorld._41 = g_vecPosition.x;
    matWorld._42 = g_vecPosition.y;
    matWorld._43 = g_vecPosition.z;
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_Master);

    // Draw the softbody mesh (chest)
    D3DXMatrixTranslation(&matWorld, g_vecPosition.x, g_vecPosition.y, g_vecPosition.z);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_Chest);

    // Turn off lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
