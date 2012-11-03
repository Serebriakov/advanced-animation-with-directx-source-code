#include <stdio.h>
#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "dshow.h"
#include "Direct3D.h"
#include "Face.h"

// Window class and caption text
char g_szClass[]   = "FacialAnimationClass";
char g_szCaption[] = "Facial Animation Demo by Jim Adams";

// DirectShow interfaces used for sound playback
IGraphBuilder  *g_pGraph    = NULL;
IMediaControl  *g_pControl  = NULL;
IMediaEvent    *g_pEvent    = NULL;
IMediaPosition *g_pPosition = NULL;

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Phoneme loader
cXPhonemeParser g_PhonemeParser;

// Facial mesh list object
D3DXMESHCONTAINER_EX *g_FaceMeshes = NULL;

// Body mesh
D3DXMESHCONTAINER_EX *g_BodyMesh = NULL;

// Background vertex structure, fvf, vertex buffer, and texture
typedef struct {
  float x, y, z, rhw;
  float u, v;
} sBackdropVertex;
#define BACKDROPFVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
IDirect3DVertexBuffer9 *g_BackdropVB = NULL;
IDirect3DTexture9      *g_BackdropTexture = NULL;

// Phoneme/facial ID's
enum FaceBlend {
  FACE_NEUTRAL = 0,
  FACE_BLINK,
  FACE_CONSONANT,
  FACE_AA,
  FACE_EE,
  FACE_II,
  FACE_OO,
  FACE_UU,
  FACE_FF,
  FACE_SMILE
};

// Phoneme conversions from standard's set to demo's set
DWORD g_PhonemeSet[] = { 
  FACE_NEUTRAL,   // Silence/unknown
  FACE_AA,        // AA
  FACE_AA,        // AE
  FACE_AA,        // AH
  FACE_AA,        // AO
  FACE_AA,        // AX
  FACE_FF,        // B
  FACE_CONSONANT, // CH
  FACE_II,        // D
  FACE_CONSONANT, // DH
  FACE_EE,        // EH
  FACE_CONSONANT, // ER
  FACE_EE,        // EY
  FACE_FF,        // F
  FACE_CONSONANT, // G
  FACE_CONSONANT, // HH
  FACE_II,        // IH
  FACE_II,        // IY
  FACE_CONSONANT, // JH
  FACE_CONSONANT, // K
  FACE_CONSONANT, // L
  FACE_CONSONANT, // M
  FACE_CONSONANT, // N
  FACE_CONSONANT, // NX
  FACE_OO,        // OW
  FACE_CONSONANT, // P
  FACE_CONSONANT, // R
  FACE_II,        // S
  FACE_CONSONANT, // SH
  FACE_CONSONANT, // T
  FACE_CONSONANT, // TH
  FACE_CONSONANT, // UH
  FACE_CONSONANT, // UW
  FACE_FF,        // V
  FACE_CONSONANT, // W
  FACE_UU,        // Y
  FACE_CONSONANT, // Z
  FACE_CONSONANT  // ZH
};
  
// Structure and array to hold face .X file names
typedef struct {
  DWORD ID;
  char  MeshName[MAX_PATH];
} sFaceMeshAssignment;
sFaceMeshAssignment g_FaceMeshAssignments[] = 
{
  { FACE_NEUTRAL,   "neutral"   },
  { FACE_BLINK,     "blink"     },
  { FACE_CONSONANT, "consonant" },
  { FACE_AA,        "aaa"       },
  { FACE_EE,        "eee"       },
  { FACE_II,        "iii"       },
  { FACE_OO,        "ooo"       },
  { FACE_UU,        "uuu"       },
  { FACE_FF,        "fff"       },
  { FACE_SMILE,     "smile"     }
};

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL DoInit(HWND hWnd);
void DoShutdown();
void DoFrame();

void PlaySequence(char *XFilename, WCHAR *WAVFilename);
void FreeSequence();

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

  // Load the body mesh
  LoadMesh(&g_BodyMesh, NULL, g_pD3DDevice, "..\\Data\\Body.x", "..\\Data\\");

  // Load the facial meshes (don't load frames)
  LoadMesh(&g_FaceMeshes, NULL, g_pD3DDevice, "..\\Data\\Faces.x", "..\\Data\\", BLENDFVF);

  // Load the blended morphing vertex shader
  LoadBlendingShader(g_pD3DDevice);

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

  // Setup a light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(Light));
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Direction = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  // Play the initial phoneme sequence
  PlaySequence("..\\Data\\Speech1.x", L"..\\Data\\Speech1.wav");

  return TRUE;
}

