#include "Direct3D.h"

///////////////////////////////////////////////////////////
//
// Initialize Direct3D function
//
///////////////////////////////////////////////////////////
HRESULT InitD3D(IDirect3D9 **ppD3D, 
                IDirect3DDevice9 **ppD3DDevice,
                HWND hWnd, 
                BOOL ForceWindowed,
                BOOL MultiThreaded)
{
  IDirect3D9       *pD3D       = NULL;
  IDirect3DDevice9 *pD3DDevice = NULL;
  HRESULT           hr;

  // Error checking
  if(!ppD3D || !ppD3DDevice || !hWnd)
    return E_FAIL;

  // Initialize Direct3D
  if((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    return E_FAIL;
  *ppD3D = pD3D;

  // Ask if user wants to run windowed or fullscreen
  // or force windowed if flagged to do such
  int Mode;
  if(ForceWindowed == TRUE)
    Mode = IDNO;
  else
    Mode = MessageBox(hWnd, "Use fullscreen mode? (640x480x16)", "Initialize D3D", MB_YESNO | MB_ICONQUESTION);

  // Set the video (depending on windowed mode or fullscreen)
  D3DPRESENT_PARAMETERS d3dpp;
  ZeroMemory(&d3dpp, sizeof(d3dpp));

  // Setup video settings based on choice of fullscreen or not
  if(Mode == IDYES) {

    //////////////////////////////////////////////////////////
    // Setup fullscreen format (set to your own if you prefer)
    //////////////////////////////////////////////////////////
    DWORD     Width  = 640;
    DWORD     Height = 480;
    D3DFORMAT Format = D3DFMT_R5G6B5;

    // Set the presentation parameters (use fullscreen)
    d3dpp.BackBufferWidth  = Width;
    d3dpp.BackBufferHeight = Height;
    d3dpp.BackBufferFormat = Format;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_FLIP;
    d3dpp.Windowed         = FALSE;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_DEFAULT;
  } else {

    //////////////////////////////////////////////////////////
    // Setup windowed format (set to your own dimensions below)
    //////////////////////////////////////////////////////////

    // Get the client and window dimensions
    RECT ClientRect, WndRect;
    GetClientRect(hWnd, &ClientRect);
    GetWindowRect(hWnd, &WndRect);

    // Set the width and height (set your dimensions here)
    DWORD DesiredWidth  = 640;
    DWORD DesiredHeight = 480;
    DWORD Width  = (WndRect.right - WndRect.left) + (DesiredWidth  - ClientRect.right);
    DWORD Height = (WndRect.bottom - WndRect.top) + (DesiredHeight - ClientRect.bottom);

    // Set the window's dimensions
    MoveWindow(hWnd, WndRect.left, WndRect.top, Width, Height, TRUE);

    // Get the desktop format
    D3DDISPLAYMODE d3ddm;
    pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    // Set the presentation parameters (use windowed)
    d3dpp.BackBufferWidth  = DesiredWidth;
    d3dpp.BackBufferHeight = DesiredHeight;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_DEFAULT;
  }

  // Create the 3-D device
  DWORD Flags= D3DCREATE_MIXED_VERTEXPROCESSING;
//  DWORD Flags= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  if(MultiThreaded == TRUE)
    Flags |= D3DCREATE_MULTITHREADED;
  if(FAILED(hr = pD3D->CreateDevice(
                       D3DADAPTER_DEFAULT, 
                       D3DDEVTYPE_HAL, hWnd, Flags, 
                       &d3dpp, &pD3DDevice)))
      return hr;

  // Store the 3-D device object pointer
  *ppD3DDevice = pD3DDevice;

  // Set the perspective projection
  float Aspect = (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight;
  D3DXMATRIX matProjection;
  D3DXMatrixPerspectiveFovLH(&matProjection, D3DX_PI/4.0f, Aspect, 1.0f, 10000.0f);
  pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProjection);

  // Set the default render states
  pD3DDevice->SetRenderState(D3DRS_LIGHTING,         FALSE);
  pD3DDevice->SetRenderState(D3DRS_ZENABLE,          D3DZB_TRUE);
  pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE,  FALSE);

  // Set the default texture stage states
  pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);

  // Set the default texture filters
  pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

  return S_OK;
}


