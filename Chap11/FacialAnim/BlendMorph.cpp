typedef struct {
  float x, y, z;
  float nz, ny, nz;
  float u, v;
} sMorphVertex;

#define MORPHFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

DWORD g_FacialMorphDecl[] =
{
    D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_FLOAT3),   // Position
    D3DVSD_REG(1, D3DVSDT_FLOAT3),   // Normal
    D3DVSD_REG(2, D3DVSDT_FLOAT2),   // Texture
    D3DVSD_STREAM(1),
    D3DVSD_REG(3, D3DVSDT_FLOAT3),   // Position
    D3DVSD_REG(4, D3DVSDT_FLOAT3),   // Normal
    D3DVSD_REG(5, D3DVSDT_FLOAT2),   // Texture
    D3DVSD_STREAM(2),
    D3DVSD_REG(6, D3DVSDT_FLOAT3),   // Position
    D3DVSD_REG(7, D3DVSDT_FLOAT3),   // Normal
    D3DVSD_REG(8, D3DVSDT_FLOAT2),   // Texture
    D3DVSD_STREAM(3),
    D3DVSD_REG(9, D3DVSDT_FLOAT3),   // Position
    D3DVSD_REG(10, D3DVSDT_FLOAT3),  // Normal
    D3DVSD_REG(11, D3DVSDT_FLOAT2),  // Texture
    D3DVSD_STREAM(4),
    D3DVSD_REG(12, D3DVSDT_FLOAT3),  // Position
    D3DVSD_REG(13, D3DVSDT_FLOAT3),  // Normal
    D3DVSD_REG(14, D3DVSDT_FLOAT2),  // Texture
    D3DVSD_END()
};

class cBlendedMorphAnimationKey
{
  public:
    char  *m_MeshName;

    DWORD  m_StartTime,  m_EndTime;
    float  m_StartBlend, m_EndBlend;

    void  *m_MeshPtr;

  public:
    cBlendedMorphAnimationKey();
    ~cBlendedMorphAnimationKey();
};
 
class cBlendedMorphAnimation
{
  public:
    char                      *m_Name;        // Animation name

    DWORD                      m_Loop;
    DWORD                      m_ChannelNum;

    DWORD                      m_Length;      // Length of animation

    DWORD                      m_NumKeys;
    cBlendedMorphAnimationKey *m_Keys;

  public:
    cBlendedMorphAnimation();
    ~cBlendedMorphAnimation();
};

class cBlendedMorphAnimationSet
{
  public:
    char                   *m_Name;           // Animation set name

    cBlendedMorphAnimation *m_BaseMesh;       // Base mesh keys

    DWORD                   m_NumAnimations;  // # animations in set
    cBlendedMorphAnimation *m_Animations;     // Animations

  public:
    cBlendedMorphAnimationSet();
    ~cBlendedMorphAnimationSet();

    void Render(DWORD Time);
    void RenderMesh(void *Ptr);
}

class cBlendMorphAnimation : cXParser
{
  protected:
    static cVertexShaderLoader m_VSLoader;
    static DWORD               m_VSCount;

    DWORD               m_NumMeshes;
    cMeshGroup          m_Meshes;

  protected:
    // Parse an .X file for animation data
    BOOL ParseObject(IDirectXFileData *pDataObj,
                       IDirectXFileData *pParentDataObj,
                       DWORD Depth,
                       void **Data, BOOL Reference);

  public:
    cBlendMorphAnimation();
    ~cBlendMorphAnimation();

    void Init(IDirect3DDevice8 *pDevice);
    void Shutdown();

    BOOL LoadMesh(DWORD ID, char *Name,
                  IDirect3DDevice8 *pDevice,
                  char *Filename,
                  char *TexturePath=".\\",
                  DWORD NewFVF=0, 
                  DWORD LoadFlags = D3DXMESH_SYSTEM);
    void FreeMesh(DWORD ID);
    void FreeMesh(char *Name);
    void FreeAllMeshes();

    BOOL LoadAnimation(char *Filename);
    void FreeAnimation();

    BOOL  LoadSound(char *Filename);
    void  FreeSound();
    void  PlaySound();
    void  StopSound();
    DWORD GetSoundPos();

