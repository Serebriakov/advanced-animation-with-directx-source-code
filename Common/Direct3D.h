#ifndef _DIRECT3D_H_
#define _DIRECT3D_H_

#include <stdio.h>
#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "dxfile.h"
#include "XFile.h"

#define ReleaseCOM(x) { if(x!=NULL) x->Release(); x=NULL; }

// Declare an extended version of D3DXFRAME
// that contains a constructor and destructor
// as well as a combined transformation matrix
struct D3DXFRAME_EX : D3DXFRAME 
{
  D3DXMATRIX matCombined;   // Combined matrix
  D3DXMATRIX matOriginal;   // Original transformation from .X
  
  D3DXFRAME_EX()
  {
    Name = NULL;
    pMeshContainer = NULL;
    pFrameSibling = pFrameFirstChild = NULL;
    D3DXMatrixIdentity(&matCombined);
    D3DXMatrixIdentity(&matOriginal);
    D3DXMatrixIdentity(&TransformationMatrix);
  }

  ~D3DXFRAME_EX()
  { 
    delete [] Name;          Name = NULL;
    delete pFrameSibling;    pFrameSibling = NULL;
    delete pFrameFirstChild; pFrameFirstChild = NULL;
  }

  // Function to scan hierarchy for matching frame name
  D3DXFRAME_EX *Find(const char *FrameName)
  {
    D3DXFRAME_EX *pFrame, *pFramePtr;

    // Return this frame instance if name matched
    if(Name && FrameName && !strcmp(FrameName, Name))
      return this;

    // Scan siblings
    if((pFramePtr = (D3DXFRAME_EX*)pFrameSibling)) {
      if((pFrame = pFramePtr->Find(FrameName)))
        return pFrame;
    }

    // Scan children
    if((pFramePtr = (D3DXFRAME_EX*)pFrameFirstChild)) {
      if((pFrame = pFramePtr->Find(FrameName)))
        return pFrame;
    }

    // Return none found
    return NULL;
  }

  // Reset transformation matrices to originals
  void Reset()
  {
    // Copy original matrix
    TransformationMatrix = matOriginal;

    // Reset sibling frames
    D3DXFRAME_EX *pFramePtr;
    if((pFramePtr = (D3DXFRAME_EX*)pFrameSibling))
      pFramePtr->Reset();

    // Reset child frames
    if((pFramePtr = (D3DXFRAME_EX*)pFrameFirstChild))
      pFramePtr->Reset();
  }

  // Function to combine matrices in frame hiearchy
  void UpdateHierarchy(D3DXMATRIX *matTransformation = NULL)
  {
    D3DXFRAME_EX *pFramePtr;
    D3DXMATRIX matIdentity;

    // Use an identity matrix if none passed
    if(!matTransformation) {
      D3DXMatrixIdentity(&matIdentity);
      matTransformation = &matIdentity;
    }

    // Combine matrices w/supplied transformation matrix
    matCombined = TransformationMatrix * (*matTransformation);

    // Combine w/sibling frames
    if((pFramePtr = (D3DXFRAME_EX*)pFrameSibling))
      pFramePtr->UpdateHierarchy(matTransformation);

    // Combine w/child frames
    if((pFramePtr = (D3DXFRAME_EX*)pFrameFirstChild))
      pFramePtr->UpdateHierarchy(&matCombined);
  }

  void Count(DWORD *Num)
  {
    // Error checking
    if(!Num)
      return;

    // Increase count of frames
    (*Num)+=1;

    // Process sibling frames
    D3DXFRAME_EX *pFrame;
    if((pFrame=(D3DXFRAME_EX*)pFrameSibling))
      pFrame->Count(Num);

    // Process child frames
    if((pFrame=(D3DXFRAME_EX*)pFrameFirstChild))
      pFrame->Count(Num);
  }
};

// Declare an extended version of D3DXMESHCONTAINER
// that contains a constructor and destructor
// as well as an array of textures, a mesh object
// that contains the generated skin mesh, and 
// matrices that map to the frame hierarchy's and
// for updating bones.
struct D3DXMESHCONTAINER_EX : D3DXMESHCONTAINER
{
  IDirect3DTexture9 **pTextures;
  ID3DXMesh          *pSkinMesh;

  D3DXMATRIX        **ppFrameMatrices;
  D3DXMATRIX         *pBoneMatrices;

  D3DXMESHCONTAINER_EX()
  {
    Name               = NULL;
    MeshData.pMesh     = NULL;
    pMaterials         = NULL;
    pEffects           = NULL;
    NumMaterials       = 0;
    pAdjacency         = NULL;
    pSkinInfo          = NULL;
    pNextMeshContainer = NULL;
    pTextures          = NULL;
    pSkinMesh          = NULL;
    ppFrameMatrices    = NULL;
    pBoneMatrices      = NULL;
  }