///////////////////////////////////////////////////////////
//
// Load a vertex shader function
//
///////////////////////////////////////////////////////////
HRESULT LoadVertexShader(IDirect3DVertexShader9 **ppShader, 
                         IDirect3DDevice9 *pDevice,
                         char *Filename,
                         D3DVERTEXELEMENT9 *pElements,
                         IDirect3DVertexDeclaration9 **ppDecl)
{
  HRESULT hr;

  // Error checking
  if(!ppShader || !pDevice || !Filename)
    return E_FAIL;

  // Load and assemble the shader
  ID3DXBuffer *pCode;
  if(FAILED(hr=D3DXAssembleShaderFromFile(Filename, NULL, NULL, 0, &pCode, NULL)))
    return hr;
  if(FAILED(hr=pDevice->CreateVertexShader((DWORD*)pCode->GetBufferPointer(), ppShader)))
    return hr;
  pCode->Release();

  // Create the declaration interface if needed
  if(pElements && ppDecl)
    pDevice->CreateVertexDeclaration(pElements, ppDecl);

  // Return success
  return S_OK;
}


///////////////////////////////////////////////////////////
//
// Load mesh functions
//
///////////////////////////////////////////////////////////
HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 IDirect3DDevice9 *pDevice,
                 char *Filename,
                 char *TexturePath,
                 DWORD NewFVF,
                 DWORD LoadFlags)
{
  ID3DXMesh *pLoadMesh = NULL;
  HRESULT hr;

  // Error checking
  if(!ppMesh || !pDevice || !Filename || !TexturePath)
    return E_FAIL;

  // Use system memory if converting FVF
  DWORD TempLoadFlags = LoadFlags;
  if(NewFVF)
    TempLoadFlags = D3DXMESH_SYSTEMMEM;

  // Load the mesh using D3DX routines
  ID3DXBuffer *MaterialBuffer = NULL, *AdjacencyBuffer = NULL;
  DWORD NumMaterials;
  if(FAILED(hr=D3DXLoadMeshFromX(Filename, TempLoadFlags,
                                 pDevice, &AdjacencyBuffer,
                                 &MaterialBuffer, NULL,
                                 &NumMaterials, &pLoadMesh)))
    return hr;

  // Convert to new FVF first as needed
  if(NewFVF) {
    ID3DXMesh *pTempMesh;

    // Use CloneMeshFVF to convert mesh
    if(FAILED(hr=pLoadMesh->CloneMeshFVF(LoadFlags, NewFVF, pDevice, &pTempMesh))) {
      ReleaseCOM(AdjacencyBuffer);
      ReleaseCOM(MaterialBuffer);
      ReleaseCOM(pLoadMesh);
      return hr;
    }

    // Free prior mesh and store new pointer
    ReleaseCOM(pLoadMesh);
    pLoadMesh = pTempMesh; pTempMesh = NULL;
  }

  // Allocate a D3DXMESHCONTAINER_EX structure
  D3DXMESHCONTAINER_EX *pMesh = new D3DXMESHCONTAINER_EX();
  *ppMesh = pMesh;

  // Store mesh name (filename), type, and mesh pointer
  pMesh->Name = strdup(Filename);
  pMesh->MeshData.Type = D3DXMESHTYPE_MESH;
  pMesh->MeshData.pMesh = pLoadMesh; pLoadMesh = NULL;

  // Store adjacency buffer
  DWORD AdjSize = AdjacencyBuffer->GetBufferSize();
  if(AdjSize) {
    pMesh->pAdjacency = new DWORD[AdjSize];
    memcpy(pMesh->pAdjacency, AdjacencyBuffer->GetBufferPointer(), AdjSize);
  }
  ReleaseCOM(AdjacencyBuffer);

  // Build material list
  if(!(pMesh->NumMaterials = NumMaterials)) {
    
    // Create a default material
    pMesh->NumMaterials = 1;
    pMesh->pMaterials   = new D3DXMATERIAL[1];
    pMesh->pTextures    = new IDirect3DTexture9*[1];

    ZeroMemory(pMesh->pMaterials, sizeof(D3DXMATERIAL));
    pMesh->pMaterials[0].MatD3D.Diffuse.r = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.g = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.b = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.a = 1.0f;
    pMesh->pMaterials[0].MatD3D.Ambient   = pMesh->pMaterials[0].MatD3D.Diffuse;
    pMesh->pMaterials[0].MatD3D.Specular  = pMesh->pMaterials[0].MatD3D.Diffuse;
    pMesh->pMaterials[0].pTextureFilename = NULL;
    pMesh->pTextures[0]                   = NULL;

  } else {

    // Load the materials
    D3DXMATERIAL *Materials = (D3DXMATERIAL*)MaterialBuffer->GetBufferPointer();
    pMesh->pMaterials = new D3DXMATERIAL[pMesh->NumMaterials];
    pMesh->pTextures  = new IDirect3DTexture9*[pMesh->NumMaterials];

    for(DWORD i=0;i<pMesh->NumMaterials;i++) {
      pMesh->pMaterials[i].MatD3D = Materials[i].MatD3D;
      pMesh->pMaterials[i].MatD3D.Ambient = pMesh->pMaterials[i].MatD3D.Diffuse;

      // Load the texture if one exists
      pMesh->pTextures[i] = NULL;
      if(Materials[i].pTextureFilename) {
        char TextureFile[MAX_PATH];
        sprintf(TextureFile, "%s%s", TexturePath, 
                             Materials[i].pTextureFilename);
        D3DXCreateTextureFromFile(pDevice,
                                  TextureFile,
                                  &pMesh->pTextures[i]);
      }
    }
  }
  ReleaseCOM(MaterialBuffer);

  // Optimize the mesh for better attribute access
  pMesh->MeshData.pMesh->OptimizeInplace(D3DXMESHOPT_ATTRSORT, NULL, NULL, NULL, NULL);

  // Clear pMesh pointer just in case
  pMesh = NULL;

  return S_OK;
}

HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 IDirect3DDevice9 *pDevice,
                 IDirectXFileData *pDataObj,
                 char *TexturePath,
                 DWORD NewFVF,
                 DWORD LoadFlags)
{
  ID3DXMesh *pLoadMesh = NULL;
  ID3DXSkinInfo *pSkin = NULL;
  HRESULT hr;

  // Error checking
  if(!ppMesh || !pDevice || !pDataObj || !TexturePath)
    return E_FAIL;

  // Use system memory if converting FVF
  DWORD TempLoadFlags = LoadFlags;
  if(NewFVF)
    TempLoadFlags = D3DXMESH_SYSTEMMEM;

  // Load the mesh using the D3DX skinned mesh interface
  ID3DXBuffer *MaterialBuffer = NULL, *AdjacencyBuffer = NULL;
  DWORD NumMaterials;
  if(FAILED(hr=D3DXLoadSkinMeshFromXof(pDataObj, TempLoadFlags,
                                       pDevice, &AdjacencyBuffer,
                                       &MaterialBuffer, NULL,
                                       &NumMaterials, &pSkin,
                                       &pLoadMesh)))
    return hr;

  // Free skin info if no bones
  if(pSkin && !pSkin->GetNumBones())
    ReleaseCOM(pSkin);

  // Convert to new FVF first as needed (not w/skinned models)
  if(NewFVF) {
    ID3DXMesh *pTempMesh = NULL;

    // Use CloneMeshFVF to convert mesh
    if(FAILED(hr=pLoadMesh->CloneMeshFVF(LoadFlags, NewFVF, pDevice, &pTempMesh))) {
      ReleaseCOM(pLoadMesh);
      ReleaseCOM(pSkin);
      ReleaseCOM(MaterialBuffer);
      ReleaseCOM(AdjacencyBuffer);
      return hr;
    }

    // Free prior mesh and store new pointer
    ReleaseCOM(pLoadMesh);
    pLoadMesh = pTempMesh; pTempMesh = NULL;
  }
 
  // Allocate a D3DXMESHCONTAINER_EX structure
  D3DXMESHCONTAINER_EX *pMesh = new D3DXMESHCONTAINER_EX();
  *ppMesh = pMesh;

  // Store mesh template name, type, and mesh pointers
  DWORD Size;
  pDataObj->GetName(NULL, &Size);
  if(Size) {
    pMesh->Name = new char[Size];
    pDataObj->GetName(pMesh->Name, &Size);
  }
  pMesh->MeshData.Type = D3DXMESHTYPE_MESH;
  pMesh->MeshData.pMesh = pLoadMesh; pLoadMesh = NULL;
  pMesh->pSkinInfo = pSkin; pSkin = NULL;

  // Store adjacency buffer
  DWORD AdjSize = AdjacencyBuffer->GetBufferSize();
  if(AdjSize) {
    pMesh->pAdjacency = (DWORD*)new char[AdjSize];
    memcpy(pMesh->pAdjacency, AdjacencyBuffer->GetBufferPointer(), AdjSize);
  }
  ReleaseCOM(AdjacencyBuffer);

  // Create a duplicate mesh in case skinning is used
  if(pMesh->pSkinInfo)
    pMesh->MeshData.pMesh->CloneMeshFVF(0, //D3DXMESH_MANAGED, 
                                        pMesh->MeshData.pMesh->GetFVF(), 
                                        pDevice, &pMesh->pSkinMesh);

  // Build material list
  if(!(pMesh->NumMaterials = NumMaterials)) {
    
    // Create a default material
    pMesh->NumMaterials = 1;
    pMesh->pMaterials   = new D3DXMATERIAL[1];
    pMesh->pTextures    = new IDirect3DTexture9*[1];

    ZeroMemory(&pMesh->pMaterials[0], sizeof(D3DXMATERIAL));
    pMesh->pMaterials[0].MatD3D.Diffuse.r = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.g = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.b = 1.0f;
    pMesh->pMaterials[0].MatD3D.Diffuse.a = 1.0f;
    pMesh->pMaterials[0].MatD3D.Ambient   = pMesh->pMaterials[0].MatD3D.Diffuse;
    pMesh->pMaterials[0].MatD3D.Specular  = pMesh->pMaterials[0].MatD3D.Diffuse;
    pMesh->pMaterials[0].pTextureFilename = NULL;
    pMesh->pTextures[0]                   = NULL;

  } else {

    // Load the materials
    D3DXMATERIAL *Materials = (D3DXMATERIAL*)MaterialBuffer->GetBufferPointer();
    pMesh->pMaterials = new D3DXMATERIAL[pMesh->NumMaterials];
    pMesh->pTextures  = new IDirect3DTexture9*[pMesh->NumMaterials];

    for(DWORD i=0;i<pMesh->NumMaterials;i++) {
      pMesh->pMaterials[i].MatD3D = Materials[i].MatD3D;
      pMesh->pMaterials[i].MatD3D.Ambient = pMesh->pMaterials[i].MatD3D.Diffuse;

      // Load the texture if one exists
      pMesh->pTextures[i] = NULL;
      if(Materials[i].pTextureFilename) {
        char TextureFile[MAX_PATH];
        sprintf(TextureFile, "%s%s", TexturePath, 
                             Materials[i].pTextureFilename);
        D3DXCreateTextureFromFile(pDevice,
                                  TextureFile,
                                  &pMesh->pTextures[i]);
      }
    }
  }
  ReleaseCOM(MaterialBuffer);

  // Optimize the mesh for better attribute access
  pMesh->MeshData.pMesh->OptimizeInplace(D3DXMESHOPT_ATTRSORT, NULL, NULL, NULL, NULL);

  // Clear pMesh pointer just in case
  pMesh = NULL;

  return S_OK;
}

