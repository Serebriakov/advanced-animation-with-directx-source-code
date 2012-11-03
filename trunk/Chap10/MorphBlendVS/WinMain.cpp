#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Vertex shader declaration and interfaces
D3DVERTEXELEMENT9 g_MorphBlendMeshDecl[] =
{
  // 1st stream is for base mesh
  { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
  { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
  { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
  // 2nd stream is for mesh 1
  { 1,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
  { 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   1 },
  { 1, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
  // 3rd stream is for mesh 2
  { 2,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 2 },
  { 2, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   2 },
  { 2, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
  // 4th stream is for mesh 3
  { 3,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 3 },
  { 3, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   3 },
  { 3, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
  // 5th stream is for mesh 4
  { 4,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 4 },
  { 4, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   4 },
  { 4, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 },
  D3DDECL_END()
};
IDirect3DVertexShader9      *g_VS   = NULL;
IDirect3DVertexDeclaration9 *g_Decl = NULL;

// Load mesh FVF
#define BLENDFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// Base and target morphing mesh objects
D3DXMESHCONTAINER_EX *g_BaseMesh    = NULL;
D3DXMESHCONTAINER_EX *g_Mesh1       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh2       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh3       = NULL;
D3DXMESHCONTAINER_EX *g_Mesh4       = NULL;

// Flags to toggle blending
BOOL g_BlendMesh1 = TRUE;
BOOL g_BlendMesh2 = TRUE;
BOOL g_BlendMesh3 = TRUE;
BOOL g_BlendMesh4 = TRUE;

// Guide texture and sprite interface
IDirect3DTexture9 *g_GuideTexture = NULL;
ID3DXSprite       *g_Guide        = NULL;

// Window class and caption text
char g_szClass[]   = "BlendedVSMorphClass";
char g_szCaption[] = "Blended Vertex Shader Morphing Demo by Jim Adams";

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

   // Load the vertex shader
  LoadVertexShader(&g_VS, g_pD3DDevice, "MorphBlend.vsh", g_MorphBlendMeshDecl, &g_Decl);

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

  // Free guide texture and sprite interface
  ReleaseCOM(g_GuideTexture);
  ReleaseCOM(g_Guide);

  // Free shader interfaces
  ReleaseCOM(g_VS);
  ReleaseCOM(g_Decl);

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
  // Enable zbuffering
  g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  // Get the world, view, and projection matrices
  D3DXMATRIX matWorld, matView, matProj;
  g_pD3DDevice->GetTransform(D3DTS_WORLD, &matWorld);
  g_pD3DDevice->GetTransform(D3DTS_VIEW, &matView);
  g_pD3DDevice->GetTransform(D3DTS_PROJECTION, &matProj);

  // Get the world*view*proj matrix and set it
  D3DXMATRIX matWVP;
  matWVP = matWorld * matView * matProj;
  D3DXMatrixTranspose(&matWVP, &matWVP);
  g_pD3DDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

  // Set the scalar blending values to use
  g_pD3DDevice->SetVertexShaderConstantF(4, (float*)&D3DXVECTOR4(Blend1, Blend2, Blend3, Blend4), 1);

  // Set the light direction
  g_pD3DDevice->SetVertexShaderConstantF(5, (float*)&D3DXVECTOR4(0.5f, -0.5f, 1.0f, 0.0f), 1);

  // Get the meshes' vertex buffers
  IDirect3DVertexBuffer9 *pVB1 = NULL;
  IDirect3DVertexBuffer9 *pVB2 = NULL;
  IDirect3DVertexBuffer9 *pVB3 = NULL;
  IDirect3DVertexBuffer9 *pVB4 = NULL;
  g_Mesh1->MeshData.pMesh->GetVertexBuffer(&pVB1);
  g_Mesh2->MeshData.pMesh->GetVertexBuffer(&pVB2);
  g_Mesh3->MeshData.pMesh->GetVertexBuffer(&pVB3);
  g_Mesh4->MeshData.pMesh->GetVertexBuffer(&pVB4);

  // Set the stream sources
  g_pD3DDevice->SetStreamSource(1, pVB1, 0, D3DXGetFVFVertexSize(g_Mesh1->MeshData.pMesh->GetFVF()));
  g_pD3DDevice->SetStreamSource(2, pVB2, 0, D3DXGetFVFVertexSize(g_Mesh2->MeshData.pMesh->GetFVF()));
  g_pD3DDevice->SetStreamSource(3, pVB3, 0, D3DXGetFVFVertexSize(g_Mesh3->MeshData.pMesh->GetFVF()));
  g_pD3DDevice->SetStreamSource(4, pVB4, 0, D3DXGetFVFVertexSize(g_Mesh4->MeshData.pMesh->GetFVF()));

  // Draw the mesh in the vertex shader
  DrawMesh(g_BaseMesh, g_VS, g_Decl);

  // Clear the stream sources
  g_pD3DDevice->SetStreamSource(1, NULL, 0, 0);
  g_pD3DDevice->SetStreamSource(2, NULL, 0, 0);
  g_pD3DDevice->SetStreamSource(3, NULL, 0, 0);
  g_pD3DDevice->SetStreamSource(4, NULL, 0, 0);
  
  // Free the vertex buffer interface
  ReleaseCOM(pVB1);
  ReleaseCOM(pVB2);
  ReleaseCOM(pVB3);
  ReleaseCOM(pVB4);
}