void DoShutdown()
{
  // Stop playback and free Directshow objects
  if(g_pControl) {
    g_pControl->Stop();
    g_pControl->Release();
    g_pControl = NULL;
  }
  ReleaseCOM(g_pEvent);
  ReleaseCOM(g_pPosition);
  ReleaseCOM(g_pGraph);

  // Free phoneme data
  g_PhonemeParser.Free();

  // Free face meshes
  delete g_FaceMeshes; g_FaceMeshes = NULL;

  // Free vertex shader
  FreeBlendingShader();

  // Free body mesh
  delete g_BodyMesh; g_BodyMesh = NULL;

  // Release Backdrop data
  ReleaseCOM(g_BackdropVB);
  ReleaseCOM(g_BackdropTexture);

  // Shutdown Direct3D
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD Sequence = 1;

  // Determine if current sample if done playing, and if
  // so, go ahead and play a new random one.
  if(g_pEvent) {
    long Code, Param1, Param2;
    while(g_pEvent->GetEvent(&Code, &Param1, &Param2, 1) == S_OK) {

      // Sound is complete, pick another
      if(Code == EC_COMPLETE) {

        // Increase sequence count
        Sequence++;
        if(Sequence >= 6)
          Sequence = 1;

        // Build the names to load the new sequence
        char  XFilename[MAX_PATH];
        WCHAR WAVFilename[MAX_PATH];
        sprintf(XFilename, "..\\Data\\Speech%lu.x", Sequence);
        swprintf(WAVFilename, L"..\\Data\\Speech%lu.wav", Sequence);
        PlaySequence(XFilename, WAVFilename);
      }
      
      // Release the event parameters
      g_pEvent->FreeEventParams(Code, Param1, Param2);
    }
  }

  // Calculate time based on sound position (if playing)
  DWORD Time = timeGetTime();
  if(g_pPosition) {
    REFTIME SoundTime;  // REFTIME = float
    g_pPosition->get_CurrentPosition(&SoundTime);
    Time = (DWORD)(SoundTime * 1000.0f);
  }

  // Calculate timing of eyes blinking.
  // Eyes will remain open 3 seconds and blink for 1/3 second
  float BlinkTime = (float)(Time % 3301);
  if(BlinkTime < 3000.0f)
    BlinkTime = 0.0f;
  else {
    // Calculate scalar based on time (ranging from 0 to 300)
    BlinkTime -= 3000.0f;
    if(BlinkTime > 150.0f)
      BlinkTime = 300.0f - BlinkTime;
    BlinkTime *= (1000.0f / 150.0f);
    BlinkTime /= 1000.0f;
  }

  // Set emotion time (0=not smiling)
  static float EmotionTime = 0.0f;

  // Get phoneme animation data based on time
  DWORD PhonemeTime = Time % g_PhonemeParser.m_Length;
  DWORD Phoneme1, Phoneme2;
  float Phoneme1Time, Phoneme2Time;
  g_PhonemeParser.GetAnimData(PhonemeTime, &Phoneme1, &Phoneme1Time, &Phoneme2, &Phoneme2Time);

  // Set the view transformation
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(-4.0f, 2.0f, -17.0f),
                     &D3DXVECTOR3(-4.0f, 3.0f, 0.0f),
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

    // Enable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Draw the body mesh
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMeshes(g_BodyMesh);

    // Disable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Draw the blended facial mesh
    D3DXMATRIX matRotation;
    D3DXMatrixRotationYawPitchRoll(&matRotation, 0.191017f, -0.1f, 0.0f);
    D3DXMatrixTranslation(&matWorld, -0.062f, 4.65f, 0.656f);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &(matRotation*matWorld));
    DrawBlendedMesh(g_FaceMeshes->Find("neutral"),
                    g_FaceMeshes->Find(g_FaceMeshAssignments[Phoneme1].MeshName), Phoneme1Time,
                    g_FaceMeshes->Find(g_FaceMeshAssignments[Phoneme2].MeshName), Phoneme2Time,
                    g_FaceMeshes->Find("smile"),                                  EmotionTime,
                    g_FaceMeshes->Find("blink"),                                  BlinkTime);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void PlaySequence(char *XFilename, WCHAR *WAVFilename)
{
  // Free a prior sequence
  FreeSequence();

  // Load the phoneme sequence
  g_PhonemeParser.Parse(XFilename);

  // Convert loaded phoneme set into our demo's set
  for(DWORD i=0;i<g_PhonemeParser.m_NumPhonemes;i++) {
    DWORD Code = g_PhonemeParser.m_Phonemes[i].Code;
    g_PhonemeParser.m_Phonemes[i].Code = g_PhonemeSet[Code];
  }

  // Initialize DirectShow for sound playback
  CoCreateInstance(CLSID_FilterGraph, NULL,                     \
                   CLSCTX_INPROC_SERVER, IID_IGraphBuilder,     \
                   (void**)&g_pGraph);

  // Load the speech sound
  g_pGraph->RenderFile(WAVFilename, NULL);

  // Query for the media interfaces
  g_pGraph->QueryInterface(IID_IMediaControl, (void**)&g_pControl);
  g_pGraph->QueryInterface(IID_IMediaEvent,   (void**)&g_pEvent);
  g_pGraph->QueryInterface(IID_IMediaPosition,(void**)&g_pPosition);

  // Play the speech sound
  g_pControl->Run();
}

void FreeSequence()
{
  // Stop playback
  if(g_pControl)
    g_pControl->Stop();

  // Free Directshow objects
  ReleaseCOM(g_pControl);
  ReleaseCOM(g_pEvent);
  ReleaseCOM(g_pPosition);
  ReleaseCOM(g_pGraph);

  // Free phoneme data
  g_PhonemeParser.Free();
}
