#include <stdio.h>
#include <windows.h>
#include "resource.h"

#include "Direct3D.h"
#include "XParser.h"
#include "XFile.h"

// Frame container
D3DXFRAME_EX *g_RootFrame = NULL;

// Window class and caption text
char g_szClass[]   = "ParseFrameClass";
char g_szCaption[] = ".X Frame Parser Demo by Jim Adams";

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void AddFramesToList(D3DXFRAME_EX *Frame, DWORD Indent, HWND hList);

///////////////////////////////////////////////////////////
// Frame .X Parser class
///////////////////////////////////////////////////////////
class cXFrameParser : public cXParser
{
  protected:
    D3DXFRAME_EX *m_RootFrame;

  protected:
    BOOL ParseObject(IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference);
  public:
    BOOL Load(char *Filename, D3DXFRAME_EX **ppFrame);
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
  hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FRAMEVIEW), 0, NULL);
  UpdateWindow(hWnd);
  ShowWindow(hWnd, nCmdShow);

  // Enter the message loop
  while(GetMessage(&Msg, NULL, 0, 0)) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

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
          SendMessage(GetDlgItem(hWnd, IDC_FRAMELIST), LB_RESETCONTENT, 0, 0);

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

          // Get rid of last loaded frame hierarchy
          delete g_RootFrame; g_RootFrame = NULL;

          // Parse the .X file and display hierarchy
          cXFrameParser Parser;
          if(Parser.Load(Filename, &g_RootFrame) == TRUE)
            AddFramesToList(g_RootFrame, 2, GetDlgItem(hWnd, IDC_FRAMELIST));

          break;
      }
      break;

    case WM_DESTROY:
      delete g_RootFrame; g_RootFrame = NULL;
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

BOOL cXFrameParser::ParseObject(                            \
                       IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference)
{ 
  const GUID *Type = GetObjectGUID(pDataObj);

  // Make sure template being parsed is a frame (non-referenced)
  if(*Type == TID_D3DRMFrame && Reference == FALSE) {

    // Allocate a frame structure
    D3DXFRAME_EX *Frame = new D3DXFRAME_EX();

    // Get frame name (assign one if none found)
    if((Frame->Name = GetObjectName(pDataObj)) == NULL)
      Frame->Name = strdup("NoNameFrame");

    // Link frame structure into list
    if(Data == NULL) {
      // Link as sibling of root
      Frame->pFrameSibling = m_RootFrame;
      m_RootFrame = Frame;
      Data = (void**)&m_RootFrame;
    } else {
      // Link as child of supplied frame
      D3DXFRAME_EX *FramePtr = (D3DXFRAME_EX*)*Data;
      Frame->pFrameSibling = FramePtr->pFrameFirstChild;
      FramePtr->pFrameFirstChild = Frame;
      Data = (void**)&FramePtr->pFrameFirstChild;
    }

    // Clear frame pointer since it's been assigned
    Frame = NULL;
  }

  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}    

BOOL cXFrameParser::Load(char *Filename, D3DXFRAME_EX **ppFrame)
{
  m_RootFrame = NULL;
  if(Parse(Filename) == TRUE) {
    *ppFrame = m_RootFrame; m_RootFrame = NULL;
    return TRUE;
  }
  return FALSE;
}

void AddFramesToList(D3DXFRAME_EX *Frame, DWORD Indent, HWND hList)
{
  char Text[1024];

  // Error checking
  if(!Frame)
    return;

  // Add siblings to list first
  if(Frame->pFrameSibling)
    AddFramesToList((D3DXFRAME_EX*)Frame->pFrameSibling, Indent, hList);

  // Build text to add to list
  memset(Text, ' ', Indent);
  Text[Indent] = 0;
  strcat(Text, Frame->Name);
  SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)Text);

  // Add children to list
  if(Frame->pFrameFirstChild)
    AddFramesToList((D3DXFRAME_EX*)Frame->pFrameFirstChild, Indent+4, hList);

}
