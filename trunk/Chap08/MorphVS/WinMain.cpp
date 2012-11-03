#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

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

// Structure to contain a morphing mesh
typedef struct sMorphMesh {

  // Mesh containers (source and target)
  D3DXMESHCONTAINER_EX *SourceMesh;
  D3DXMESHCONTAINER_EX *TargetMesh;

  sMorphMesh() { 
    SourceMesh = NULL;
    TargetMesh = NULL;
  }

  ~sMorphMesh() {
    delete SourceMesh; SourceMesh = NULL;
    delete TargetMesh; TargetMesh = NULL;
  }
} sMorphMesh;

// Instance morphing meshes (for the water and the dolphin)
sMorphMesh *g_Water   = NULL; 
sMorphMesh *g_Dolphin = NULL;

// Background vertex structure, fvf, vertex buffer, and texture
typedef struct {
  float x, y, z, rhw;
  float u, v;
} sBackdropVertex;
#define BACKDROPFVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
IDirect3DVertexBuffer9 *g_BackdropVB = NULL;
IDirect3DTexture9      *g_BackdropTexture = NULL;

// Window class and caption text
char g_szClass[]   = "VSMorphClass";
char g_szCaption[] = "Vertex Shader Morphing Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd, BOOL Windowed = TRUE);
void DoShutdown();
void DoFrame();

// Function to load a group of meshes for morphing
void LoadMorphingMesh(sMorphMesh *Mesh,
                      char *SourceMesh, 
                      char *TargetMesh,
                      char *TexturePath);

// Function to draw a morphing mesh
void DrawMorphMesh(sMorphMesh *Mesh, float Scalar);

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

  // Load the vertex shader
  LoadVertexShader(&g_VS, g_pD3DDevice, "Morph.vsh", g_MorphMeshDecl, &g_Decl);

  // Load the morphing water meshes
  g_Water = new sMorphMesh();
  LoadMorphingMesh(g_Water,
                   "..\\Data\\Water1.x",
                   "..\\Data\\Water2.x",
                   "..\\Data\\");

  // Load the morphing dolphin meshes
  g_Dolphin = new sMorphMesh();
  LoadMorphingMesh(g_Dolphin, 
                   "..\\Data\\Dolphin1.x",
                   "..\\Data\\Dolphin3.x",
                   "..\\Data\\");

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

  // Start playing an ocean sound
  PlaySound("..\\Data\\Ocean.wav", NULL, SND_ASYNC | SND_LOOP);

  return TRUE;
}

void DoShutdown()
{
  // Stop playing an ocean sound
  PlaySound(NULL, NULL, 0);

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
  static float DolphinXPos = 0.0f, DolphinZPos = 256.0f;

  // Calculate the water morphing scalar value
  float WaterTimeFactor = (float)(timeGetTime()) * 0.001f;
  float WaterScalar = ((float)sin(WaterTimeFactor) + 1.0f) * 0.5f;

  // Calculate the dolphin morphing scalar value
  float DolphinTimeFactor = (float)(timeGetTime() % 501) / 250.0f;
  float DolphinScalar = (DolphinTimeFactor<=1.0f)?DolphinTimeFactor:(2.0f-DolphinTimeFactor);

  // Calculate the angle of the dolphin's movement and
  // reposition the dolphin if it's far enough underwater
  float DolphinAngle = (float)(timeGetTime() % 6280) / 1000.0f * 3.0f;
  if(sin(DolphinAngle) < -0.7f) {
    DolphinXPos = (float)(rand()%1400) - 700.0f;
    DolphinZPos = (float)(rand()%1500);
  }

  // Create and set the view transformation
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(0.0f, 170.0f, -1000.0f), 
                               &D3DXVECTOR3(0.0f, 150.0f, 0.0f), 
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

    // Set identity matrix for world transformation
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Draw the water morphing mesh
    DrawMorphMesh(g_Water, WaterScalar);

    // Draw the jumping dolphin
    D3DXMatrixRotationZ(&matWorld, DolphinAngle - 1.57f);
    matWorld._41 = DolphinXPos + (float)cos(DolphinAngle) * 256.0f;
    matWorld._42 = (float)sin(DolphinAngle) * 512.0f;
    matWorld._43 = DolphinZPos;
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMorphMesh(g_Dolphin, DolphinScalar);
  
    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void LoadMorphingMesh(sMorphMesh *Mesh,
                      char *SourceMesh, 
                      char *TargetMesh,
                      char *TexturePath)
{
  // Load the source and target meshes
  LoadMesh(&Mesh->SourceMesh, g_pD3DDevice, SourceMesh, TexturePath, MORPHFVF);
  LoadMesh(&Mesh->TargetMesh, g_pD3DDevice, TargetMesh, TexturePath, MORPHFVF);
}

void DrawMorphMesh(sMorphMesh *Mesh, float Scalar)
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

  // Set the light direction
  g_pD3DDevice->SetVertexShaderConstantF(5, (float*)&D3DXVECTOR4(0.0f, -1.0f, 0.0f, 0.0f), 1);

  // Set the 2nd stream source
  IDirect3DVertexBuffer9 *pVB = NULL;
  Mesh->TargetMesh->MeshData.pMesh->GetVertexBuffer(&pVB);
  g_pD3DDevice->SetStreamSource(1, pVB, 0, D3DXGetFVFVertexSize(Mesh->TargetMesh->MeshData.pMesh->GetFVF()));

  // Draw the mesh in the vertex shader
  DrawMesh(Mesh->SourceMesh, g_VS, g_Decl);

  // Clear the 2nd stream source and free the vertex buffer interface
  g_pD3DDevice->SetStreamSource(1, NULL, 0, 0);
  ReleaseCOM(pVB);
}