HRESULT LoadMesh(D3DXMESHCONTAINER_EX **ppMesh,
                 D3DXFRAME_EX **ppFrame,
                 IDirect3DDevice9 *pDevice,
                 char *Filename,
                 char *TexturePath,
                 DWORD NewFVF,
                 DWORD LoadFlags)
{
  cXInternalParser Parser;

  // Error checking
  if(!pDevice || !Filename || !TexturePath)
    return E_FAIL;

  // Set parser data
  Parser.m_pD3DDevice  = pDevice;
  Parser.m_TexturePath = TexturePath;
  Parser.m_NewFVF      = NewFVF;
  Parser.m_LoadFlags   = LoadFlags;
  Parser.m_Flags       = ((!ppMesh)?0:1) | ((!ppFrame)?0:2);

  // Clear mesh and frame pointers
  Parser.m_RootFrame   = NULL;
  Parser.m_RootMesh    = NULL;

  // Parse the file
  Parser.Parse(Filename);

  // Map the matrices to the frames and create an array of bone 
  // matrices, but only if user passed pointers to receive and 
  // the loader found some meshes and frames.
  if(ppMesh && ppFrame && Parser.m_RootMesh && Parser.m_RootFrame) {

    // Scan through all meshes
    D3DXMESHCONTAINER_EX *pMesh = Parser.m_RootMesh;
    while(pMesh) {

      // Does this mesh use skinning?
      if(pMesh->pSkinInfo) {

        // Get the number of bones
        DWORD NumBones = pMesh->pSkinInfo->GetNumBones();

        // Allocate the matrix pointers and bone matrices
        pMesh->ppFrameMatrices = new D3DXMATRIX*[NumBones];
        pMesh->pBoneMatrices   = new D3DXMATRIX[NumBones];

        // Match matrix pointers to frames
        for(DWORD i=0;i<NumBones;i++) {

          // Get bone name
          const char *BoneName = pMesh->pSkinInfo->GetBoneName(i);

          // Find matching name in frames
          D3DXFRAME_EX *pFrame = Parser.m_RootFrame->Find(BoneName);

          // Match frame to bone
          if(pFrame)
            pMesh->ppFrameMatrices[i] = &pFrame->matCombined;
          else
            pMesh->ppFrameMatrices[i] = NULL;
        }
      }

      // Go to next mesh
      pMesh = (D3DXMESHCONTAINER_EX*)pMesh->pNextMeshContainer;
    } 
  }

  // Copy the pointers into passed variables
  if(ppMesh) {
    // Assign mesh list pointer
    *ppMesh = Parser.m_RootMesh;
    Parser.m_RootMesh = NULL;
  } else {
    // Delete list of meshes in case any were loaded
    // and were not needed.
    delete Parser.m_RootMesh;
    Parser.m_RootMesh = NULL;
  }

  if(ppFrame) {
    // Assign frame hierarchy pointer
    *ppFrame = Parser.m_RootFrame;
    Parser.m_RootFrame = NULL;
  } else {
    // Delete frame hierarchy in case it was loaded
    // and it was not needed.
    delete Parser.m_RootFrame;
    Parser.m_RootFrame = NULL;
  }

  return S_OK;
}