    void Render(DWORD Time);
};

/*
    // DirectShow interfaces for playing back voice track
    IGraphBuilder      *m_Graph;
    IMediaControl      *m_MediaControl;
    IMediaPosition     *m_MediaPosition;


cBlendedMorphAnimationSet::Render(DWORD Time)
{
  // Go through all animations and render those between time
  for(DWORD i=0;i<m_NumAnimations;i++) {

    // Get a pointer to animation
    cBlendedMorphAnimation *Anim = m_Animations[i];

    // Bounds check time to length of animation
    DWORD KeyTime = Time % Anim->m_Length;

    // Go through each animation and examine keys
    for(DWORD j=0;j<Anim->m_NumKeys;j++) {

      // Get a pointer to key
      cBlendedMorphAnimationKey *Key = Anim->m_Keys[j];

      // Look for time match in keys
      if(KeyTime >= Key->m_StartBlend && KeyTime <= Key->m_EndTime) {

        // Got a match! Render it.

        // Go to next animation
        break;
      }
    }
  }
}







/*

// {C5D9F28D-F0AF-416c-94D7-755174D65DC9}
DEFINE_GUID(BlendedMorphAnimationKey,
            0xc5d9f28d, 0xf0af, 0x416c, 
            0x94, 0xd7, 0x75, 0x51, 
            0x74, 0xd6, 0x5d, 0xc9);

template BlendedMorphAnimationKey
{
  <C5D9F28D-F0AF-416c-94D7-755174D65DC9>
  <???????????????>
  DWORD  StartTime;
  DWORD  EndTime;
  STRING MeshName;
  FLOAT  StartBlend;
  FLOAT  EndBlend;
}

// {2816376D-E98A-46ac-8075-59CAF3528E9A}
DEFINE_GUID(BlendedMorphAnimation,
            0x2816376d, 0xe98a, 0x46ac, 
            0x80, 0x75, 0x59, 0xca, 
            0xf3, 0x52, 0x8e, 0x9a);

template BlendedMorphAnimation 
{
  <2816376D-E98A-46ac-8075-59CAF3528E9A>
  DWORD Loop;
  DWORD ChannelNum;
  DWORD NumKeys;
  array BlendedMorphAnimationKey Keys[NumKeys];
}

// {FE99245E-E782-43f7-9329-5E183003D85E}
DEFINE_GUID(BlendedMorphBaseMesh,
            0xfe99245e, 0xe782, 0x43f7, 
            0x93, 0x29, 0x5e, 0x18, 
            0x30, 0x3, 0xd8, 0x5e);

template BlendedMorphBaseMesh
{
  <FE99245E-E782-43f7-9329-5E183003D85E>
  DWORD NumKeys;
  array BlendedMorphAnimationKey Keys[NumKeys];
}

// {5A7A03DB-273D-4112-8825-CBC45EF7219C}
DEFINE_GUID(BlendedMorphAnimationSet,
            0x5a7a03db, 0x273d, 0x4112, 
            0x88, 0x25, 0xcb, 0xc4, 
            0x5e, 0xf7, 0x21, 0x9c);

template BlendedMorphAnimationSet
{
  <5A7A03DB-273D-4112-8825-CBC45EF7219C>
  [BlendedMorphBaseMesh]
  [...]
}
*/

/*
BlendedMorphAnimationSet TalkingToYou
{
  BlendedMorphBaseMesh
  {
    1;
    0, 10000, "FACE_NEUTRAL", 0.00000, 0.00000;;
  }

  BlendedMorphAnimation Blink
  {
    1;  // Loop 0=no, 1=yes
    0;  // Channel # to use
    2;  // # keys to follow
       0, 499, "FACE_BLINK", 0.00000, 1.00000;,
     500, 999, "FACE_BLINK", 1.00000, 0.00000;;
  }

  BlendedMorphAnimation Expressions
  {
    1;  // Loop
    1;  // Channel
    2;  // # keys to follow
       0, 4999, "FACE_SMILE", 0.00000, 1.00000;,
    5000, 9999, "FACE_SMILE", 1.00000, 0.00000;;
  }

  BlendedMorphAnimation Mouthing1
  {
    1;
    2;
  }
}
*/
