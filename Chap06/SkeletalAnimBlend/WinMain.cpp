#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "SkeletalAnim.h"
#include "SkeletalAnimBlend.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Mesh collection and frame hierarchy
D3DXMESHCONTAINER_EX *g_Mesh  = NULL;
D3DXFRAME_EX         *g_Frame = NULL;

// Animation collection object
cBlendedAnimationCollection g_Anim;

// Blending toggles (arms, legs)
char g_BlendFlags[5];

// Guide texture and sprite interface
IDirect3DTexture9 *g_GuideTexture = NULL;
ID3DXSprite       *g_Guide        = NULL;

// Window class and caption text
char g_szClass[]   = "BlendSkeletalAnimClass";
char g_szCaption[] = "Blended Skeletal Animation Demo by Jim Adams";

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
  switch(uMsg) {
    case WM_KEYUP:

      // Toggle a body part
      if(wParam >= '1' && wParam <='5')
        g_BlendFlags[wParam-'1'] ^= 1;

      // Clear toggles
      if(wParam == 32)
        memset(g_BlendFlags, 1, 5);

      break;

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

  // Load a skeletal mesh
  LoadMesh(&g_Mesh, &g_Frame, g_pD3DDevice, "..\\Data\\tiny.x", "..\\Data\\");

  // Load an animation collection
  g_Anim.Load("..\\Data\\tiny.x");

  // Map the animation to the frame hierarchy
  g_Anim.Map(g_Frame);

  // Load the guide texture and create the sprite interface
  D3DXCreateTextureFromFileEx(g_pD3DDevice, "..\\Data\\Guide.bmp", 
                              D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0,
                              D3DFMT_A1R5G5B5, D3DPOOL_DEFAULT, D3DX_DEFAULT, 
                              D3DX_DEFAULT, 0xFF000000,
                              NULL, NULL, &g_GuideTexture);
  D3DXCreateSprite(g_pD3DDevice, &g_Guide);

  // Clear toggles
  memset(g_BlendFlags, 1, 5);

  return TRUE;
}

void DoShutdown()
{
  // Free animation collection data
  g_Anim.Free();

  // Free mesh and frame data
  delete g_Mesh;
  delete g_Frame;

  // Free guide texture and sprite interface
  ReleaseCOM(g_GuideTexture);
  ReleaseCOM(g_Guide);

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD StartTime = timeGetTime();
  DWORD ThisTime = timeGetTime();

  // Clear the frames' transformation matrices
  if(g_Frame)
    g_Frame->Reset();

  // Blend the animations
  if(g_BlendFlags[0])
    g_Anim.Blend("left_arm",  (ThisTime-StartTime), TRUE);
  if(g_BlendFlags[1])
    g_Anim.Blend("right_arm", (ThisTime-StartTime), TRUE);
  if(g_BlendFlags[2])
    g_Anim.Blend("left_leg",  (ThisTime-StartTime), TRUE);
  if(g_BlendFlags[3])
    g_Anim.Blend("right_leg", (ThisTime-StartTime), TRUE);
  if(g_BlendFlags[4])
    g_Anim.Blend("body",      (ThisTime-StartTime), TRUE);

  // Rebuild the frame hierarchy transformations
  if(g_Frame)
    g_Frame->UpdateHierarchy();

  // Build the skinned mesh
  UpdateMesh(g_Mesh);

  // Calculate a view transformation matrix
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(600.0f, 200.0f, -600.0f),
                     &D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Set a world transformation
  D3DXMATRIX matWorld;
  D3DXMatrixIdentity(&matWorld);
  g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0f, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Render skinned mesh
    DrawMesh(g_Mesh);

    // Draw the guide
    g_Guide->Draw(g_GuideTexture, NULL, NULL, NULL, 0.0f, &D3DXVECTOR2(0.0f, 0.0f), 0xFFFFFFFF);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