///////////////////////////////////////////////////////////
//
// Update a skinned mesh
//
///////////////////////////////////////////////////////////
HRESULT UpdateMesh(D3DXMESHCONTAINER_EX *pMesh)
{
  // Error checking
  if(!pMesh)
    return E_FAIL;
  if(!pMesh->MeshData.pMesh || !pMesh->pSkinMesh || !pMesh->pSkinInfo)
    return E_FAIL;
  if(!pMesh->pBoneMatrices || !pMesh->ppFrameMatrices)
    return E_FAIL;

  // Copy the bone matrices over (must have been combined before call DrawMesh)
  for(DWORD i=0;i<pMesh->pSkinInfo->GetNumBones();i++) {

    // Start with bone offset matrix
    pMesh->pBoneMatrices[i] = (*pMesh->pSkinInfo->GetBoneOffsetMatrix(i));

    // Apply frame transformation
    if(pMesh->ppFrameMatrices[i])
      pMesh->pBoneMatrices[i] *= (*pMesh->ppFrameMatrices[i]);
  }

  // Lock the meshes' vertex buffers
  void *SrcPtr, *DestPtr;
  pMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&SrcPtr);
  pMesh->pSkinMesh->LockVertexBuffer(0, (void**)&DestPtr);

  // Update the skinned mesh using provided transformations
  pMesh->pSkinInfo->UpdateSkinnedMesh(pMesh->pBoneMatrices, NULL, SrcPtr, DestPtr);

  // Unlock the meshes vertex buffers
  pMesh->pSkinMesh->UnlockVertexBuffer();
  pMesh->MeshData.pMesh->UnlockVertexBuffer();

  // Return success
  return S_OK;
}


///////////////////////////////////////////////////////////
//
// Draw mesh functions
//
///////////////////////////////////////////////////////////
HRESULT DrawMesh(D3DXMESHCONTAINER_EX *pMesh)
{
  IDirect3DDevice9 *pD3DDevice;
  DWORD LastState, OldAlphaState, OldSrcBlend, OldDestBlend;

  // Error checking
  if(!pMesh)
    return E_FAIL;
  if(!pMesh->MeshData.pMesh)
    return E_FAIL;
  if(!pMesh->NumMaterials || !pMesh->pMaterials)
    return E_FAIL;

  // Get the device interface
  pMesh->MeshData.pMesh->GetDevice(&pD3DDevice);

  // Release vertex shader if being used
  pD3DDevice->SetVertexShader(NULL);
  pD3DDevice->SetVertexDeclaration(NULL);

  // Save render states
  pD3DDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &OldAlphaState);
  pD3DDevice->GetRenderState(D3DRS_SRCBLEND, &OldSrcBlend);
  pD3DDevice->GetRenderState(D3DRS_DESTBLEND, &OldDestBlend);
  LastState = OldAlphaState;

  // Setup pointer for mesh to draw, either regular or skinned
  ID3DXMesh *pDrawMesh = (!pMesh->pSkinMesh)?pMesh->MeshData.pMesh:pMesh->pSkinMesh;

  // Look through all subsets
  for(DWORD i=0;i<pMesh->NumMaterials;i++) {

    // Set material and texture
    pD3DDevice->SetMaterial(&pMesh->pMaterials[i].MatD3D);
    pD3DDevice->SetTexture(0, pMesh->pTextures[i]);

    // Enable or disable alpha blending per material
    if(pMesh->pMaterials[i].MatD3D.Diffuse.a != 1.0f) {
      if(LastState != TRUE) {
        LastState = TRUE;
        pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);//SRCCOLOR);
        pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);
      }
    } else {
      if(LastState != FALSE) {
        LastState = FALSE;
        pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      }
    }

    // Draw the mesh subset
    pDrawMesh->DrawSubset(i);
  }

  // Restore alpha blending states
  if(LastState != OldAlphaState) {
    pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, OldAlphaState);
    pD3DDevice->SetRenderState(D3DRS_SRCBLEND, OldSrcBlend);
    pD3DDevice->SetRenderState(D3DRS_DESTBLEND, OldDestBlend);
  }

  // Make sure to release the device object!
  pD3DDevice->Release();

  // Return success
  return S_OK;
}

