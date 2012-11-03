#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// A generic coordinate vertex structure
typedef struct {
  D3DXVECTOR3 vecPos;
} sVertex;

// Structure to contain a morphing mesh
typedef struct sMorphMesh {

  // Mesh containers (along w/resulting morphed mesh)
  D3DXMESHCONTAINER_EX *SourceMesh;
  D3DXMESHCONTAINER_EX *TargetMesh;
  D3DXMESHCONTAINER_EX *ResultMesh;

  // Normal offset in vertex structure and vertex pitch
  long                  NormalOffset;
  DWORD                 VertexPitch;

  sMorphMesh() { 
    SourceMesh = NULL;
    TargetMesh = NULL;
    ResultMesh = NULL; 
  }

  ~sMorphMesh() {
    delete SourceMesh; SourceMesh = NULL;
    delete TargetMesh; TargetMesh = NULL;
    delete ResultMesh; ResultMesh = NULL; 
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
char g_szClass[]   = "MorphClass";
char g_szCaption[] = "Morphing Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd);
void DoShutdown();
void DoFrame();

// Function to load a group of meshes for morphing
void LoadMorphingMesh(sMorphMesh *Mesh,
                      char *SourceMesh, 
                      char *TargetMesh,
                      char *TexturePath);

// Function to build a resulting morphed mesh
void BuildMorphMesh(sMorphMesh *Mesh, float Scalar);

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

  // Create and enable a directional light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(Light));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Direction = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

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

  // Delete meshes
  delete g_Water;   g_Water   = NULL;
  delete g_Dolphin; g_Dolphin = NULL;

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static float DolphinXPos = 0.0f, DolphinZPos = 256.0f;

  // Build the water morphing mesh using a time-based sine-wave scalar value
  float WaterTimeFactor = (float)(timeGetTime()) * 0.001f;
  float WaterScalar = ((float)sin(WaterTimeFactor) + 1.0f) * 0.5f;
  BuildMorphMesh(g_Water, WaterScalar);

  // Build the dolphin morphing mesh using a time-based scalar value
  float DolphinTimeFactor = (float)(timeGetTime() % 501) / 250.0f;
  float DolphinScalar = (DolphinTimeFactor<=1.0f)?DolphinTimeFactor:(2.0f-DolphinTimeFactor);
  BuildMorphMesh(g_Dolphin, DolphinScalar);

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

    // Enable lighting to draw water and dolpin
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Draw the water morphing mesh
    DrawMesh(g_Water->ResultMesh);

    // Draw the jumping dolphin
    D3DXMatrixRotationZ(&matWorld, DolphinAngle - 1.57f);
    matWorld._41 = DolphinXPos + (float)cos(DolphinAngle) * 256.0f;
    matWorld._42 = (float)sin(DolphinAngle) * 512.0f;
    matWorld._43 = DolphinZPos;
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_Dolphin->ResultMesh);
  
    // Turn off lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

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
  LoadMesh(&Mesh->SourceMesh, g_pD3DDevice, SourceMesh, TexturePath);
  LoadMesh(&Mesh->TargetMesh, g_pD3DDevice, TargetMesh, TexturePath);

  // Reload the source mesh as a resulting morphed mesh container
  LoadMesh(&Mesh->ResultMesh, g_pD3DDevice, SourceMesh, TexturePath);

  // Determine if source mesh uses normals and calculate offset
  if(Mesh->SourceMesh->MeshData.pMesh->GetFVF() & D3DFVF_NORMAL)
    Mesh->NormalOffset = 3 * sizeof(float);
  else Mesh->NormalOffset = 0;

  // Get vertex buffer pitch
  Mesh->VertexPitch = D3DXGetFVFVertexSize(Mesh->SourceMesh->MeshData.pMesh->GetFVF());
}

void BuildMorphMesh(sMorphMesh *Mesh, float Scalar)
{
  // Lock mesh vertex buffers
  char *SourcePtr, *TargetPtr, *ResultPtr;
  Mesh->SourceMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&SourcePtr);
  Mesh->TargetMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&TargetPtr);
  Mesh->ResultMesh->MeshData.pMesh->LockVertexBuffer(0,  (void**)&ResultPtr);

  // Go through each vertex and interpolate coordinates
  for(DWORD i=0;i<Mesh->SourceMesh->MeshData.pMesh->GetNumVertices();i++) {

    // Get pointers to vertex data
    sVertex *SourceVert = (sVertex*)SourcePtr;
    sVertex *TargetVert = (sVertex*)TargetPtr;
    sVertex *ResultVert = (sVertex*)ResultPtr;

    // Get source coordinates and scale them
    D3DXVECTOR3 vecSource = SourceVert->vecPos;
    vecSource *= (1.0f - Scalar);

    // Get target coordinates and scale them
    D3DXVECTOR3 vecTarget = TargetVert->vecPos;
    vecTarget *= Scalar;

    // Store morphed coordinates
    ResultVert->vecPos = vecSource + vecTarget;

    // Handle interpolation of normals
    if(Mesh->NormalOffset) {
      SourceVert = (sVertex*)&SourcePtr[Mesh->NormalOffset];
      TargetVert = (sVertex*)&TargetPtr[Mesh->NormalOffset];
      ResultVert = (sVertex*)&ResultPtr[Mesh->NormalOffset];

      // Get source coordinates and scale them
      D3DXVECTOR3 vecSource = SourceVert->vecPos;
      vecSource *= (1.0f - Scalar);

      // Get target coordinates and scale them
      D3DXVECTOR3 vecTarget = TargetVert->vecPos;
      vecTarget *= Scalar;

      // Store morphed coordinates
      ResultVert->vecPos = vecSource + vecTarget;
    }

    // Go to next vertex
    SourcePtr += Mesh->VertexPitch;
    TargetPtr += Mesh->VertexPitch;
    ResultPtr += Mesh->VertexPitch;
  }

  // Unlock buffers
  Mesh->SourceMesh->MeshData.pMesh->UnlockVertexBuffer();
  Mesh->TargetMesh->MeshData.pMesh->UnlockVertexBuffer();
  Mesh->ResultMesh->MeshData.pMesh->UnlockVertexBuffer();
}
