#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Base, target morphing, and blended mesh objects
D3DXMESHCONTAINER_EX *g_BaseMesh    = NULL;
D3DXMESHCONTAINER_EX *g_Mesh1       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh2       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh3       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh4       = NULL;
D3DXMESHCONTAINER_EX *g_BlendedMesh = NULL;

// Flags to toggle blending
BOOL g_BlendMesh1 = TRUE;
BOOL g_BlendMesh2 = TRUE;
BOOL g_BlendMesh3 = TRUE;
BOOL g_BlendMesh4 = TRUE;

// Guide texture and sprite interface
IDirect3DTexture9 *g_GuideTexture = NULL;
ID3DXSprite       *g_Guide        = NULL;

// Morphing mesh vertex structure and FVF
typedef struct {
  D3DXVECTOR3 vecPos;
  D3DXVECTOR3 vecNormal;
  float       u, v;
} sVertex;
#define BLENDFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// Window class and caption text
char g_szClass[]   = "BlendedMorphingClass";
char g_szCaption[] = "Blended Morphing Demo by Jim Adams";

// Function prototypes ////////////////////////////////////
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd);
void DoShutdown();
void DoFrame();

void DrawBlendedMesh(D3DXMESHCONTAINER_EX *BaseMesh,
                     D3DXMESHCONTAINER_EX *Mesh1, float Blend1,
                     D3DXMESHCONTAINER_EX *Mesh2, float Blend2,
                     D3DXMESHCONTAINER_EX *Mesh3, float Blend3,
                     D3DXMESHCONTAINER_EX *Mesh4, float Blend4);

///////////////////////////////////////////////////////////
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
      switch(wParam) {
        case '1':
          g_BlendMesh1 = (g_BlendMesh1==TRUE)?FALSE:TRUE;
          break;

        case '2':
          g_BlendMesh2 = (g_BlendMesh2==TRUE)?FALSE:TRUE;
          break;

        case '3':
          g_BlendMesh3 = (g_BlendMesh3==TRUE)?FALSE:TRUE;
          break;

        case '4':
          g_BlendMesh4 = (g_BlendMesh4==TRUE)?FALSE:TRUE;
          break;

        case ' ':
          g_BlendMesh1 = g_BlendMesh2 = g_BlendMesh3 = g_BlendMesh4 = TRUE;
          break;
      }
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

  // Load the base and target meshes using the blended morphing FVF
  LoadMesh(&g_BaseMesh, g_pD3DDevice, "..\\Data\\Base.x",  "..\\Data\\", BLENDFVF);
  LoadMesh(&g_Mesh1,    g_pD3DDevice, "..\\Data\\Mesh1.x", "..\\Data\\", BLENDFVF);
  LoadMesh(&g_Mesh2,    g_pD3DDevice, "..\\Data\\Mesh2.x", "..\\Data\\", BLENDFVF);
  LoadMesh(&g_Mesh3,    g_pD3DDevice, "..\\Data\\Mesh3.x", "..\\Data\\", BLENDFVF);
  LoadMesh(&g_Mesh4,    g_pD3DDevice, "..\\Data\\Mesh4.x", "..\\Data\\", BLENDFVF);

  // Reload the base mesh to use as blended mesh
  LoadMesh(&g_BlendedMesh, g_pD3DDevice, "..\\Data\\Base.x",  "..\\Data\\", BLENDFVF);

  // Load the guide texture and create the sprite interface
  D3DXCreateTextureFromFileEx(g_pD3DDevice, "..\\Data\\Guide.bmp", 
                              D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0,
                              D3DFMT_A1R5G5B5, D3DPOOL_DEFAULT, D3DX_DEFAULT, 
                              D3DX_DEFAULT, 0xFF000000,
                              NULL, NULL, &g_GuideTexture);
  D3DXCreateSprite(g_pD3DDevice, &g_Guide);

  return TRUE;
}