HRESULT DrawMesh(D3DXMESHCONTAINER_EX *pMesh,
                 IDirect3DVertexShader9 *pShader, 
                 IDirect3DVertexDeclaration9 *pDecl)
{
  IDirect3DDevice9 *pD3DDevice;
  DWORD LastState, OldAlphaState, OldSrcBlend, OldDestBlend;

  // Error checking
  if(!pMesh || !pShader || !pDecl)
    return E_FAIL;
  if(!pMesh->MeshData.pMesh)
    return E_FAIL;
  if(!pMesh->NumMaterials || !pMesh->pMaterials)
    return E_FAIL;

  // Get the device interface
  pMesh->MeshData.pMesh->GetDevice(&pD3DDevice);

  // Save render states
  pD3DDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &OldAlphaState);
  pD3DDevice->GetRenderState(D3DRS_SRCBLEND, &OldSrcBlend);
  pD3DDevice->GetRenderState(D3DRS_DESTBLEND, &OldDestBlend);
  LastState = OldAlphaState;

  // Get mesh buffer pointers
  IDirect3DVertexBuffer9 *pVB = NULL;
  IDirect3DIndexBuffer9 *pIB  = NULL;
  pMesh->MeshData.pMesh->GetVertexBuffer(&pVB);
  pMesh->MeshData.pMesh->GetIndexBuffer(&pIB);

  // Get attribute table
  DWORD NumAttributes;
  D3DXATTRIBUTERANGE *pAttributes = NULL;
  pMesh->MeshData.pMesh->GetAttributeTable(NULL, &NumAttributes);
  pAttributes = new D3DXATTRIBUTERANGE[NumAttributes];
  pMesh->MeshData.pMesh->GetAttributeTable(pAttributes, &NumAttributes);

  // Use the vertex shader interface passed
  pD3DDevice->SetFVF(NULL);
  pD3DDevice->SetVertexShader(pShader);
  pD3DDevice->SetVertexDeclaration(pDecl);

  // Set stream sources
  pD3DDevice->SetStreamSource(0, pVB, 0, D3DXGetFVFVertexSize(pMesh->MeshData.pMesh->GetFVF()));
  pD3DDevice->SetIndices(pIB);

  // Go through each attribute group and render
  for(DWORD i=0;i<NumAttributes;i++) {

    if(pAttributes[i].FaceCount) {

      // Get material number
      DWORD MatNum = pAttributes[i].AttribId;

      // Set texture
      pD3DDevice->SetTexture(0, pMesh->pTextures[MatNum]);

      // Enable or disable alpha blending per material
      if(pMesh->pMaterials[i].MatD3D.Diffuse.a != 1.0f) {
        if(LastState != TRUE) {
          LastState = TRUE;
          pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
          pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);//SRCCOLOR);
          pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);
        }
      } else {
        if(LastState != FALSE) {
          LastState = FALSE;
          pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        }
      }

      // Draw the mesh subset
      pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0,
                                       pAttributes[i].VertexStart,
                                       pAttributes[i].VertexCount,
                                       pAttributes[i].FaceStart * 3,
                                       pAttributes[i].FaceCount);
    }
  }

  // Clear stream uses
  pD3DDevice->SetStreamSource(0, NULL, 0, 0);
  pD3DDevice->SetIndices(NULL);

  // Free resources
  ReleaseCOM(pVB);
  ReleaseCOM(pIB);
  delete [] pAttributes;

  // Restore alpha blending states
  if(LastState != OldAlphaState) {
    pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, OldAlphaState);
    pD3DDevice->SetRenderState(D3DRS_SRCBLEND, OldSrcBlend);
    pD3DDevice->SetRenderState(D3DRS_DESTBLEND, OldDestBlend);
  }

  // Make sure to release the device object!
  pD3DDevice->Release();

  // Release vertex shader and declaration mapping
  pD3DDevice->SetVertexShader(NULL);
  pD3DDevice->SetVertexDeclaration(NULL);

  return S_OK;
}

