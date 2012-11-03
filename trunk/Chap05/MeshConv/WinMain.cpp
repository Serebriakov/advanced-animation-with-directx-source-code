/*
  Make sure you have the /J compiler switch in your project settings!
  The /J switch ensures chars are unsigned by default.
*/

#include <windows.h>
#include <stdio.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "resource.h"
#include "MD2.h"
#include "MS3D.h"

// Window class
char g_szClass[] = "MESHCONV";

// Open/save filename structures and buffers
OPENFILENAME g_ofn, g_sfn;
char g_MD2Filename[MAX_PATH];
char g_MS3DFilename[MAX_PATH];
char g_XFilename[MAX_PATH];
char g_TextureFilename[MAX_PATH];

// Function prototypes
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow);
long FAR PASCAL WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL ConvertMD2(char *SrcFilename, char *TextureFilename, char *DestFilename);
BOOL ConvertMS3D(char *SrcFilename, char *DestFilename);
void WriteFrame(sMS3DBoneContainer *Bone, FILE *fp, DWORD Indent, BOOL Close);
void CombineTransformation(sMS3DBoneContainer *Bone, D3DXMATRIX *Matrix);
void CreateYawPitchRollRotationMatrix(D3DXMATRIX *matRotation,
                                      float Yaw, float Pitch, float Roll);