void DoShutdown()
{
  // Free all meshes
  delete g_BaseMesh;
  delete g_Mesh1;
  delete g_Mesh2;
  delete g_Mesh3;
  delete g_Mesh4;
  delete g_BlendedMesh;

  // Free guide texture and sprite interface
  ReleaseCOM(g_GuideTexture);
  ReleaseCOM(g_Guide);

  // Shutdown Direct3D
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Set a view transformation
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3(-5.0f, 4.0f, -12.0f),
                       &D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                       &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

    // Set a world transformation
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Calculate the blending scalar values based on time
    DWORD Time = timeGetTime();
    float Mesh1Blend = (float)(Time %  501) /  250.0f;
    float Mesh2Blend = (float)(Time % 1001) /  500.0f;
    float Mesh3Blend = (float)(Time % 2001) / 1000.0f;
    float Mesh4Blend = (float)(Time % 3001) / 1500.0f;

    // Apply toggles to meshes
    if(g_BlendMesh1 == FALSE)
      Mesh1Blend = 0.0f;
    if(g_BlendMesh2 == FALSE)
      Mesh2Blend = 0.0f;
    if(g_BlendMesh3 == FALSE)
      Mesh3Blend = 0.0f;
    if(g_BlendMesh4 == FALSE)
      Mesh4Blend = 0.0f;

    // Draw the blended mesh
    DrawBlendedMesh(g_BaseMesh,
                    g_Mesh1, ((Mesh1Blend<=1.0f)?Mesh1Blend:(2.0f-Mesh1Blend)),
                    g_Mesh2, ((Mesh2Blend<=1.0f)?Mesh2Blend:(2.0f-Mesh2Blend)),
                    g_Mesh3, ((Mesh3Blend<=1.0f)?Mesh3Blend:(2.0f-Mesh3Blend)),
                    g_Mesh4, ((Mesh4Blend<=1.0f)?Mesh4Blend:(2.0f-Mesh4Blend)));

    // Draw the guide
    g_Guide->Draw(g_GuideTexture, NULL, NULL, NULL, 0.0f, &D3DXVECTOR2(0.0f, 0.0f), 0xFFFFFFFF);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void DrawBlendedMesh(D3DXMESHCONTAINER_EX *BaseMesh,
                     D3DXMESHCONTAINER_EX *Mesh1, float Blend1,
                     D3DXMESHCONTAINER_EX *Mesh2, float Blend2,
                     D3DXMESHCONTAINER_EX *Mesh3, float Blend3,
                     D3DXMESHCONTAINER_EX *Mesh4, float Blend4)
{
  // Lock all meshes and get pointers
  sVertex *pBaseVertices,  *pBlendedVertices;
  sVertex *pMesh1Vertices, *pMesh2Vertices;
  sVertex *pMesh3Vertices, *pMesh4Vertices;
  
  BaseMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pBaseVertices);
  g_Mesh1->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pMesh1Vertices);
  g_Mesh2->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pMesh2Vertices);
  g_Mesh3->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pMesh3Vertices);
  g_Mesh4->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pMesh4Vertices);
  g_BlendedMesh->MeshData.pMesh->LockVertexBuffer(0, (void**)&pBlendedVertices);

  // Iterate all vertices
  for(DWORD i=0;i<g_BaseMesh->MeshData.pMesh->GetNumVertices();i++) {

    // Get the difference in vertex coordinates 
    D3DXVECTOR3 vecXYZDiff1 = pMesh1Vertices->vecPos - pBaseVertices->vecPos;
    D3DXVECTOR3 vecXYZDiff2 = pMesh2Vertices->vecPos - pBaseVertices->vecPos;
    D3DXVECTOR3 vecXYZDiff3 = pMesh3Vertices->vecPos - pBaseVertices->vecPos;
    D3DXVECTOR3 vecXYZDiff4 = pMesh4Vertices->vecPos - pBaseVertices->vecPos;

    // Get the difference in normals
    D3DXVECTOR3 vecNormDiff1 = pMesh1Vertices->vecNormal - pBaseVertices->vecNormal;
    D3DXVECTOR3 vecNormDiff2 = pMesh2Vertices->vecNormal - pBaseVertices->vecNormal;
    D3DXVECTOR3 vecNormDiff3 = pMesh3Vertices->vecNormal - pBaseVertices->vecNormal;
    D3DXVECTOR3 vecNormDiff4 = pMesh4Vertices->vecNormal - pBaseVertices->vecNormal;

    // Apply blending values
    vecXYZDiff1 *= Blend1; vecNormDiff1 *= Blend1;
    vecXYZDiff2 *= Blend2; vecNormDiff2 *= Blend2;
    vecXYZDiff3 *= Blend3; vecNormDiff3 *= Blend3;
    vecXYZDiff4 *= Blend4; vecNormDiff4 *= Blend4;

    // Get tallied blended difference values
    D3DXVECTOR3 vecXYZBlended = vecXYZDiff1 + vecXYZDiff2 + vecXYZDiff3 + vecXYZDiff4;
    D3DXVECTOR3 vecNormBlended = vecNormDiff1 + vecNormDiff2 + vecNormDiff3 + vecNormDiff4;

    // Add the differences back to base mesh values and store
    pBlendedVertices->vecPos = vecXYZBlended + pBaseVertices->vecPos;

    // Add the differences in normals to based mesh values,
    // normalize the values, and store in resulting mesh!
    vecNormBlended += pBaseVertices->vecNormal;
    D3DXVec3Normalize(&pBlendedVertices->vecNormal, &vecNormBlended);

    // Go to next vertices
    pBaseVertices++;  pBlendedVertices++;
    pMesh1Vertices++; pMesh2Vertices++;
    pMesh3Vertices++; pMesh4Vertices++;
  }  // Next loop iteration

  // Unlock the vertex buffers
  g_BlendedMesh->MeshData.pMesh->UnlockVertexBuffer();
  g_Mesh4->MeshData.pMesh->UnlockVertexBuffer();
  g_Mesh3->MeshData.pMesh->UnlockVertexBuffer();
  g_Mesh2->MeshData.pMesh->UnlockVertexBuffer();
  g_Mesh1->MeshData.pMesh->UnlockVertexBuffer();
  g_BaseMesh->MeshData.pMesh->UnlockVertexBuffer();

  // Set a light for the scene 
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(D3DLIGHT9));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Direction.x = 0.0f;
  Light.Direction.y = 0.0f;
  Light.Direction.z = 1.0f;
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  // Enable lighting and zbuffering
  g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
  g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  // Render the blended mesh
  DrawMesh(g_BlendedMesh);
}