HRESULT DrawMeshes(D3DXMESHCONTAINER_EX *pMesh)
{
  D3DXMESHCONTAINER_EX *MeshPtr = pMesh;

  // Loop through all meshes in list
  while(MeshPtr) {

    // Draw mesh, returning on error
    HRESULT hr = DrawMesh(MeshPtr);
    if(FAILED(hr))
      return hr;

    // Go to next mesh
    MeshPtr = (D3DXMESHCONTAINER_EX*)MeshPtr->pNextMeshContainer;
  }

  // Return success
  return S_OK;
}

HRESULT DrawMeshes(D3DXMESHCONTAINER_EX *pMesh,
                   IDirect3DVertexShader9 *pShader,
                   IDirect3DVertexDeclaration9 *pDecl)
{
  D3DXMESHCONTAINER_EX *MeshPtr = pMesh;

  // Loop through all meshes in list
  while(MeshPtr) {

    // Draw mesh, returning on error
    HRESULT hr = DrawMesh(MeshPtr, pShader, pDecl);
    if(FAILED(hr))
      return hr;

    // Go to next mesh
    MeshPtr = (D3DXMESHCONTAINER_EX*)MeshPtr->pNextMeshContainer;
  }

  // Return success
  return S_OK;
}

///////////////////////////////////////////////////////////
//
// Generic .X parser class code
//
///////////////////////////////////////////////////////////
cXInternalParser::cXInternalParser()
{
  m_pD3DDevice  = NULL;
  m_TexturePath = NULL;
  m_Flags       = 0;
  m_RootMesh    = NULL;
  m_RootFrame   = NULL;
}

cXInternalParser::~cXInternalParser()
{
  delete m_RootMesh;  m_RootMesh  = NULL;
  delete m_RootFrame; m_RootFrame = NULL;
}

BOOL cXInternalParser::Parse(char *Filename, void **Data)
{
  IDirectXFile           *pDXFile = NULL;
  IDirectXFileEnumObject *pDXEnum = NULL;
  IDirectXFileData       *pDXData = NULL;

  // Error checking
  if(Filename == NULL)
    return FALSE;

  // Create the file object
  if(FAILED(DirectXFileCreate(&pDXFile)))
    return FALSE;

  // Register the common templates
  if(FAILED(pDXFile->RegisterTemplates(                       \
                     (LPVOID)D3DRM_XTEMPLATES,                \
                     D3DRM_XTEMPLATE_BYTES))) {
    pDXFile->Release();
    return FALSE;
  }

  // Create an enumeration object
  if(FAILED(pDXFile->CreateEnumObject((LPVOID)Filename,       \
                                      DXFILELOAD_FROMFILE,    \
                                      &pDXEnum))) {
    pDXFile->Release();
    return FALSE;
  }
  
  // Loop through all top-level objects, breaking on errors
  BOOL ParseResult;
  while(SUCCEEDED(pDXEnum->GetNextDataObject(&pDXData))) {
    ParseResult = ParseObject(pDXData, NULL, 0, Data, FALSE);
    ReleaseCOM(pDXData);
    if(ParseResult == FALSE)
      break;
  }

  // Release used COM objects
  ReleaseCOM(pDXEnum);
  ReleaseCOM(pDXFile);

  return TRUE;
}

