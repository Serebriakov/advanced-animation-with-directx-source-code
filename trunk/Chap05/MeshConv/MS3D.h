#ifndef _MS3D_H_
#define _MS3D_H_

#define NUMMS3DNAMES(x) (sizeof(x)/sizeof(char*)/2)

///////////////////////////////////////////////////////////
// Milkshape 3-D .MS3D file structures
///////////////////////////////////////////////////////////
#pragma pack(1)

typedef struct {
  char Signature[10];  // Must be 'MS3D000000'
  int  Version;        // File format version (3 or 4)
} sMS3DHeader;

typedef struct {
  char        Flags;   // Editor flags
  float       Pos[3];  // World coordinates
  signed char Bone;    // -1 = no bone
  char        Count;   // Reference count
} sMS3DVertex;

typedef struct {
  unsigned short Flags;           // Editor flags
  unsigned short Indices[3];      // Vertex indices
  float          Normal[3][3];    // Normals (each vertex, x/y/z)
  float          u[3];            // Texture u coordinate
  float          v[3];            // Texture v coordinate
  char           SmoothingGroup;  // Smoothing group
  char           GroupIndex;      // Material group
} sMS3DFace;

typedef struct sMS3DGroup {
  struct {
    char            Flags;     // Editor flags
    char            Name[32];  // Name of group
    unsigned short  NumFaces;  // # faces in group
  } Header;
  unsigned short  *Indices;    // Face indices
  char             Material;   // -1 = no material

  sMS3DGroup()  { Indices = NULL;    }
  ~sMS3DGroup() { delete [] Indices; }
} sMS3DGroup;

typedef struct {
  char  Name[32];      // Material name
  float Ambient[4];    // Ambient colors
  float Diffuse[4];    // Diffuse colors
  float Specular[4];   // Specular colors
  float Emissive[4];   // Emmisive colors
  float Shininess;     // Shininess strength
  float Transparency;  // Transparency amount
  char  Mode;          // Mode 0-3
  char  Texture[128];  // Texture map .bmp filename
  char  Alphamap[128]; // Alpha map .bmp filename
} sMS3DMaterial;

typedef struct {
  float Time;     // Time of keyframe
  float Value[3]; // keyframe values (position, rotation)
} sMS3DKeyFrame;

typedef struct sMS3DBone {
  struct {
    char           Flags;         // Editor flags
    char           Name[32];      // Bone name
    char           Parent[32];    // Parent bone name
    float          Rotation[3];   // Rotation of bone
    float          Position[3];   // Position of bone
    unsigned short NumRotFrames;  // # of rotation key frames
    unsigned short NumPosFrames;  // # of position key frames
  } Header;
  sMS3DKeyFrame *RotKeyFrames;    // Rotation key frames
  sMS3DKeyFrame *PosKeyFrames;    // Position key frames

  sMS3DBone()  { RotKeyFrames = NULL; PosKeyFrames = NULL;       }
  ~sMS3DBone() { delete [] RotKeyFrames; delete [] PosKeyFrames; }
} sMS3DBone;

typedef struct sMS3DMeshVertex {
  float  x,  y,  z;   // Coordinates of vertex
  float  nx, ny, nz;  // Vertex normals
  float  u,  v;       // Texture coordinates
  DWORD  BoneNum;     // Bone connected to
} sMS3DMeshVertex;

typedef struct sMS3DBoneContainer {
  char Name[32];
  D3DXMATRIX matRotation;
  D3DXMATRIX matTranslation;

  D3DXMATRIX matTransformation;
  D3DXMATRIX matCombined;
  D3DXMATRIX matInvCombined;

  sMS3DBoneContainer *Sibling;
  sMS3DBoneContainer *Child;

  sMS3DBoneContainer() { Sibling = NULL; Child = NULL; }
} sMS3DBoneContainer;

#pragma pack()

#endif
