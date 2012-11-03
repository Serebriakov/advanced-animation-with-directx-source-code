#include <stdio.h>
#include <windows.h>
#include "resource.h"

#include "Direct3D.h"
#include "XParser.h"
#include "XFile.h"

// Root mesh container
D3DXMESHCONTAINER_EX *g_RootMesh = NULL;

// Direct3D interfaces
IDirect3D9       *g_pD3D       = NULL;
IDirect3DDevice9 *g_pD3DDevice = NULL;

// Window class and caption text
char g_szClass[]   = "ParseMeshClass";
char g_szCaption[] = ".X Mesh Parser Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void AddMeshToList(D3DXMESHCONTAINER_EX *Mesh, DWORD Indent, HWND hList);

///////////////////////////////////////////////////////////
// Mesh .X Parser class
///////////////////////////////////////////////////////////
class cXMeshParser : public cXParser
{
  protected:
    D3DXMESHCONTAINER_EX *m_RootMesh;

  protected:
    BOOL ParseObject(IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference);
  public:
    BOOL Load(char *Filename, D3DXMESHCONTAINER_EX **ppMesh);
};

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
  WNDCLASS wc;
  MSG      Msg;
  HWND     hWnd;

  // Initialize the COM system
  CoInitialize(NULL);

  // Create the window class here and register it
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = WindowProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = DLGWINDOWEXTRA;
  wc.hInstance     = hInst;
  wc.hIcon         = LoadIcon(hInst, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = g_szClass;
  if(!RegisterClass(&wc))
    return FALSE;

  // Create the dialog box window and show it
  hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MESHVIEW), 0, NULL);
  UpdateWindow(hWnd);
  ShowWindow(hWnd, nCmdShow);

  // Create a Direct3D interface
  InitD3D(&g_pD3D, &g_pD3DDevice, hWnd, TRUE);

  // Enter the message loop
  while(GetMessage(&Msg, NULL, 0, 0)) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  // Release Direct3D interfaces
  ReleaseCOM(g_pD3DDevice);
  ReleaseCOM(g_pD3D);

  // Unregister the window class
  UnregisterClass(g_szClass, hInst);

  // Shut down the COM system
  CoUninitialize();

  return 0;
}

long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg,              \
                           WPARAM wParam, LPARAM lParam)
{
  OPENFILENAME ofn;
  char Filename[MAX_PATH];

  switch(uMsg) {
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDC_SELECT:
          
          // Clear list
          SendMessage(GetDlgItem(hWnd, IDC_MESHLIST), LB_RESETCONTENT, 0, 0);

          // Get filename to open
          Filename[0] = 0;
          ZeroMemory(&ofn, sizeof(OPENFILENAME));
          ofn.lStructSize = sizeof(OPENFILENAME);
          ofn.nMaxFile = MAX_PATH;
          ofn.nMaxFileTitle = MAX_PATH;
          ofn.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
          ofn.hwndOwner   = hWnd;
          ofn.lpstrFile   = Filename;
          ofn.lpstrTitle  = "Load and Parse .X File";
          ofn.lpstrFilter = ".X Files (*.x)\0*.x\0All Files (*.*)\0*.*\0\0";
          ofn.lpstrDefExt = "x";
          if(!GetOpenFileName(&ofn))
            return 0;

          // Get rid of last loaded mesh list
          delete g_RootMesh; g_RootMesh = NULL;

          // Parse the .X file and display hierarchy
          cXMeshParser Parser;
          if(Parser.Load(Filename, &g_RootMesh) == TRUE)
            AddMeshToList(g_RootMesh, 2, GetDlgItem(hWnd, IDC_MESHLIST));

          break;
      }
      break;

    case WM_DESTROY:
      delete g_RootMesh; g_RootMesh = NULL;
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

BOOL cXMeshParser::ParseObject(                             \
                       IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference)
{ 
  const GUID *Type = GetObjectGUID(pDataObj);

  // Make sure template being parsed is a mesh (non-referenced)
  if(*Type == TID_D3DRMMesh && Reference == FALSE) {

    // Load the mesh data
    D3DXMESHCONTAINER_EX *Mesh = NULL;
    LoadMesh(&Mesh, g_pD3DDevice, pDataObj);

    // Assign a name if none already
    if(Mesh && !Mesh->Name)
      Mesh->Name = strdup("NoNameMesh");

    // Link mesh into head of list
    if(Mesh)
      Mesh->pNextMeshContainer = m_RootMesh;
    m_RootMesh = Mesh; Mesh = NULL;
  }

  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}    

BOOL cXMeshParser::Load(char *Filename, D3DXMESHCONTAINER_EX **ppMesh)
{
  m_RootMesh = NULL;
  if(Parse(Filename) == TRUE) {
    *ppMesh = m_RootMesh; m_RootMesh = NULL;
    return TRUE;
  }
  return FALSE;
}

void AddMeshToList(D3DXMESHCONTAINER_EX *Mesh, DWORD Indent, HWND hList)
{
  // Error checking
  if(!Mesh)
    return;

  // Add siblings to list first
  if(Mesh->pNextMeshContainer)
    AddMeshToList((D3DXMESHCONTAINER_EX*)Mesh->pNextMeshContainer, Indent, hList);

  // Build text to add to list
  char Text[1024];
  if(Mesh->pSkinMesh && Mesh->pSkinInfo)
    sprintf(Text, "%s (SKINNED ) Verts: %lu, Faces: %lu, Bones: %lu", Mesh->Name, Mesh->MeshData.pMesh->GetNumVertices(), Mesh->MeshData.pMesh->GetNumFaces(), Mesh->pSkinInfo->GetNumBones());
  else
    sprintf(Text, "%s (STANDARD) Verts: %lu, Faces: %lu", Mesh->Name, Mesh->MeshData.pMesh->GetNumVertices(), Mesh->MeshData.pMesh->GetNumFaces());
  SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)Text);
}