BOOL cXInternalParser::ParseObject(IDirectXFileData *pDataObj,
                                     IDirectXFileData *pParentDataObj,
                                     DWORD Depth,
                                     void **Data, BOOL Reference)
{ 
  const GUID *Type = GetObjectGUID(pDataObj);

  // Process templates based on their type

  // Build on to frame hierarchy (ony non-referenced frames)
  if(*Type == TID_D3DRMFrame && Reference == FALSE && m_Flags & 2) {

    // Allocate a frame
    D3DXFRAME_EX *pFrame = new D3DXFRAME_EX();

    // Get the frame's name (if any)
    pFrame->Name = GetObjectName(pDataObj);

    // Link frame into hierarchy
    if(Data == NULL) {
      // Link as sibling of root
      pFrame->pFrameSibling = m_RootFrame;
      m_RootFrame = pFrame; pFrame = NULL;
      Data = (void**)&m_RootFrame;
    } else {
      // Link as child of supplied frame
      D3DXFRAME_EX *pFramePtr = (D3DXFRAME_EX*)*Data;
      pFrame->pFrameSibling = pFramePtr->pFrameFirstChild;
      pFramePtr->pFrameFirstChild = pFrame; pFrame = NULL;
      Data = (void**)&pFramePtr->pFrameFirstChild;
    }
  }

  // Set a frame transformation matrix
  if(*Type == TID_D3DRMFrameTransformMatrix && Reference == FALSE && m_Flags & 2 && Data) {

    D3DXFRAME_EX *Frame = (D3DXFRAME_EX*)*Data;
    if(Frame) {
      Frame->TransformationMatrix = *(D3DXMATRIX*)GetObjectData(pDataObj, NULL);
      Frame->matOriginal = Frame->TransformationMatrix;
    }
  }

  // Load a mesh (skinned or regular)
  if(*Type == TID_D3DRMMesh && m_Flags & 1) {

    // Only load non-referenced skin meshes into memory
    if(Reference == FALSE) {
      
      // Load the mesh using the data object load method
      D3DXMESHCONTAINER_EX *pMesh = NULL;
      LoadMesh(&pMesh, m_pD3DDevice, pDataObj, m_TexturePath, m_NewFVF, m_LoadFlags);

      // Link mesh to head of list of meshes
      if(pMesh) {
        pMesh->pNextMeshContainer = m_RootMesh;
        m_RootMesh = pMesh; pMesh = NULL;

        // Link mesh to frame if needed
        if(Data) {
          D3DXFRAME_EX *pFrame = (D3DXFRAME_EX*)*Data;
          if(m_Flags & 2 && pFrame)
            pFrame->pMeshContainer = m_RootMesh;
        }
      }
    } else {

      // If referenced, then check if wanting to link to frame
      if(Data) {
        D3DXFRAME_EX *pFrame = (D3DXFRAME_EX*)*Data;
        if(m_Flags & 2 && m_RootMesh && pFrame) {

          // Get name of mesh reference to link to
          char *Name = GetObjectName(pDataObj);
          if(Name) {
            // Find matching mesh by name and store result
            pFrame->pMeshContainer = m_RootMesh->Find(Name);

            // Clear name
            delete [] Name; Name = NULL;
          }
        }
      }
    }
  }

  // Parse child templates
  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}

BOOL cXInternalParser::ParseChildObjects(IDirectXFileData *pDataObj,
                                           DWORD Depth, void **Data,
                                           BOOL ForceReference)
{
  IDirectXFileObject        *pSubObj  = NULL;
  IDirectXFileData          *pSubData = NULL;
  IDirectXFileDataReference *pDataRef = NULL;
  BOOL                       ParseResult = TRUE;

  // Scan for embedded templates
  while(SUCCEEDED(pDataObj->GetNextObject(&pSubObj))) {

    // Process embedded references
    if(SUCCEEDED(pSubObj->QueryInterface(
                              IID_IDirectXFileDataReference,
                              (void**)&pDataRef))) {

      // Resolve the data object
      if(SUCCEEDED(pDataRef->Resolve(&pSubData))) {

        // Parse the object, remembering the return code
        ParseResult = ParseObject(pSubData, pDataObj, Depth+1, Data, TRUE);
        ReleaseCOM(pSubData);
      }
      ReleaseCOM(pDataRef);

      // Return on parsing failure
      if(ParseResult == FALSE)
        return FALSE;
    } else

    // Process non-referenced embedded templates
    if(SUCCEEDED(pSubObj->QueryInterface(
                              IID_IDirectXFileData,
                              (void**)&pSubData))) {

      // Parse the object, remembering the return code
      ParseResult = ParseObject(pSubData, pDataObj, Depth+1, Data, ForceReference);
      ReleaseCOM(pSubData);
    }

    // Release the data object
    ReleaseCOM(pSubObj);

    // Return on parsing failure
    if(ParseResult == FALSE)
      return FALSE;
  }

  return TRUE;
}

const GUID *cXInternalParser::GetObjectGUID(IDirectXFileData *pDataObj)
{
  const GUID *Type = NULL;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get the template type
  if(FAILED(pDataObj->GetType(&Type)))
    return NULL;

  return Type;
}

char *cXInternalParser::GetObjectName(IDirectXFileData *pDataObj)
{
  char  *Name = NULL;
  DWORD  Size = 0;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get the template name (if any)
  if(FAILED(pDataObj->GetName(NULL, &Size)))
    return NULL;

  // Allocate a name buffer and retrieve name
  if(Size) {
    if((Name = new char[Size]) != NULL)
      pDataObj->GetName(Name, &Size);
  }

  return Name;
}

void *cXInternalParser::GetObjectData(IDirectXFileData *pDataObj,
                                DWORD *Size)
{
  void *TemplateData = NULL;
  DWORD TemplateSize = 0;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get a data pointer to template
  pDataObj->GetData(NULL, &TemplateSize, (PVOID*)&TemplateData);

  // Save size if needed
  if(Size != NULL)
    *Size = TemplateSize;

  return TemplateData;
}
