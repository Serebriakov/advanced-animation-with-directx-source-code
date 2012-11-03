#include <windows.h>

#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "Particles.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Helicopter smoke, tree, and people particle emitter objects
cParticleEmitter g_ChopperEmitter;
cParticleEmitter g_TreeEmitter;
cParticleEmitter g_PeopleEmitter;

// Helicopter, rotor, shadow, and backdrop mesh objects
D3DXMESHCONTAINER_EX *g_ChopperMesh  = NULL;
D3DXMESHCONTAINER_EX *g_RotorMesh    = NULL;
D3DXMESHCONTAINER_EX *g_ShadowMesh   = NULL;
D3DXMESHCONTAINER_EX *g_BackdropMesh = NULL;

// Helicopter's position and Y-axis rotation
D3DXVECTOR3 g_vecChopper = D3DXVECTOR3(0.0f, 50.0f, 0.0f);
float       g_rotChopper = 0.0f;

// Window class and caption text
char g_szClass[]   = "ParticlesClass";
char g_szCaption[] = "Particles Demo by Jim Adams";

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

  // Load the helicopter, rotor, and shadow meshes
  LoadMesh(&g_ChopperMesh, g_pD3DDevice, "..\\Data\\Chopper.x", "..\\Data\\");
  LoadMesh(&g_RotorMesh,   g_pD3DDevice, "..\\Data\\Rotor.x",   "..\\Data\\");
  LoadMesh(&g_ShadowMesh,  g_pD3DDevice, "..\\Data\\Shadow.x",  "..\\Data\\");

  // Load the backdrop mesh
  LoadMesh(&g_BackdropMesh, g_pD3DDevice, "..\\Data\\Backdrop.x", "..\\Data\\");

  // Create the particle emitters
  g_ChopperEmitter.Create(g_pD3DDevice, &D3DXVECTOR3(0.0f, 0.0f, 0.0f), EMITTER_CLOUD);
  g_TreeEmitter.Create(g_pD3DDevice,    &D3DXVECTOR3(0.0f, 0.0f, 0.0f), EMITTER_TREE);
  g_PeopleEmitter.Create(g_pD3DDevice,  &D3DXVECTOR3(0.0f, 0.0f, 0.0f), EMITTER_PEOPLE);

  // Populate the ground with trees and people
  DWORD Type = PARTICLE_TREE1;
  for(DWORD i=0;i<50;i++) {
    float rot  = (float)(rand() % 628) / 100.0f;
    float dist = (float)(rand() % 400);
    g_TreeEmitter.Add(Type++,
                      &D3DXVECTOR3((float)cos(rot)*dist,25.0f,(float)sin(rot)*dist),
                      50.0f, 0xFFFFFFFF, 0, &D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    if(Type > PARTICLE_TREE3)
      Type = PARTICLE_TREE1;

    rot  = (float)(rand() % 628) / 100.0f;
    dist = (float)(rand() % 400);
    g_PeopleEmitter.Add(PARTICLE_PEOPLE1,
                      &D3DXVECTOR3((float)cos(rot)*dist,7.5f,(float)sin(rot)*dist),
                      15.0f, 0xFFFFFFFF, 0, &D3DXVECTOR3(0.0f, 0.0f, 0.0f));
  }

  // Setup a light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(D3DLIGHT9));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Direction = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  // Play a chopper sound
  PlaySound("..\\Data\\Chopper.wav", NULL, SND_LOOP | SND_ASYNC);

  return TRUE;
}

void DoShutdown()
{
  // Stop the sound
  PlaySound(NULL, NULL, 0);

  // Release particle emitters
  g_ChopperEmitter.Free();
  g_TreeEmitter.Free();
  g_PeopleEmitter.Free();

  // Release meshes
  delete g_BackdropMesh; g_BackdropMesh = NULL;
  delete g_ShadowMesh;   g_ShadowMesh   = NULL;
  delete g_RotorMesh;    g_RotorMesh    = NULL;
  delete g_ChopperMesh;  g_ChopperMesh  = NULL;

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD LastTime = timeGetTime();
  DWORD Time = timeGetTime();
  DWORD Elapsed;
  D3DXMATRIX matWorld;

  // Get elapsed time and update last frame's time
  Elapsed = Time - LastTime;
  LastTime = Time;

  // Rotate and move the helicopter
  g_rotChopper = (float)Time / 5000.0f;
  g_vecChopper.x += (float)cos(g_rotChopper) * ((float)Elapsed / 30.0f);
  g_vecChopper.z -= (float)sin(g_rotChopper) * ((float)Elapsed / 30.0f);

  // Call special function that creates smoke under helicopter
  g_ChopperEmitter.HandleSmoke(&g_vecChopper, Elapsed);

  // Call special function that makes people duck when chopper is too close
  g_PeopleEmitter.HandleDucking(&g_vecChopper);

  // Update the helicopter's particle emitter
  g_ChopperEmitter.Process(Elapsed);

  // Calculate a view transformation matrix
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(g_vecChopper.x, 80.0f, g_vecChopper.z - 150.0f),
                     &D3DXVECTOR3(g_vecChopper.x, 0.0f, g_vecChopper.z),
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(NULL, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0f, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Turn on lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Draw the backdrop mesh at origin
    D3DXMatrixIdentity(&matWorld);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_BackdropMesh);

    // Draw the helicopter's shadow
    D3DXMatrixTranslation(&matWorld, g_vecChopper.x, 0.5f, g_vecChopper.z);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_ShadowMesh);

    // Draw the helicopter
    D3DXMatrixRotationYawPitchRoll(&matWorld, g_rotChopper, 0.0f, -0.0872222f);
    matWorld._41 = g_vecChopper.x;  // Set position manually in matrix
    matWorld._42 = g_vecChopper.y;
    matWorld._43 = g_vecChopper.z;
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
    DrawMesh(g_ChopperMesh);

    // Draw the rotor (rotate and tilt it forward a bit and position above helicopter)
    D3DXMATRIX matRotY, matRotZ, matRotYY;
    D3DXMatrixRotationY(&matRotY, (float)Time / 10.0f);  // Turn rotor
    D3DXMatrixRotationZ(&matRotZ, -0.174444f);                    // Angle forward
    D3DXMatrixRotationY(&matRotYY, g_rotChopper);                 // Orient to chopper
    D3DXMatrixTranslation(&matWorld, g_vecChopper.x, g_vecChopper.y, g_vecChopper.z);
    g_pD3DDevice->SetTransform(D3DTS_WORLD, &(matRotY * matRotZ * matRotYY * matWorld));
    DrawMesh(g_RotorMesh);

    // Draw the particles (turn off lights and enable alpha testing)
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    g_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, 0x08);
    g_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    g_TreeEmitter.Render(&matView);
    g_PeopleEmitter.Render(&matView);

    // Enable alpha blending for the smoke particles
    g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    g_ChopperEmitter.Render(&matView);
    g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);


    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
