#include "Face.h"

// Vertex shader declaration and interfaces
D3DVERTEXELEMENT9 g_MorphBlendMeshDecl[] =
{
  // 1st stream is for base mesh
  { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
  { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
  { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
  // 2nd stream is for mesh 1
  { 1,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
  { 1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   1 },
  { 1, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
  // 3rd stream is for mesh 2
  { 2,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 2 },
  { 2, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   2 },
  { 2, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
  // 4th stream is for mesh 3
  { 3,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 3 },
  { 3, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   3 },
  { 3, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
  // 5th stream is for mesh 4
  { 4,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 4 },
  { 4, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   4 },
  { 4, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 },
  D3DDECL_END()
};
IDirect3DVertexShader9      *g_VS   = NULL;
IDirect3DVertexDeclaration9 *g_Decl = NULL;

// Phoneme parser GUID definitions
DEFINE_GUID(Phoneme, 
            0x12b0fb22, 0x4f25, 0x4adb, 
            0xba, 0x0, 0xe5, 0xb9, 0xc1, 0x8a, 0x83, 0x9d);

DEFINE_GUID(PhonemeSequence, 
            0x918dee50, 0x657c, 0x48b0, 
            0x94, 0xa5, 0x15, 0xec, 0x23, 0xe6, 0x3b, 0xc9);

HRESULT LoadBlendingShader(IDirect3DDevice9 *pDevice)
{
  return LoadVertexShader(&g_VS, pDevice, "MorphBlend.vsh", g_MorphBlendMeshDecl, &g_Decl);
}

void FreeBlendingShader()
{
  ReleaseCOM(g_VS);
  ReleaseCOM(g_Decl);
}

void DrawBlendedMesh(D3DXMESHCONTAINER_EX *BaseMesh,
                     D3DXMESHCONTAINER_EX *Mesh1, float Blend1,
                     D3DXMESHCONTAINER_EX *Mesh2, float Blend2,
                     D3DXMESHCONTAINER_EX *Mesh3, float Blend3,
                     D3DXMESHCONTAINER_EX *Mesh4, float Blend4)
{
  // Get the device pointer
  IDirect3DDevice9 *pD3DDevice;
  BaseMesh->MeshData.pMesh->GetDevice(&pD3DDevice);

  // Enable zbuffering
  pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  // Get the world, view, and projection matrices
  D3DXMATRIX matWorld, matView, matProj;
  pD3DDevice->GetTransform(D3DTS_WORLD, &matWorld);
  pD3DDevice->GetTransform(D3DTS_VIEW, &matView);
  pD3DDevice->GetTransform(D3DTS_PROJECTION, &matProj);

  // Get the world*view*proj matrix and set it
  D3DXMATRIX matWVP;
  matWVP = matWorld * matView * matProj;
  D3DXMatrixTranspose(&matWVP, &matWVP);
  pD3DDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

  // Set the scalar blending values to use
  pD3DDevice->SetVertexShaderConstantF(4, (float*)&D3DXVECTOR4(Blend1, Blend2, Blend3, Blend4), 1);

  // Set the light direction
  pD3DDevice->SetVertexShaderConstantF(5, (float*)&D3DXVECTOR4(0.0f, 0.0f, 1.0f, 0.0f), 1);

  // Get the meshes' vertex buffers
  IDirect3DVertexBuffer9 *pVB1 = NULL;
  IDirect3DVertexBuffer9 *pVB2 = NULL;
  IDirect3DVertexBuffer9 *pVB3 = NULL;
  IDirect3DVertexBuffer9 *pVB4 = NULL;
  Mesh1->MeshData.pMesh->GetVertexBuffer(&pVB1);
  Mesh2->MeshData.pMesh->GetVertexBuffer(&pVB2);
  Mesh3->MeshData.pMesh->GetVertexBuffer(&pVB3);
  Mesh4->MeshData.pMesh->GetVertexBuffer(&pVB4);

  // Set the stream sources
  pD3DDevice->SetStreamSource(1, pVB1, 0, D3DXGetFVFVertexSize(Mesh1->MeshData.pMesh->GetFVF()));
  pD3DDevice->SetStreamSource(2, pVB2, 0, D3DXGetFVFVertexSize(Mesh2->MeshData.pMesh->GetFVF()));
  pD3DDevice->SetStreamSource(3, pVB3, 0, D3DXGetFVFVertexSize(Mesh3->MeshData.pMesh->GetFVF()));
  pD3DDevice->SetStreamSource(4, pVB4, 0, D3DXGetFVFVertexSize(Mesh4->MeshData.pMesh->GetFVF()));

  // Draw the mesh in the vertex shader
  DrawMesh(BaseMesh, g_VS, g_Decl);

  // Clear the stream sources
  pD3DDevice->SetStreamSource(1, NULL, 0, 0);
  pD3DDevice->SetStreamSource(2, NULL, 0, 0);
  pD3DDevice->SetStreamSource(3, NULL, 0, 0);
  pD3DDevice->SetStreamSource(4, NULL, 0, 0);
  
  // Free the vertex buffer interface
  ReleaseCOM(pVB4);
  ReleaseCOM(pVB3);
  ReleaseCOM(pVB2);
  ReleaseCOM(pVB1);
}

cXPhonemeParser::cXPhonemeParser()
{ 
  m_Name        = NULL; 
  m_NumPhonemes = 0;
  m_Phonemes    = NULL;
  m_Length      = 0;
}

cXPhonemeParser::~cXPhonemeParser()
{
  Free();
}

void cXPhonemeParser::Free()
{
  delete [] m_Name; 
  delete [] m_Phonemes; 
  m_Name        = NULL;
  m_NumPhonemes = 0;
  m_Phonemes    = NULL;
  m_Length      = 0;
}

BOOL cXPhonemeParser::ParseObject(                          \
                   IDirectXFileData *pDataObj,                \
                   IDirectXFileData *pParentDataObj,          \
                   DWORD Depth,                               \
                   void **Data, BOOL Reference)
{
  const GUID *Type = GetObjectGUID(pDataObj);

  // Only process phoneme sequence templates
  if(*Type == PhonemeSequence) {

    // Free currently loaded sequence
    Free();

    // Get name and pointer to data
    m_Name = GetObjectName(pDataObj);
    DWORD *DataPtr = (DWORD*)GetObjectData(pDataObj, NULL);

    // Get # phonemes, allocate structures, and load data
    m_NumPhonemes = *DataPtr++;
    m_Phonemes = new sPhoneme[m_NumPhonemes];
    for(DWORD i=0;i<m_NumPhonemes;i++) {
      m_Phonemes[i].Code      = *DataPtr++;
      m_Phonemes[i].StartTime = *DataPtr++;
      m_Phonemes[i].EndTime   = *DataPtr++;
    }
    m_Length = m_Phonemes[m_NumPhonemes-1].EndTime + 1;
  }

  // Parse child templates
  return ParseChildObjects(pDataObj, Depth, Data);
}

DWORD cXPhonemeParser::FindPhoneme(DWORD Time)
{
  if(m_NumPhonemes) {
    // Search for time
    for(DWORD i=0;i<m_NumPhonemes;i++) {
      if(Time >= m_Phonemes[i].StartTime &&                   \
                               Time <= m_Phonemes[i].EndTime)
         return i;
    }
  }
  return 0;
}

void cXPhonemeParser::GetAnimData(                            \
              DWORD Time,                                     \
              DWORD *Phoneme1, float *Phoneme1Time,           \
              DWORD *Phoneme2, float *Phoneme2Time)
{
  // Quick check if past end of animation
  if(Time >= m_Length) {
    *Phoneme1 = m_Phonemes[m_NumPhonemes-1].Code;
    *Phoneme2 = 0;
    *Phoneme1Time = 1.0f;
    *Phoneme2Time = 0.0f;
    return;
  }

  // Find the key to use in the phoneme sequence
  DWORD Index1 = FindPhoneme(Time);
  DWORD Index2 = Index1+1;
  if(Index2 >= m_NumPhonemes)
    Index2 = Index1;

  // Set phoneme index #'s
  *Phoneme1 = m_Phonemes[Index1].Code;
  *Phoneme2 = m_Phonemes[Index2].Code;

  // Calculate timing values
  DWORD Time1 = m_Phonemes[Index1].StartTime;
  DWORD Time2 = m_Phonemes[Index1].EndTime;
  DWORD TimeDiff = Time2 - Time1;
  Time -= Time1;
  float TimeFactor = 1.0f / (float)TimeDiff;
  float Timing = (float)Time * TimeFactor;

  // Set phoneme times
  *Phoneme1Time = 1.0f - Timing;
  *Phoneme2Time = Timing;
}
