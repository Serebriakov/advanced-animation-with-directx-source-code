#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Direct3D objects
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Robot mesh object
D3DXMESHCONTAINER_EX *g_RobotMesh = NULL;

// Keyframe structure
typedef struct sKeyframe 
{
  DWORD     Time;
  D3DMATRIX matTransformation;
} sKeyframe;

// Keyframes used in demo
sKeyframe g_Keyframes[4] = 
{
  // Keyframe 1, 0ms
  {   0, 1.000000f, 0.000000f, 0.000000f, 0.000000f,
         0.000000f, 1.000000f, 0.000000f, 0.000000f,
         0.000000f, 0.000000f, 1.000000f, 0.000000f,
         0.000000f, 0.000000f, 0.000000f, 1.000000f },

  // Keyframe 2, 40ms
  {  400, 0.000796f, 1.000000f, 0.000000f, 0.000000f,
         -1.000000f, 0.000796f, 0.000000f, 0.000000f,
          0.000000f, 0.000000f, 1.000000f, 0.000000f,
         50.000000f, 0.000000f, 0.000000f, 1.000000f },

  // Keyframe 3, 80ms
  {  800, -0.999999f,  0.001593f, 0.000000f, 0.000000f,
          -0.001593f, -0.999999f, 0.000000f, 0.000000f,
           0.000000f,  0.000000f, 1.000000f, 0.000000f,
          25.000000f, 25.000000f, 0.000000f, 1.000000f },

  // Keyframe 4, 120ms
  { 1200, 1.000000f, 0.000000f, 0.000000f, 0.000000f,
          0.000000f, 1.000000f, 0.000000f, 0.000000f,
          0.000000f, 0.000000f, 1.000000f, 0.000000f,
          0.000000f, 0.000000f, 0.000000f, 1.000000f }
};

// Window class and caption text
char g_szClass[]   = "TimedAnimClass";
char g_szCaption[] = "Timed Animation Demo by Jim Adams";

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

  // Load the robot mesh
  LoadMesh(&g_RobotMesh, g_pD3DDevice, "..\\Data\\robot.x", "..\\Data\\");

  // Setup a directional light
  D3DLIGHT9 Light;
  ZeroMemory(&Light, sizeof(D3DLIGHT9));
  Light.Type = D3DLIGHT_DIRECTIONAL;
  Light.Diffuse.r = Light.Diffuse.g = Light.Diffuse.b = Light.Diffuse.a = 1.0f;
  Light.Direction = D3DXVECTOR3(0.0f, -0.5f, 0.5f);
  g_pD3DDevice->SetLight(0, &Light);
  g_pD3DDevice->LightEnable(0, TRUE);

  return TRUE;
}

void DoShutdown()
{
  // Free mesh data
  delete g_RobotMesh;  g_RobotMesh  = NULL;

  // Release D3D objects
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);
}

void DoFrame()
{
  static DWORD StartTime = timeGetTime();

  // Get the current time into the animation
  DWORD Time = timeGetTime() - StartTime;

  // Bounds the time to the animation time
  Time %= (g_Keyframes[3].Time+1);

  // Determine which keyframe to use
  DWORD Keyframe = 0;  // Start at first keyframe
  for(DWORD i=0;i<4;i++) {

    // If time is greater or equal to a 
    // key-frame's time then update the 
    // keyframe to use
    if(Time >= g_Keyframes[i].Time)
      Keyframe = i;
  }

  // Get second keyframe 
  DWORD Keyframe2 = (Keyframe==3) ? Keyframe:Keyframe + 1;

  // Calculate the difference in time between keyframes
  // and calculate a scalar value to use for adjusting
  // the transformations.
  DWORD TimeDiff = g_Keyframes[Keyframe2].Time - 
                   g_Keyframes[Keyframe].Time;
  if(!TimeDiff) 
    TimeDiff=1;
  float Scalar = (float)(Time - g_Keyframes[Keyframe].Time) / (float)TimeDiff;

  // Calculate the difference in transformations
  D3DXMATRIX matInt = D3DXMATRIX(g_Keyframes[Keyframe2].matTransformation) - 
                      D3DXMATRIX(g_Keyframes[Keyframe].matTransformation);
  matInt *= Scalar; // Scale the difference

  // Add scaled transformation matrix back to 1st keyframe matrix
  matInt += D3DXMATRIX(g_Keyframes[Keyframe].matTransformation);

  // Set the world transformation matrix
  g_pD3DDevice->SetTransform(D3DTS_WORLD, &matInt);

  // Set a view transformation matrix
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView,
                     &D3DXVECTOR3(25.0f, 0.0f, -80.0f),
                     &D3DXVECTOR3(25.0f, 0.0f, 0.0f),
                     &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
  g_pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

  // Clear the device and start drawing the scene
  g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0,0,64,255), 1.0f, 0);
  if(SUCCEEDED(g_pD3DDevice->BeginScene())) {

    // Enable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Draw the robot mesh
    DrawMesh(g_RobotMesh);

    // Disable lighting
    g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // End the scene
    g_pD3DDevice->EndScene();
  }

  // Present the scene to the user
  g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}
