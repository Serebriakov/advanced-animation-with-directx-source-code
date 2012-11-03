#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "Ragdoll.h"

// Ragdoll animation object
cRagdoll g_Ragdoll;

// Collision object collection 
cCollision g_Collision;

// Structure to contain sphere collision object data
typedef struct {
  D3DXVECTOR3 vecSphere;
  float       Scale;
} sCollisionSphere;
sCollisionSphere *g_SphereObjects = NULL;

typedef struct {
  D3DXVECTOR3 vecPos;
  DWORD Color;
  float       u, v;
} sVertex;
#define FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Floor's vertex buffer and texture
IDirect3DVertexBuffer9 *g_FloorVB = NULL;
IDirect3DTexture9      *g_FloorTexture = NULL;

// Mesh collection and frame hierarchy
D3DXMESHCONTAINER_EX *g_Mesh  = NULL;
D3DXFRAME_EX         *g_Frame = NULL;

// A ball mesh used to draw sphere collision objects
D3DXMESHCONTAINER_EX *g_Sphere = NULL;

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Window class and caption text
char g_szClass[]   = "RagdollClass";
char g_szCaption[] = "Ragdoll Animation Demo by Jim Adams";

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

  // Load a skeletal mesh
  LoadMesh(&g_Mesh, &g_Frame, g_pD3DDevice, "..\\Data\\ragdoll.x", "..\\Data\\");

  // Create the ragdoll object to work with
  D3DXMATRIX matPosition;
  D3DXMatrixTranslation(&matPosition, 0.0f, 20.0f, 0.0f);
  g_Ragdoll.Create(g_Frame, g_Mesh, &matPosition);

  // Add the floor's plane
  g_Collision.AddPlane(&D3DXPLANE(0.0f, 1.0f, 0.0f, 0.0f));

  // Create the floor object
  g_pD3DDevice->CreateVertexBuffer(sizeof(sVertex)*4, D3DUSAGE_WRITEONLY, FVF, D3DPOOL_DEFAULT, &g_FloorVB, NULL);
  sVertex *VBPtr;
  g_FloorVB->Lock(0, 0, (void**)&VBPtr, 0);
  VBPtr[0].vecPos = D3DXVECTOR3(-2000.0f, 0.0f,  2000.0f);
  VBPtr[0].u = 0.0f; VBPtr[0].v = 0.0f; VBPtr[0].Color = 0xFFFFFFFF;
  VBPtr[1].vecPos = D3DXVECTOR3( 2000.0f, 0.0f,  2000.0f);
  VBPtr[1].u = 4.0f; VBPtr[1].v = 0.0f; VBPtr[1].Color = 0xFFFFFFFF;
  VBPtr[2].vecPos = D3DXVECTOR3(-2000.0f, 0.0f, -2000.0f);
  VBPtr[2].u = 0.0f; VBPtr[2].v = 4.0f; VBPtr[2].Color = 0xFFFFFFFF;
  VBPtr[3].vecPos = D3DXVECTOR3( 2000.0f, 0.0f, -2000.0f);
  VBPtr[3].u = 4.0f; VBPtr[3].v = 4.0f; VBPtr[3].Color = 0xFFFFFFFF;
  g_FloorVB->Unlock();

  // Load the floor texture
  D3DXCreateTextureFromFile(g_pD3DDevice, "..\\Data\\Floor.bmp", &g_FloorTexture);

  // Load the sphere mesh
  LoadMesh(&g_Sphere, g_pD3DDevice, "..\\Data\\Sphere.x", "..\\Data\\");

  // Create some sphere collision objects
  srand(timeGetTime());
  g_SphereObjects = new sCollisionSphere[15];
  for(DWORD i=0;i<15;i++) {
    g_SphereObjects[i].vecSphere = D3DXVECTOR3((float)(rand()%20)-10.0f,
                                               (float)(rand()%20),
                                               (float)(rand()%20)-10.0f);
    g_SphereObjects[i].Scale = (float)(rand() % 5) + 1.0f;

    g_Collision.AddSphere(&g_SphereObjects[i].vecSphere, g_SphereObjects[i].Scale);
  }

  return TRUE;
}

void DoShutdown()
{
  // Free sphere
  delete g_Sphere; g_Sphere = NULL;

  // Free floor and texture
  ReleaseCOM(g_FloorTexture);
  ReleaseCOM(g_FloorVB);

  // Free the ragdoll and collision data
  g_Ragdoll.Free();
  g_Collision.Free();

  // Free mesh and frame data
  delete g_Mesh;  g_Mesh  = NULL;
  delete g_Frame; g_Frame = NULL;

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD LastTime = timeGetTime() - 1;
  DWORD ThisTime = timeGetTime();

  // Calculate the elapsed time
  DWORD Elapsed = ThisTime - LastTime;
  LastTime = ThisTime;

  // Resolve the motion for this time-period
  g_Ragdoll.Resolve((float)Elapsed / 1000.0f,
                    -0.5f, -0.4f,
                    &D3DXVECTOR3(0.0f, -9.8f, 0.0f), 
                    &g_Collision);

  // Rebuild the ragdoll's frame hierarchy
  g_Ragdoll.RebuildHierarchy();

  // Build the skinned mesh
  UpdateMesh(g_Mesh);

  // Calculate a view transformation matrix
  D3DXVECTOR3 vecPos = g_Ragdoll.GetBone(0)->m_State.m_vecPosition;
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView, 
                     &D3DXVECTOR3(vecPos.x, vecPos.y+10.0f, vecPos.z-20.0f),
                     &vecPos,
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Set a world transformation
  D3DXMATRIX matWorld;
  D3DXMatrixIdentity(&matWorld);
  g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0f, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Render the floor
    g_pD3DDevice->SetTexture(0, g_FloorTexture);
    g_pD3DDevice->SetStreamSource(0, g_FloorVB, 0, sizeof(sVertex));
    g_pD3DDevice->SetFVF(FVF);
    g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // Render the spheres
    for(DWORD i=0;i<15;i++) {
      D3DXMatrixScaling(&matWorld, g_SphereObjects[i].Scale,
                                   g_SphereObjects[i].Scale,
                                   g_SphereObjects[i].Scale);
      matWorld._41 = g_SphereObjects[i].vecSphere.x;
      matWorld._42 = g_SphereObjects[i].vecSphere.y;
      matWorld._43 = g_SphereObjects[i].vecSphere.z;
      g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
      DrawMesh(g_Sphere);
    }

    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Render skinned mesh
    DrawMesh(g_Mesh);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
