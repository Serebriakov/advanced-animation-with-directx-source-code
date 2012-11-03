#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "MorphAnim.h"

// Vertex shader declaration and interfaces
D3DVERTEXELEMENT9 g_MorphMeshDecl[] =
{
  // 1st stream is for source mesh
  { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
  { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
  { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

  // 2nd stream is for target mesh
  { 1,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
  { 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   1 },
  { 1, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
  D3DDECL_END()
};
IDirect3DVertexShader9      *g_VS   = NULL;
IDirect3DVertexDeclaration9 *g_Decl = NULL;

// Declare a new FVF for morphing meshes
#define MORPHFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Mesh container for morphing meshes
D3DXMESHCONTAINER_EX *g_MorphMeshes = NULL;

// Key-framed morphing animation collection
cMorphAnimationCollection g_MorphAnim;

// Mesh container for base
D3DXMESHCONTAINER_EX *g_BaseMesh = NULL;

// Background vertex structure, fvf, vertex buffer, and texture
typedef struct {
  float x, y, z, rhw;
  float u, v;
} sBackdropVertex;
#define BACKDROPFVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
IDirect3DVertexBuffer9 *g_BackdropVB = NULL;
IDirect3DTexture9      *g_BackdropTexture = NULL;

// Window class and caption text
char g_szClass[]   = "MorphAnimClass";
char g_szCaption[] = "Key-framed Morphing Animation Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd);
void DoShutdown();
void DoFrame();

void DrawMorphMesh(D3DXMESHCONTAINER_EX *SourceMesh,
                   D3DXMESHCONTAINER_EX *TargetMesh,
                   float Scalar);

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

  // Load the base mesh
  LoadMesh(&g_BaseMesh, g_pD3DDevice, "..\\Data\\Base.x", "..\\Data\\");

  // Load the collection of meshes used for demo
  LoadMesh(&g_MorphMeshes, NULL, g_pD3DDevice, "..\\Data\\Dummy.x", "..\\Data\\", MORPHFVF);

  // Load the morphing animation set and map animations to meshes
  g_MorphAnim.Load("..\\Data\\dummy.x");
  g_MorphAnim.Map(g_MorphMeshes);

  // Load the vertex shader
  LoadVertexShader(&g_VS, g_pD3DDevice, "Morph.vsh", g_MorphMeshDecl, &g_Decl);

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
  D3DXCreateTextureFromFile(g_pD3DDevice, "..\\Data\\Backdrop.bmp", &g_BackdropTexture);

  // Play the song
  PlaySound("..\\Data\\dance.wav", NULL,  SND_ASYNC | SND_LOOP);

  return TRUE;
}

void DoShutdown()
{
  // Stop the sound
  PlaySound(NULL, NULL, 0);

  // Free animation data
  g_MorphAnim.Free();

  // Free meshes
  delete g_BaseMesh;    g_BaseMesh    = NULL;
  delete g_MorphMeshes; g_MorphMeshes = NULL;

  // Release Backdrop data
  ReleaseCOM(g_BackdropVB);
  ReleaseCOM(g_BackdropTexture);

  // Free shader interfaces
  ReleaseCOM(g_VS);
  ReleaseCOM(g_Decl);

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD StartTime = timeGetTime();
  DWORD Time = timeGetTime();

  // Create and set the view transformation
  float Angle = (float)timeGetTime() / 1000.0f;
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(1100.0f, 600.0f, -1200.0f),
                               &D3DXVECTOR3(0.0f, 400.0f, 0.0f), 
                               &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Draw the backdrop
    g_pD3DDevice->SetFVF(BACKDROPFVF);
    g_pD3DDevice->SetStreamSource(0, g_BackdropVB, 0, sizeof(sBackdropVertex));
    g_pD3DDevice->SetTexture(0, g_BackdropTexture);
    g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // Set world transformation (rotate based on time)
    D3DXMATRIX matWorld;
    D3DXMatrixRotationY(&matWorld, (float)timeGetTime() / 1000.0f);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Draw base
    DrawMesh(g_BaseMesh);

    // Get the pointers to the meshes to use for morphing
    D3DXMESHCONTAINER_EX *pSourceMesh, *pTargetMesh;
    float Scalar;
    g_MorphAnim.Update("Dance", (Time-StartTime), TRUE, 
                       &pSourceMesh, &pTargetMesh, &Scalar);

    // Draw the morphing mesh using the time as a scalar (and rotate it)
    DrawMorphMesh(pSourceMesh, pTargetMesh, Scalar);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void DrawMorphMesh(D3DXMESHCONTAINER_EX *SourceMesh,
                   D3DXMESHCONTAINER_EX *TargetMesh,
                   float Scalar)
{
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

  // Set the scalar value to use
  g_pD3DDevice->SetVertexShaderConstantF(4, (float*)&D3DXVECTOR4(1.0f-Scalar, Scalar, 0.0f, 0.0f), 1);

  // Set the light direction (convert from model space to view space)
  D3DXMATRIX matInvWorld;
  D3DXMatrixInverse(&matInvWorld, NULL, &matWorld);
  D3DXVECTOR3 vecLight = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
  D3DXVec3TransformCoord(&vecLight, &vecLight, &matInvWorld);
  g_pD3DDevice->SetVertexShaderConstantF(5, (float*)&D3DXVECTOR4(vecLight.x, vecLight.y, vecLight.z, 0.0f), 1);

  // Set the 2nd stream source
  IDirect3DVertexBuffer9 *pVB = NULL;
  TargetMesh->MeshData.pMesh->GetVertexBuffer(&pVB);
  g_pD3DDevice->SetStreamSource(1, pVB, 0, D3DXGetFVFVertexSize(TargetMesh->MeshData.pMesh->GetFVF()));

  // Draw the mesh in the vertex shader
  DrawMesh(SourceMesh, g_VS, g_Decl);

  // Clear the 2nd stream source and free the vertex buffer interface
  g_pD3DDevice->SetStreamSource(1, NULL, 0, 0);
  ReleaseCOM(pVB);
}
