#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "SkeletalAnim.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Mesh collection and frame hierarchy
D3DXMESHCONTAINER_EX *g_Mesh = NULL;
D3DXFRAME_EX         *g_Frame = NULL;

// Animation collection object
cAnimationCollection g_Anim;

// Bounding radius of mesh
float g_MeshRadius = 0.0f;

// Window class and caption text
char g_szClass[]   = "SkeletalAnimClass";
char g_szCaption[] = "Skeletal Animation Demo by Jim Adams";

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
  LoadMesh(&g_Mesh, &g_Frame, g_pD3DDevice, "..\\Data\\tiny.x", "..\\Data\\");

  // Load an animation collection
  g_Anim.Load("..\\Data\\tiny.x");

  // Map the animation to the frame hierarchy
  g_Anim.Map(g_Frame);

  // Get the bounding radius of the object
  g_MeshRadius = 0.0f;
  D3DXMESHCONTAINER_EX *pMesh = g_Mesh;
  while(pMesh) {

    // Lock the vertex buffer, get its radius, and unlock buffer
    if(pMesh->MeshData.pMesh) {
      D3DXVECTOR3 *pVertices, vecCenter;
      float Radius;
      pMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pVertices);
      D3DXComputeBoundingSphere(pVertices, 
                                pMesh->MeshData.pMesh->GetNumVertices(),
                                D3DXGetFVFVertexSize(pMesh->MeshData.pMesh->GetFVF()),
                                &vecCenter, &Radius);
      pMesh->MeshData.pMesh->UnlockVertexBuffer();

      // Update radius
      if(Radius > g_MeshRadius)
        g_MeshRadius = Radius;
    }

    // Go to next mesh
    pMesh = (D3DXMESHCONTAINER_EX*)pMesh->pNextMeshContainer;
  }

  return TRUE;
}

void DoShutdown()
{
  // Free animation collection data
  g_Anim.Free();

  // Free mesh and frame data
  delete g_Mesh;
  delete g_Frame;

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD StartTime = timeGetTime();
  DWORD ThisTime = timeGetTime();

  // Update the animation (convert to 30 fps)
  g_Anim.Update(NULL, (ThisTime-StartTime)*3, TRUE);

  // Rebuild the frame hierarchy transformations
  if(g_Frame)
    g_Frame->UpdateHierarchy();

  // Build the skinned mesh
  UpdateMesh(g_Mesh);

  // Calculate a view transformation matrix
  // Using the mesh's bounding radius to position the viewer
  float Distance = g_MeshRadius * 3.0f;
  float Angle = (float)timeGetTime() / 2000.0f;
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3((float)cos(Angle) * Distance, g_MeshRadius, (float)sin(Angle) * Distance),
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

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