// Application //////////////////////////////////////////////////////
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
  WNDCLASS wc;
  MSG      Msg;

  // Register window class
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
  RegisterClass(&wc);

  // Create the dialog box window and show it
  HWND hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONVERSION), 0, NULL);
  UpdateWindow(hWnd);
  ShowWindow(hWnd, nCmdShow);

  // Setup open filename data structure
  ZeroMemory(&g_ofn, sizeof(OPENFILENAME));
  g_ofn.lStructSize   = sizeof(OPENFILENAME);
  g_ofn.hInstance     = hInst;
  g_ofn.hwndOwner     = hWnd;
  g_ofn.nMaxFile      = MAX_PATH;
  g_ofn.nMaxFileTitle = MAX_PATH;
  g_ofn.Flags         = OFN_HIDEREADONLY;

  // Setup save filename data structure
  ZeroMemory(&g_sfn, sizeof(OPENFILENAME));
  g_sfn.lStructSize   = sizeof(OPENFILENAME);
  g_sfn.hInstance     = hInst;
  g_sfn.hwndOwner     = hWnd;
  g_sfn.nMaxFile      = MAX_PATH;
  g_sfn.nMaxFileTitle = MAX_PATH;
  g_sfn.Flags         = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
  g_sfn.lpstrDefExt   = "x";
  g_sfn.lpstrFile     = g_XFilename;
  g_sfn.lpstrTitle    = "Select .X file";
  g_sfn.lpstrFilter   = ".X files (*.X)\0*.x\0All files (*.*)\0*.*\0\0";

  // Clear filenames
  g_MD2Filename[0] = g_MS3DFilename[0] = g_XFilename[0] = 0;

  // Message loop
  while(GetMessage(&Msg, NULL, 0, 0)) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  // Clean up
  UnregisterClass(g_szClass, hInst);

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {

    case WM_COMMAND:

      switch(LOWORD(wParam)) {

        // Convert .MD2 to .X
        case IDC_CONVERTMD2:

          // Get the source filename
          g_ofn.lpstrDefExt = "md2";
          g_ofn.lpstrFile   = g_MD2Filename;
          g_ofn.lpstrTitle  = "Select .MD2 file";
          g_ofn.lpstrFilter = ".MD2 files (*.MD2)\0*.md2\0All files (*.*)\0*.*\0\0";
          if(!GetOpenFileName(&g_ofn))
            break;

          // Get texture filename
          g_ofn.lpstrDefExt = "bmp";
          g_ofn.lpstrFile   = g_TextureFilename;
          g_ofn.lpstrTitle  = "Select texture file";
          g_ofn.lpstrFilter = ".bmp files (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0";
          if(!GetOpenFileName(&g_ofn))
            break;

          // Get the destination filename
          if(!GetSaveFileName(&g_sfn))
            break;

          // Do file conversion
          if(ConvertMD2(g_MD2Filename, g_TextureFilename, g_XFilename) == TRUE)
            MessageBox(hWnd, "Conversion complete!", "Success", MB_OK);
          else
            MessageBox(hWnd, "Conversion failed!", "Failure", MB_OK);

          break;

        // Convert .MS3D to .X
        case IDC_CONVERTMS3D:

          // Get the source filename
          g_ofn.lpstrDefExt = "ms3d";
          g_ofn.lpstrFile   = g_MS3DFilename;
          g_ofn.lpstrTitle  = "Select .MS3D file";
          g_ofn.lpstrFilter = ".MS3D files (*.MS3D)\0*.ms3d\0All files (*.*)\0*.*\0\0";
          if(!GetOpenFileName(&g_ofn))
            break;

          // Get the destination filename
          if(!GetSaveFileName(&g_sfn))
            break;

          // Do file conversion
          if(ConvertMS3D(g_MS3DFilename, g_XFilename) == TRUE)
            MessageBox(hWnd, "Conversion complete!", "Success", MB_OK);
          else
            MessageBox(hWnd, "Conversion failed!", "Failure", MB_OK);

          break;

      }
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

BOOL ConvertMD2(char *SrcFilename, char *TextureFilename, char *DestFilename)
{
  FILE *fp_in, *fp_out;

  sMD2Header        mdHeader;

  char             *mdFrame;
  sMD2Frame        *mdFramePtr;
  sMD2FrameVertex  *mdVertexPtr;

  sMD2TextureCoord *mdTextureCoords = NULL;
  sMD2Face         *mdFaces = NULL;

  char             *MeshNames = NULL;
  sMD2MeshVertex   *Vertices = NULL;
  unsigned short   *Indices = NULL;

  // Error checking
  if(!SrcFilename || !TextureFilename || !DestFilename)
    return FALSE;

  // Get pointer to local texture filename
  char *TextureFilenamePtr = strrchr(TextureFilename, '\\');
  if(!TextureFilenamePtr)
    TextureFilenamePtr = TextureFilename;
  else
    TextureFilenamePtr++;

  // Open the target file
  if((fp_out = fopen(DestFilename, "wb"))==NULL)
    return FALSE;

  // Write the target .X file header and templates
  fprintf(fp_out, "xof 0303txt 0032\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "// This file was created with MeshConv (c) 2003 by Jim Adams\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "template MorphAnimationKey\r\n");
  fprintf(fp_out, "{\r\n");
  fprintf(fp_out, "  <2746B58A-B375-4cc3-8D23-7D094D3C7C67>\r\n");
  fprintf(fp_out, "  DWORD  Time;        // Key's time\r\n");
  fprintf(fp_out, "  STRING MeshName;    // Mesh to use (name reference)\r\n");
  fprintf(fp_out, "}\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "template MorphAnimationSet\r\n");
  fprintf(fp_out, "{\r\n");
  fprintf(fp_out, "  <0892DE81-915A-4f34-B503-F7C397CB9E06>\r\n");
  fprintf(fp_out, "  DWORD Loop;     // 0=don't loop, 1=loop\r\n");
  fprintf(fp_out, "  DWORD NumKeys;  // # keys in animation\r\n");
  fprintf(fp_out, "  array MorphAnimationKey Keys[NumKeys];\r\n");
  fprintf(fp_out, "}\r\n");
  fprintf(fp_out, "\r\n");

  // Open the source file
  if((fp_in = fopen(SrcFilename, "rb"))==NULL) {
    fclose(fp_out);
    return FALSE;
  }

  // Read the .MD2 file header and make sure it's valid
  fread(&mdHeader, 1, sizeof(sMD2Header), fp_in);
  if(mdHeader.Signature != 0x32504449 || mdHeader.Version != 8) {
    fclose(fp_in);
    fclose(fp_out);
    return FALSE;
  }

  /////////////////////////////////////////
  //
  // Parse model data
  //
  /////////////////////////////////////////

  // Load texture coordinates /////////////////////////////
  if(mdHeader.NumTextureCoords) {
    mdTextureCoords = new sMD2TextureCoord[mdHeader.NumTextureCoords];
    fseek(fp_in, mdHeader.OffsetTextureCoords, SEEK_SET);
    fread(mdTextureCoords, 1, mdHeader.NumTextureCoords * sizeof(sMD2TextureCoord), fp_in);
  }

  // Load face data ///////////////////////////////////////
  if(mdHeader.NumFaces) {
    mdFaces = new sMD2Face[mdHeader.NumFaces];
    fseek(fp_in, mdHeader.OffsetFaces, SEEK_SET);
    fread(mdFaces, 1, mdHeader.NumFaces * sizeof(sMD2Face), fp_in);
  }

  // Allocate a temporary frame structure
  mdFrame = new char[mdHeader.FrameSize];
  mdFramePtr = (sMD2Frame*)mdFrame;

  // Store mesh values in local variables
  DWORD NumVertices  = mdHeader.NumFaces * 3;
  DWORD NumFaces     = mdHeader.NumFaces;

  // Allocate vertex and index buffers
  Vertices = new sMD2MeshVertex[NumVertices];
  Indices  = new unsigned short[NumFaces*3];

  // Allocate space for mesh names
  MeshNames = new char[16 * mdHeader.NumFrames];

  // Jump to frame data in file and begin reading in each frame
  fseek(fp_in, mdHeader.OffsetFrames, SEEK_SET);
  for(DWORD i=0;i<mdHeader.NumFrames;i++) {

    // Read in the frame (mesh)
    fread(mdFrame, 1, mdHeader.FrameSize, fp_in);

    // Assign a simple name if none already assigned (rarely happens)
    if(!mdFramePtr->Name[0])
      sprintf(mdFramePtr->Name, "NoName%02lu", i);

    // Copy name into mesh name buffer
    memcpy(&MeshNames[16*i], mdFramePtr->Name, 16);

    // Loop and build vertex/index data
    DWORD Num = 0;
    for(DWORD j=0;j<NumFaces;j++) {

      // Three vertices per face
      for(DWORD k=0;k<3;k++) {

        // Get a pointer to the frame vertex structure
        mdVertexPtr = &mdFramePtr->Vertices[mdFaces[j].Indices[k]];

        // Calculate coordinates (note ordering and reversal of x)
        Vertices[Num].x = -((float)mdVertexPtr->Vertex[1] * mdFramePtr->Scale[1] + mdFramePtr->Translate[1]);
        Vertices[Num].y =   (float)mdVertexPtr->Vertex[2] * mdFramePtr->Scale[2] + mdFramePtr->Translate[2];
        Vertices[Num].z =   (float)mdVertexPtr->Vertex[0] * mdFramePtr->Scale[0] + mdFramePtr->Translate[0];

        // Calculate normals
        Vertices[Num].nx = -g_Quake2Normals[mdVertexPtr->LightNormal][1];
        Vertices[Num].ny =  g_Quake2Normals[mdVertexPtr->LightNormal][2];
        Vertices[Num].nz =  g_Quake2Normals[mdVertexPtr->LightNormal][0];

        // Calculate texture coordinates
        Vertices[Num].u = (float)mdTextureCoords[mdFaces[j].TextureCoords[k]].u / (float)mdHeader.SkinWidth;
        Vertices[Num].v = (float)mdTextureCoords[mdFaces[j].TextureCoords[k]].v / (float)mdHeader.SkinHeight;

        // Store face order
        Indices[Num] = (unsigned short)Num;

        // Go to next vertex
        Num++;
      }
    }

    // Write out mesh header
    fprintf(fp_out, "Mesh %s {\r\n", mdFramePtr->Name);

    // Write out # vertices and write out vertex coordinates
    fprintf(fp_out, "  %lu;\r\n", NumVertices);
    for(j=0;j<NumVertices;j++) {
      fprintf(fp_out, "  %06lf;%06lf;%06lf;", Vertices[j].x, Vertices[j].y, Vertices[j].z);
      if(j < (NumVertices-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n");
    }

    // Write out # of faces and write out face data
    fprintf(fp_out, "  %lu;\r\n", NumFaces);
    for(j=0;j<NumFaces;j++) {
      fprintf(fp_out, "  3;%lu,%lu,%lu;", Indices[j*3], Indices[j*3+1], Indices[j*3+2]);
      if(j < (NumFaces-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n\r");
    }

    // Write normal data
    fprintf(fp_out, "  MeshNormals {\r\n");
    fprintf(fp_out, "    %lu;\r\n", NumVertices);
    for(j=0;j<NumVertices;j++) {
      fprintf(fp_out, "    %06lf;%06lf;%06lf;", Vertices[j].nx, Vertices[j].ny, Vertices[j].nz);
      if(j < (NumVertices-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n");
    }
    fprintf(fp_out, "    %lu;\r\n", NumFaces);
    for(j=0;j<NumFaces;j++) {
      fprintf(fp_out, "    3;%lu,%lu,%lu;", Indices[j*3], Indices[j*3+1], Indices[j*3+2]);
      if(j < (NumFaces-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n");
    }
    fprintf(fp_out, "  }\r\n");

    // Write texture coordinates
    fprintf(fp_out, "  MeshTextureCoords {\r\n");
    fprintf(fp_out, "    %lu;\r\n", NumVertices);
    for(j=0;j<NumVertices;j++) {
      fprintf(fp_out, "    %06lf;%06lf;", Vertices[j].u, Vertices[j].v);
      if(j < (NumVertices-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n");
    }
    fprintf(fp_out, "  }\r\n");

    // Write mesh material list
    fprintf(fp_out, "  MeshMaterialList {\r\n");
    fprintf(fp_out, "    1;\r\n    %lu;\r\n", NumFaces);
    for(j=0;j<NumFaces;j++) {
      fprintf(fp_out, "    0");
      if(j < (NumFaces-1))
        fprintf(fp_out, ",\r\n");
      else
        fprintf(fp_out, ";\r\n");
    }

    // Write material
    fprintf(fp_out, "    Material {\r\n");
    fprintf(fp_out, "      1.000000;1.000000;1.000000;1.000000;;\r\n");
    fprintf(fp_out, "      0.000000;\r\n");
    fprintf(fp_out, "      1.000000;1.000000;1.000000;;\r\n");
    fprintf(fp_out, "      1.000000;1.000000;1.000000;;\r\n");
    fprintf(fp_out, "      TextureFilename {\r\n");
    fprintf(fp_out, "        %c%s%c;\r\n", '"', TextureFilenamePtr, '"');
    fprintf(fp_out, "      }\r\n");
    fprintf(fp_out, "    }\r\n");

    // Close mesh materal list
    fprintf(fp_out, "  }\r\n");

    // Close mesh template data object
    fprintf(fp_out, "}\r\n");
  }

  // Close the input file
  fclose(fp_in);

  // Free used resource
  delete [] Vertices;
  delete [] Indices;
  delete [] mdFrame;
  delete [] mdTextureCoords;
  delete [] mdFaces;

  /////////////////////////////////////////////////
  //
  // Write the animation sets
  //
  /////////////////////////////////////////////////

  // Clear out a buffer for last known mesh sequence's name
  char LastName[16];
  for(DWORD j=0;j<16;j++)
    LastName[j] = (isdigit(MeshNames[j]) && j >= strlen(&MeshNames[0])-2) ? 0:MeshNames[j];

  // Reset # of frames count
  DWORD NumFrames = 0;

  // Go through each mesh (starting at 2nd mesh since 1st already counted)
  for(i=0;i<mdHeader.NumFrames;i++) {

    // Get name of mesh (clean sequence numbers)
    char CleanName[16];
    for(DWORD j=0;j<16;j++)
      CleanName[j] = (isdigit(MeshNames[16*i+j]) && j >= strlen(&MeshNames[16*i])-2) ? 0:MeshNames[16*i+j];

    // Compare names and increase count if matched last name
    if(!strcmp(LastName, CleanName)) {
      NumFrames++;
    } else {

      // Output animation template
      fprintf(fp_out, "MorphAnimationSet %s {\r\n", LastName);
      fprintf(fp_out, "  1;\r\n");  // Always loop
      fprintf(fp_out, "  %lu;\r\n", NumFrames+1);
      for(DWORD k=0;k<NumFrames;k++)
        fprintf(fp_out, "  %lu; %c%s%c;,\r\n", k*MD2FPS, '"', &MeshNames[16*(i-NumFrames+k)], '"');
      fprintf(fp_out, "  %lu; %c%s%c;;\r\n", k*MD2FPS, '"', &MeshNames[16*(i-NumFrames)], '"');
      fprintf(fp_out, "}\r\n");

      // Record last name and go to next animation
      strcpy(LastName, CleanName);
      NumFrames = 1;
    }
  }

  // Check if any frames left to output
  if(NumFrames) {
    // Output animation template
    fprintf(fp_out, "MorphAnimationSet %s {\r\n", LastName);
    fprintf(fp_out, "  1;\r\n");  // Always loop
    fprintf(fp_out, "  %lu;\r\n", NumFrames+1);
    for(DWORD k=0;k<NumFrames;k++)
      fprintf(fp_out, "  %lu; %c%s%c;,\r\n", k*MD2FPS, '"', &MeshNames[16*(i-NumFrames+k)], '"');
    fprintf(fp_out, "  %lu; %c%s%c;;\r\n", k*MD2FPS, '"', &MeshNames[16*(i-NumFrames)], '"');
    fprintf(fp_out, "}\r\n");
  }

  // Close the output file
  fclose(fp_out);

  // Free mesh names 
  delete [] MeshNames;

  // Return success
  return TRUE;
}

BOOL ConvertMS3D(char *SrcFilename, char *DestFilename)
{
  FILE           *fp_in, *fp_out;

  DWORD           i, j, k, Num;
  
  sMS3DHeader     msHeader;

  unsigned short  msNumVertices;
  sMS3DVertex    *msVertices = NULL;

  sMS3DMeshVertex *Vertices;
  unsigned short  *Faces;

  unsigned short  msNumFaces;
  sMS3DFace      *msFaces = NULL;
  unsigned short *Indices = NULL;

  unsigned short  msNumGroups;
  sMS3DGroup     *msGroups = NULL;

  unsigned short  msNumMaterials;
  sMS3DMaterial  *msMaterials = NULL;

  unsigned short  msNumBones;
  sMS3DBone      *msBones = NULL;
  sMS3DBoneContainer *Bones = NULL;

  float           msAnimationFPS;
  float           msCurrentTime;
  int             msTotalFrames;

  // Error checking
  if(!SrcFilename || !DestFilename)
    return FALSE;

  // Open the source file
  if((fp_in = fopen(SrcFilename, "rb"))==NULL)
    return FALSE;

  // Read in the header and make sure it's valid
  fread(&msHeader, 1, sizeof(sMS3DHeader), fp_in);
  if(memcmp(msHeader.Signature, "MS3D000000", 10)) {
    fclose(fp_in);
    return FALSE;
  }

  ///////////////////////////////////////////////
  //
  // Read data into some temporary buffers
  //
  ///////////////////////////////////////////////

  // Read vertex data
  fread(&msNumVertices, 1, sizeof(short), fp_in);
  if(msNumVertices) {
    msVertices = new sMS3DVertex[msNumVertices];
    for(i=0;i<msNumVertices;i++) {
      fread(&msVertices[i], 1, sizeof(sMS3DVertex), fp_in);
    }
  }

  // Read face data
  fread(&msNumFaces, 1, sizeof(short), fp_in);
  if(msNumFaces) {
    msFaces = new sMS3DFace[msNumFaces];
    for(i=0;i<msNumFaces;i++)
      fread(&msFaces[i], 1, sizeof(sMS3DFace), fp_in);
  }

  // Read groups
  fread(&msNumGroups, 1, sizeof(short), fp_in);
  if(msNumGroups) {
    msGroups = new sMS3DGroup[msNumGroups];
    for(i=0;i<msNumGroups;i++) {
      fread(&msGroups[i], 1, sizeof(msGroups[i].Header), fp_in);
      if((Num = msGroups[i].Header.NumFaces)) {
        msGroups[i].Indices = new unsigned short[Num];
        for(j=0;j<Num;j++)
          fread(&msGroups[i].Indices[j], 1, sizeof(short), fp_in);
      }
      fread(&msGroups[i].Material, 1, sizeof(char), fp_in);

      // Change group material to 0 if none assigned (-1)
      if(msGroups[i].Material == -1)
        msGroups[i].Material = 0;
    }
  }

  // Read materials, creating a default one if none in file
  fread(&msNumMaterials, 1, sizeof(short), fp_in);
  if(!msNumMaterials) {
    // Create a single material and color it white
    msNumMaterials = 1;
    msMaterials = new sMS3DMaterial[1];
    ZeroMemory(&msMaterials[0], sizeof(sMS3DMaterial));
    msMaterials[0].Diffuse[0] = msMaterials[0].Diffuse[1] = msMaterials[0].Diffuse[2] = 1.0f;

    // Set all groups to use material #0
    // If there are no materials, set all groups to 
    // use material #0
    if(msNumGroups) {
      for(i=0;i<msNumGroups;i++)
        msGroups[i].Material = 0;
    }
  } else {
    // Read in all materials from file
    msMaterials = new sMS3DMaterial[msNumMaterials];
    for(i=0;i<msNumMaterials;i++)
      fread(&msMaterials[i], 1, sizeof(sMS3DMaterial), fp_in);
  }

  // Read in FPS, current editor time, and # frames
  fread(&msAnimationFPS, 1, sizeof(float), fp_in);
  fread(&msCurrentTime, 1, sizeof(float), fp_in);
  fread(&msTotalFrames, 1, sizeof(int), fp_in);

  // Read in bones, skipping keyframe data
  fread(&msNumBones, 1, sizeof(short), fp_in);
  if(msNumBones) {
    msBones = new sMS3DBone[msNumBones];
    for(i=0;i<msNumBones;i++) {
      fread(&msBones[i], 1, sizeof(msBones[i].Header), fp_in);

      // Allocate key frames (if any)
      if((Num = msBones[i].Header.NumRotFrames)) {
        msBones[i].RotKeyFrames = new sMS3DKeyFrame[Num]();
        for(j=0;j<Num;j++)
          fread(&msBones[i].RotKeyFrames[j], 1, sizeof(sMS3DKeyFrame), fp_in);
      }

      if((Num=msBones[i].Header.NumPosFrames)) {
        msBones[i].PosKeyFrames = new sMS3DKeyFrame[Num]();
        for(j=0;j<Num;j++)
          fread(&msBones[i].PosKeyFrames[j], 1, sizeof(sMS3DKeyFrame), fp_in);
      }
    }
  }

  // Compile a new vertex list based on vertex information from faces
  // Clip out duplicates and build a face list.
  DWORD NumMeshVertices = 0;
  Vertices = new sMS3DMeshVertex[msNumFaces*3]();
  Faces    = new unsigned short[msNumFaces*3];

  // Clear vertex list data
  for(i=0;i<(DWORD)msNumFaces*3;i++) {
    ZeroMemory(&Vertices[i], sizeof(sMS3DMeshVertex));
    Vertices[i].BoneNum = -2;
  }

  for(i=0;i<msNumFaces;i++) {
    for(j=0;j<3;j++) {
      unsigned short Index = msFaces[i].Indices[j];

      // Allocate a vertex structure and fill it w/data
      sMS3DMeshVertex NewVertex;
      NewVertex.x  = msVertices[Index].Pos[0];
      NewVertex.y  = msVertices[Index].Pos[1];
      NewVertex.z  = -msVertices[Index].Pos[2];
      NewVertex.nx = msFaces[i].Normal[j][0];
      NewVertex.ny = msFaces[i].Normal[j][1];
      NewVertex.nz = -msFaces[i].Normal[j][2];
      NewVertex.u  = msFaces[i].u[j];
      NewVertex.v  = msFaces[i].v[j];
      NewVertex.BoneNum = msVertices[Index].Bone;

      // Search existing list and look for match
      DWORD Match = -1;
      for(k=0;k<(DWORD)msNumFaces*3;k++) {
        if(!memcmp(&Vertices[k], &NewVertex, sizeof(sMS3DMeshVertex))) {
          Match = k;
          break;
        }
      }

      // If no match, then find an empty slot to use and reset index
      if(Match == -1) {
        for(k=0;k<(DWORD)msNumFaces*3;k++) {
          if(Vertices[k].BoneNum == -2) {
            Index = (unsigned short)k;
            memcpy(&Vertices[Index], &NewVertex, sizeof(sMS3DMeshVertex));
            NumMeshVertices++;
            break;
          }
        }
      } else {
        // Use matching index number
        Index = (unsigned short)Match;
      }

      // Store index number in face list
      Faces[i*3+j] = Index;
    }
  }

  // Build the frame hierarchy and combine the transformations
  if(msNumBones) {

    // Allocate array for bone data
    Bones = new sMS3DBoneContainer[msNumBones];

    // Store bone data
    for(i=0;i<msNumBones;i++) {

      // Copy over name and clean it of spaces
      strcpy(Bones[i].Name, msBones[i].Header.Name);
      for(j=0;j<strlen(Bones[i].Name);j++) { 
       if(Bones[i].Name[j] == ' ')
         Bones[i].Name[j] = '_';
      }

      // Create transformation matrix
      CreateYawPitchRollRotationMatrix(&Bones[i].matRotation,
                                       msBones[i].Header.Rotation[1],
                                       msBones[i].Header.Rotation[0],
                                       -msBones[i].Header.Rotation[2]);

      D3DXMatrixTranslation(&Bones[i].matTranslation, 
                            msBones[i].Header.Position[0],
                            msBones[i].Header.Position[1],
                            -msBones[i].Header.Position[2]);
      Bones[i].matTransformation = Bones[i].matRotation * Bones[i].matTranslation;
    }

    // Build hierarchy
    for(i=0;i<msNumBones;i++) {
      
      // Match parent
      if(msBones[i].Header.Parent[0]) {

        // Search for parent
        for(j=0;j<msNumBones;j++) {

          // i = current bone, j = searching bone

          // Compare bone names
          if(!stricmp(msBones[i].Header.Parent, msBones[j].Header.Name)) {
            
            // Link sibling
            Bones[i].Sibling = Bones[j].Child;

            // Link child
            Bones[j].Child = &Bones[i];
          }
        }
      }
    }

    // Calculate combined and inverse combined transformations
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    CombineTransformation(&Bones[0], &matIdentity);
  }

  // Open the target file
  if((fp_out = fopen(DestFilename, "wb"))==NULL)
    return FALSE;

  // Write the target .X file header and templates
  fprintf(fp_out, "xof 0303txt 0032\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "// This file was created with MeshConv (c) 2003 by Jim Adams\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "template XSkinMeshHeader {\r\n");
  fprintf(fp_out, " <3cf169ce-ff7c-44ab-93c0-f78f62d172e2>\r\n");
  fprintf(fp_out, " WORD nMaxSkinWeightsPerVertex;\r\n");
  fprintf(fp_out, " WORD nMaxSkinWeightsPerFace;\r\n");
  fprintf(fp_out, " WORD nBones;\r\n");
  fprintf(fp_out, "}\r\n");
  fprintf(fp_out, "\r\n");
  fprintf(fp_out, "template SkinWeights {\r\n");
  fprintf(fp_out, " <6f0d123b-bad2-4167-a0d0-80224f25fabb>\r\n");
  fprintf(fp_out, " STRING transformNodeName;\r\n");
  fprintf(fp_out, " DWORD nWeights;\r\n");
  fprintf(fp_out, " array DWORD vertexIndices[nWeights];\r\n");
  fprintf(fp_out, " array FLOAT weights[nWeights];\r\n");
  fprintf(fp_out, " Matrix4x4 matrixOffset;\r\n");
  fprintf(fp_out, "}\r\n");
  fprintf(fp_out, "\r\n");

  // Write out bone hierarchy if it exists
  char Indent[256]; Indent[0] = 0;
  if(msNumBones) {
    // Write root frame and recursively write out remaining
    WriteFrame(&Bones[0], fp_out, 0, FALSE);

    // Don't close out last frame - stick mesh into it
    strcpy(Indent, "  ");
  }

  // Name the mesh by clipping out mesh filename (without path and extension)
  char *NamePtr = strrchr(SrcFilename, '\\');
  if(!NamePtr)
    NamePtr= &SrcFilename[0];
  else 
    NamePtr++;
  char MeshName[MAX_PATH];
  for(i=0;i<strlen(NamePtr)+1;i++) {
    if(NamePtr[i] == 32 || NamePtr[i] == '.')
      MeshName[i] = '_';
    else
      MeshName[i] = NamePtr[i];
  }

  // Write out mesh header
  fprintf(fp_out, "%sMesh %s {\r\n", Indent, MeshName);

  // Write out vertices
  fprintf(fp_out, "%s  %lu;\r\n", Indent, NumMeshVertices);
  for(i=0;i<NumMeshVertices;i++) {
    fprintf(fp_out, "%s  %06lf;%06lf;%06lf;", Indent, Vertices[i].x, Vertices[i].y, Vertices[i].z);
    if(i < (NumMeshVertices-1))
      fprintf(fp_out, ",\r\n");
    else
      fprintf(fp_out, ";\r\n");
  }

  // Write out face data
  fprintf(fp_out, "%s  %lu;\r\n", Indent, msNumFaces);
  for(i=0;i<msNumFaces;i++) {
    fprintf(fp_out, "%s  3;%lu,%lu,%lu;", Indent, Faces[i*3], Faces[i*3+2], Faces[i*3+1]);
    if(i < ((DWORD)msNumFaces-1))
      fprintf(fp_out, ",\r\n");
    else
      fprintf(fp_out, ";\r\n");
  }

  // Write out normal data
  fprintf(fp_out, "%s  MeshNormals {\r\n", Indent);
  fprintf(fp_out, "%s    %lu;\r\n", Indent, NumMeshVertices);
  for(i=0;i<NumMeshVertices;i++) {
    fprintf(fp_out, "%s    %06lf;%06lf;%06lf;", Indent, Vertices[i].nx, Vertices[i].ny, Vertices[i].nz);
    if(i == (NumMeshVertices-1))
      fprintf(fp_out, ";\r\n");
    else
      fprintf(fp_out, ",\r\n");
  }
  fprintf(fp_out, "%s    %lu;\r\n", Indent, msNumFaces);
  for(i=0;i<msNumFaces;i++) {
    fprintf(fp_out, "%s    3;%lu,%lu,%lu;", Indent, Faces[i*3], Faces[i*3+2], Faces[i*3+1]);
    if(i < ((DWORD)msNumFaces-1))
      fprintf(fp_out, ",\r\n");
    else
      fprintf(fp_out, ";\r\n");
  }
  fprintf(fp_out, "  }\r\n");

  // Write mesh material list
  fprintf(fp_out, "%s  MeshMaterialList {\r\n", Indent);
  fprintf(fp_out, "%s    %lu;\r\n", Indent, msNumMaterials);
  fprintf(fp_out, "%s    %lu;\r\n", Indent, msNumFaces);
  for(i=0;i<msNumFaces;i++) {
    fprintf(fp_out, "%s    %lu", Indent, msGroups[msFaces[i].GroupIndex].Material);
    if(i < ((DWORD)msNumFaces-1))
      fprintf(fp_out, ",\r\n");
    else
      fprintf(fp_out, ";\r\n");
  }

  // Write materials
  for(i=0;i<msNumMaterials;i++) {
    fprintf(fp_out, "%s    Material {\r\n", Indent);
    fprintf(fp_out, "%s      %06lf;%06lf;%06lf;%06lf;;\r\n", Indent, msMaterials[i].Diffuse[0], msMaterials[i].Diffuse[1], msMaterials[i].Diffuse[2], msMaterials[i].Diffuse[3]);
    fprintf(fp_out, "%s      %06lf;\r\n", Indent, msMaterials[i].Shininess);
    fprintf(fp_out, "%s      %06lf;%06lf;%06lf;%06lf;;\r\n", Indent, msMaterials[i].Specular[0], msMaterials[i].Specular[1], msMaterials[i].Specular[2]);
    fprintf(fp_out, "%s      %06lf;%06lf;%06lf;%06lf;;\r\n", Indent, msMaterials[i].Emissive[0], msMaterials[i].Emissive[1], msMaterials[i].Emissive[2]);

    // Write texture filename
    if(msMaterials[i].Texture[0]) {
      fprintf(fp_out, "%s      TextureFileName {\r\n", Indent);

      // Only write out local filenames
      char *Ptr = strrchr(msMaterials[i].Texture, '\\');
      if(!Ptr)
        Ptr = msMaterials[i].Texture;
      else
        Ptr++;
      fprintf(fp_out, "%s        %c%s%c;\r\n", Indent, '"', Ptr, '"');
      fprintf(fp_out, "%s      }\r\n", Indent);
    }

    fprintf(fp_out, "%s    }\r\n", Indent);
  }
  fprintf(fp_out, "%s  }\r\n", Indent);

  // Write mesh texture coordinates
  fprintf(fp_out, "%s  MeshTextureCoords {\r\n", Indent);
  fprintf(fp_out, "%s    %lu;\r\n", Indent, NumMeshVertices);
  for(i=0;i<NumMeshVertices;i++) {
    fprintf(fp_out, "%s    %06lf;%06lf;", Indent, Vertices[i].u, Vertices[i].v);
    if(i < (NumMeshVertices-1))
      fprintf(fp_out, ",\r\n");
    else
      fprintf(fp_out, ";\r\n");
  }
  fprintf(fp_out, "%s  }\r\n", Indent);

  // Count the number of bones actually used
  DWORD NumBonesUsed = 0;
  for(i=0;i<msNumBones;i++) {
    for(j=0;j<NumMeshVertices;j++) {
      if(Vertices[j].BoneNum == i) {
        NumBonesUsed++;
        break;
      }
    }
  }

  // Write bone data (if any)
  if(NumBonesUsed) {

    // Write skin weight header
    fprintf(fp_out, "%s  XSkinMeshHeader {\r\n", Indent);
    fprintf(fp_out, "%s    2;\r\n", Indent);
    fprintf(fp_out, "%s    4;\r\n", Indent);
    fprintf(fp_out, "%s    %lu;\r\n", Indent, NumBonesUsed);
    fprintf(fp_out, "%s  }\r\n", Indent);

    // Write out skin weights
    for(i=0;i<msNumBones;i++) {

      // Count number of vertices attached
      DWORD Count = 0;
      for(j=0;j<NumMeshVertices;j++) {
        if(Vertices[j].BoneNum == i)
          Count++;
      }

      if(Count) {

        fprintf(fp_out, "%s  SkinWeights {\r\n", Indent);
        fprintf(fp_out, "%s    %c%s%c;\r\n", Indent, '"', Bones[i].Name, '"');
        fprintf(fp_out, "%s    %lu;\r\n", Indent, Count);
      
        // Write vertex indices
        DWORD Num=0;
        for(j=0;j<NumMeshVertices;j++) {
          if(Vertices[j].BoneNum == i) {
            fprintf(fp_out, "%s    %lu", Indent, j);
            if(Num < (Count-1))
              fprintf(fp_out, ",\r\n");
            else 
              fprintf(fp_out, ";\r\n");
            Num++;
          }
        }

        // Write weights
        for(j=0;j<Count;j++) {
          fprintf(fp_out, "%s    1.000000", Indent);
          if(j < (Count-1))
            fprintf(fp_out, ",\r\n");
          else 
            fprintf(fp_out, ";\r\n");
        }

        // Write inverse bone matrix
        fprintf(fp_out, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  Indent, Bones[i].matInvCombined._11, Bones[i].matInvCombined._12, Bones[i].matInvCombined._13, Bones[i].matInvCombined._14);
        fprintf(fp_out, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  Indent, Bones[i].matInvCombined._21, Bones[i].matInvCombined._22, Bones[i].matInvCombined._23, Bones[i].matInvCombined._24);
        fprintf(fp_out, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  Indent, Bones[i].matInvCombined._31, Bones[i].matInvCombined._32, Bones[i].matInvCombined._33, Bones[i].matInvCombined._34);
        fprintf(fp_out, "%s    %06lf,%06lf,%06lf,%06lf;;\r\n", Indent, Bones[i].matInvCombined._41, Bones[i].matInvCombined._42, Bones[i].matInvCombined._43, Bones[i].matInvCombined._44);

        // Clear data object
        fprintf(fp_out, "%s  }\r\n", Indent);
      }
    }
  }

  // Close mesh data object
  fprintf(fp_out, "%s}\r\n", Indent);

  // Close frame hierarchy (if any)
  if(msNumBones)
    fprintf(fp_out, "}\r\n");

  // Output keyframes (if any)
  if(msTotalFrames) {

    // Add a space to seperate frame/mesh data from animation data
    fprintf(fp_out, "\r\n");

    // Output animation set data object 
    fprintf(fp_out, "AnimationSet %s_Anim {\r\n", MeshName);

    // Go through each bone and look for ones that have keys
    for(i=0;i<msNumBones;i++) {
      
      // Write animation data object is bone has keyframes
      if(msBones[i].Header.NumRotFrames || msBones[i].Header.NumPosFrames) {

        // Write animation data object header
        fprintf(fp_out, "  Animation %s_Anim {\r\n", Bones[i].Name);
        fprintf(fp_out, "    {%s}\r\n", Bones[i].Name);

        // Write keyframes
        fprintf(fp_out, "    AnimationKey {\r\n");
        fprintf(fp_out, "      4;\r\n");
        fprintf(fp_out, "      %lu;\r\n", msBones[i].Header.NumRotFrames);

        for(j=0;j<msBones[i].Header.NumPosFrames;j++) {
          fprintf(fp_out, "      %lu;16;\r\n", (DWORD)(msAnimationFPS * msBones[i].RotKeyFrames[j].Time)-1);

          // Build a transformation matrix based off values
          D3DXMATRIX matAnim, matRotation, matTranslation;
          CreateYawPitchRollRotationMatrix(&matRotation,
                                           msBones[i].RotKeyFrames[j].Value[1],
                                           msBones[i].RotKeyFrames[j].Value[0],
                                           -msBones[i].RotKeyFrames[j].Value[2]);
          D3DXMatrixTranslation(&matTranslation,
                                msBones[i].PosKeyFrames[j].Value[0],
                                msBones[i].PosKeyFrames[j].Value[1],
                                -msBones[i].PosKeyFrames[j].Value[2]);
          matAnim = matRotation * matTranslation;

          // Combine animation transformation w/bone transformation
          matAnim *= Bones[i].matTransformation;

          // Write transformation matrix
          fprintf(fp_out, "      %06lf,%06lf,%06lf,%06lf,\r\n",  matAnim._11, matAnim._12, matAnim._13, matAnim._14);
          fprintf(fp_out, "      %06lf,%06lf,%06lf,%06lf,\r\n",  matAnim._21, matAnim._22, matAnim._23, matAnim._24);
          fprintf(fp_out, "      %06lf,%06lf,%06lf,%06lf,\r\n",  matAnim._31, matAnim._32, matAnim._33, matAnim._34);
          fprintf(fp_out, "      %06lf,%06lf,%06lf,%06lf;;\r\n", matAnim._41, matAnim._42, matAnim._43, matAnim._44);
        }

        // Close keyframe data object
        fprintf(fp_out, "    }\r\n");
      }

      // Close animation data object
      fprintf(fp_out, "  }\r\n");
    }

    // Close animation set data object
    fprintf(fp_out, "}\r\n");
  }

  // Close files
  fclose(fp_out);
  fclose(fp_in);

  // Release all used data
  delete [] Vertices;
  delete [] Faces;
  delete [] msVertices;
  delete [] msFaces;
  delete [] msGroups;
  delete [] msMaterials;
  delete [] msBones;

  // Return success
  return TRUE;
}

void WriteFrame(sMS3DBoneContainer *Bone, FILE *fp, DWORD Indent, BOOL Close)
{
  char IndentText[256];
  
  // Write siblings
  if(Bone->Sibling)
    WriteFrame(Bone->Sibling, fp, Indent, TRUE);

  // Create some bit of text to indent frame information
  memset(IndentText, 32, 256);
  IndentText[Indent] = 0;

  // Write frame data object
  fprintf(fp, "%sFrame %s {\r\n", IndentText, Bone->Name);
  fprintf(fp, "%s  FrameTransformMatrix {\r\n", IndentText);
  fprintf(fp, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  IndentText, Bone->matTransformation._11, Bone->matTransformation._12, Bone->matTransformation._13, Bone->matTransformation._14);
  fprintf(fp, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  IndentText, Bone->matTransformation._21, Bone->matTransformation._22, Bone->matTransformation._23, Bone->matTransformation._24);
  fprintf(fp, "%s    %06lf,%06lf,%06lf,%06lf,\r\n",  IndentText, Bone->matTransformation._31, Bone->matTransformation._32, Bone->matTransformation._33, Bone->matTransformation._34);
  fprintf(fp, "%s    %06lf,%06lf,%06lf,%06lf;;\r\n", IndentText, Bone->matTransformation._41, Bone->matTransformation._42, Bone->matTransformation._43, Bone->matTransformation._44);
  fprintf(fp, "%s  }\r\n", IndentText);

  // Write child objects
  if(Bone->Child)
    WriteFrame(Bone->Child, fp, Indent+2, TRUE);

  // Close data template (only if not first frame)
  if(Close == TRUE)
    fprintf(fp, "%s}\r\n", IndentText);
}

void CombineTransformation(sMS3DBoneContainer *Bone, D3DXMATRIX *Matrix)
{
  // Combine matrix
  Bone->matCombined = Bone->matTransformation * (*Matrix);

  // Compute inverse combined matrix
  D3DXMatrixInverse(&Bone->matInvCombined, NULL, &Bone->matCombined);

  // Combine child
  if(Bone->Child)
    CombineTransformation(Bone->Child, &Bone->matCombined);

  // Combine sibling
  if(Bone->Sibling)
    CombineTransformation(Bone->Sibling, Matrix);
}

void CreateYawPitchRollRotationMatrix(D3DXMATRIX *matRotation,
                                      float Yaw,
                                      float Pitch,
                                      float Roll)
{
  D3DXMATRIX matX, matY, matZ;

  D3DXMatrixRotationX(&matX, -Pitch);
  D3DXMatrixRotationY(&matY, -Yaw);
  D3DXMatrixRotationZ(&matZ, -Roll);

  (*matRotation) = matX * matY * matZ;
}