  ~D3DXMESHCONTAINER_EX()
  {
    if(pTextures && NumMaterials) {
      for(DWORD i=0;i<NumMaterials;i++)
        ReleaseCOM(pTextures[i]);
    }
    delete [] pTextures;       pTextures = NULL;
    NumMaterials = 0;

    delete [] Name;            Name = NULL;
    delete [] pMaterials;      pMaterials = NULL;
    delete pEffects;           pEffects = NULL;

    delete [] pAdjacency;      pAdjacency = NULL;
    delete [] ppFrameMatrices; ppFrameMatrices = NULL;
    delete [] pBoneMatrices;   pBoneMatrices = NULL;

    ReleaseCOM(MeshData.pMesh);
    ReleaseCOM(pSkinInfo);
    ReleaseCOM(pSkinMesh);

    delete pNextMeshContainer; pNextMeshContainer = NULL;
  }

  D3DXMESHCONTAINER_EX *Find(char *MeshName)
  {
    D3DXMESHCONTAINER_EX *pMesh, *pMeshPtr;

    // Return this mesh instance if name matched
    if(Name && MeshName && !strcmp(MeshName, Name))
      return this;

    // Scan next in list
    if((pMeshPtr = (D3DXMESHCONTAINER_EX*)pNextMeshContainer)) {
      if((pMesh = pMeshPtr->Find(MeshName)))
        return pMesh;
    }

    // Return none found
    return NULL;
  }
};

// Declare an internal .X file parser class for loading meshes and frames
class cXInternalParser
{
  public:
    // Information passed from calling function
    IDirect3DDevice9     *m_pD3DDevice;
    char                 *m_TexturePath;
    DWORD                 m_NewFVF;
    DWORD                 m_LoadFlags;

    // Flags for which data to load
    // 1 = mesh, 2 = frames, 3= both
    DWORD                 m_Flags;

    // Hierarchies used during loading
    D3DXMESHCONTAINER_EX *m_RootMesh;
    D3DXFRAME_EX         *m_RootFrame;

  protected:
    // Function called for every template found
    BOOL ParseObject(IDirectXFileData *pDataObj,
                       IDirectXFileData *pParentDataObj,
                       DWORD Depth,
                       void **Data, BOOL Reference);

    // Function called to enumerate child templates
    BOOL ParseChildObjects(IDirectXFileData *pDataObj,
                             DWORD Depth, void **Data,
                             BOOL ForceReference = FALSE);

  public:
    // Constructor and destructor
    cXInternalParser();
    ~cXInternalParser();

    // Function to start parsing an .X file
    BOOL Parse(char *Filename, void **Data = NULL);

    // Functions to help retrieve template information
    const GUID *GetObjectGUID(IDirectXFileData *pDataObj);
    char *GetObjectName(IDirectXFileData *pDataObj);
    void *GetObjectData(IDirectXFileData *pDataObj, DWORD *Size);
};


// Initialize D3D
HRESULT InitD3D(IDirect3D9 **ppD3D, 
                IDirect3DDevice9 **ppD3DDevice,
                HWND hWnd, BOOL ForceWindowed = FALSE,
                BOOL MultiThreaded = FALSE);

// Load a vertex shader
HRESULT LoadVertexShader(IDirect3DVertexShader9 **ppShader, 
                         IDirect3DDevice9 *pDevice,
                         char *Filename,
                         D3DVERTEXELEMENT9 *pElements = NULL,
                         IDirect3DVertexDeclaration9 **ppDecl = NULL);

// Load a single mesh from an .X file (compact multiple meshes into one)
HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 IDirect3DDevice9 *pDevice,
                 char *Filename,
                 char *TexturePath = ".\\",
                 DWORD NewFVF = 0,
                 DWORD LoadFlags = D3DXMESH_SYSTEMMEM);

// Load a single mesh (regular or skinned) from a mesh template
HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 IDirect3DDevice9 *pDevice,
                 IDirectXFileData *pDataObj,
                 char *TexturePath = ".\\",
                 DWORD NewFVF = 0,
                 DWORD LoadFlags = D3DXMESH_SYSTEMMEM);

// Load all meshes and frames from an .X file
HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 D3DXFRAME_EX **ppFrame,
                 IDirect3DDevice9 *pDevice,
                 char *Filename,
                 char *TexturePath = ".\\",
                 DWORD NewFVF = 0,
                 DWORD LoadFlags = D3DXMESH_SYSTEMMEM);

// Update a skinned mesh
HRESULT UpdateMesh(D3DXMESHCONTAINER_EX *pMesh);

// Draw the first mesh in a linked list of objects
HRESULT DrawMesh(D3DXMESHCONTAINER_EX *pMesh);

// Draw the first mesh in a linked list of objects
// using the specified vertex shader and declaration
HRESULT DrawMesh(D3DXMESHCONTAINER_EX *pMesh,
                 IDirect3DVertexShader9 *pShader,
                 IDirect3DVertexDeclaration9 *pDecl);

// Draw all meshes in a linked list of objects
HRESULT DrawMeshes(D3DXMESHCONTAINER_EX *pMesh);

// Draw all meshes in a linked list of objects
// using the specified vertex shader and declaration
HRESULT DrawMeshes(D3DXMESHCONTAINER_EX *pMesh,
                   IDirect3DVertexShader9 *pShader,
                   IDirect3DVertexDeclaration9 *pDecl);

#endif
