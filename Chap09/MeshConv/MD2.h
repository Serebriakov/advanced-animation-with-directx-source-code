#ifndef _MD2_H_
#define _MD2_H_

#define MD2FPS 200

#pragma pack(1)

// This holds the header information that is read in at the beginning of the file
typedef struct {
   DWORD Signature;            // File signature (0x32504449)
   DWORD Version;              // Version # (8)
   DWORD SkinWidth;            // Width of skin (texture)
   DWORD SkinHeight;           // Height of skin (texture)
   DWORD FrameSize;            // Size of frame
   DWORD NumSkins;             // # of skins (textures)
   DWORD NumVertices;          // # of vertices
   DWORD NumTextureCoords;     // # of texture coordinates
   DWORD NumFaces;             // # of faces
   DWORD NumGLCommands;        // # of GL (drawing) commands
   DWORD NumFrames;            // # of frames
   DWORD OffsetSkins;          // Offset to skins (textures)
   DWORD OffsetTextureCoords;  // Offset to texture structures
   DWORD OffsetFaces;          // Offset to triangle face structures
   DWORD OffsetFrames;         // Offset to frame structures
   DWORD OffsetGLCommands;     // Offset to drawing commands
   DWORD FileSize;             // Size of file
} sMD2Header;

typedef struct {
  char Vertex[3];    // Vertex x, -z, y (scaled down)
  char LightNormal;  // Light normal index
} sMD2FrameVertex;

typedef struct {
  float           Scale[3];      // Vertex scaling
  float           Translate[3];  // Vertex translation
  char            Name[16];      // Name of frame
  sMD2FrameVertex Vertices[1];   // Vertex list
} sMD2Frame;

typedef struct {
  unsigned short Indices[3];
  unsigned short TextureCoords[3];
} sMD2Face;

typedef struct {
  unsigned short u, v;  // Texture coordinates (0-width/height)
} sMD2TextureCoord;

typedef struct {
  float  x,  y,  z;
  float nx, ny, nz;
  float  u,  v;
} sMD2MeshVertex;

#pragma pack()

// These are precomputed normals used with the LightNormal index
// in the sMD2FrameVertex structure. These values are from the
// Quake 2 source code distribution. Note that the normals
// have the y and z values flipped, and z is negatated
float g_Quake2Normals[162][3] = {
  { -0.525731f,  0.000000f,  0.850651f },
  { -0.442863f,  0.238856f,  0.864188f },
  { -0.295242f,  0.000000f,  0.955423f },
  { -0.309017f,  0.500000f,  0.809017f },
  { -0.162460f,  0.262866f,  0.951056f },
  {  0.000000f,  0.000000f,  1.000000f },
  {  0.000000f,  0.850651f,  0.525731f },
  { -0.147621f,  0.716567f,  0.681718f },
  {  0.147621f,  0.716567f,  0.681718f },
  {  0.000000f,  0.525731f,  0.850651f },
  {  0.309017f,  0.500000f,  0.809017f },
  {  0.525731f,  0.000000f,  0.850651f },
  {  0.295242f,  0.000000f,  0.955423f },
  {  0.442863f,  0.238856f,  0.864188f },
  {  0.162460f,  0.262866f,  0.951056f },
  { -0.681718f,  0.147621f,  0.716567f },
  { -0.809017f,  0.309017f,  0.500000f },
  { -0.587785f,  0.425325f,  0.688191f },
  { -0.850651f,  0.525731f,  0.000000f },
  { -0.864188f,  0.442863f,  0.238856f },
  { -0.716567f,  0.681718f,  0.147621f },
  { -0.688191f,  0.587785f,  0.425325f },
  { -0.500000f,  0.809017f,  0.309017f },
  { -0.238856f,  0.864188f,  0.442863f },
  { -0.425325f,  0.688191f,  0.587785f },
  { -0.716567f,  0.681718f, -0.147621f },
  { -0.500000f,  0.809017f, -0.309017f },
  { -0.525731f,  0.850651f,  0.000000f },
  {  0.000000f,  0.850651f, -0.525731f },
  { -0.238856f,  0.864188f, -0.442863f },
  {  0.000000f,  0.955423f, -0.295242f },
  { -0.262866f,  0.951056f, -0.162460f },
  {  0.000000f,  1.000000f,  0.000000f },
  {  0.000000f,  0.955423f,  0.295242f },
  { -0.262866f,  0.951056f,  0.162460f },
  {  0.238856f,  0.864188f,  0.442863f },
  {  0.262866f,  0.951056f,  0.162460f },
  {  0.500000f,  0.809017f,  0.309017f },
  {  0.238856f,  0.864188f, -0.442863f },
  {  0.262866f,  0.951056f, -0.162460f },
  {  0.500000f,  0.809017f, -0.309017f },
  {  0.850651f,  0.525731f,  0.000000f },
  {  0.716567f,  0.681718f,  0.147621f },
  {  0.716567f,  0.681718f, -0.147621f },
  {  0.525731f,  0.850651f,  0.000000f },
  {  0.425325f,  0.688191f,  0.587785f },
  {  0.864188f,  0.442863f,  0.238856f },
  {  0.688191f,  0.587785f,  0.425325f },
  {  0.809017f,  0.309017f,  0.500000f },
  {  0.681718f,  0.147621f,  0.716567f },
  {  0.587785f,  0.425325f,  0.688191f },
  {  0.955423f,  0.295242f,  0.000000f },
  {  1.000000f,  0.000000f,  0.000000f },
  {  0.951056f,  0.162460f,  0.262866f },
  {  0.850651f, -0.525731f,  0.000000f },
  {  0.955423f, -0.295242f,  0.000000f },
  {  0.864188f, -0.442863f,  0.238856f },
  {  0.951056f, -0.162460f,  0.262866f },
  {  0.809017f, -0.309017f,  0.500000f },
  {  0.681718f, -0.147621f,  0.716567f },
  {  0.850651f,  0.000000f,  0.525731f },
  {  0.864188f,  0.442863f, -0.238856f },
  {  0.809017f,  0.309017f, -0.500000f },
  {  0.951056f,  0.162460f, -0.262866f },
  {  0.525731f,  0.000000f, -0.850651f },
  {  0.681718f,  0.147621f, -0.716567f },
  {  0.681718f, -0.147621f, -0.716567f },
  {  0.850651f,  0.000000f, -0.525731f },
  {  0.809017f, -0.309017f, -0.500000f },
  {  0.864188f, -0.442863f, -0.238856f },
  {  0.951056f, -0.162460f, -0.262866f },
  {  0.147621f,  0.716567f, -0.681718f },
  {  0.309017f,  0.500000f, -0.809017f },
  {  0.425325f,  0.688191f, -0.587785f },
  {  0.442863f,  0.238856f, -0.864188f },
  {  0.587785f,  0.425325f, -0.688191f },
  {  0.688191f,  0.587785f, -0.425325f },
  { -0.147621f,  0.716567f, -0.681718f },
  { -0.309017f,  0.500000f, -0.809017f },
  {  0.000000f,  0.525731f, -0.850651f },
  { -0.525731f,  0.000000f, -0.850651f },
  { -0.442863f,  0.238856f, -0.864188f },
  { -0.295242f,  0.000000f, -0.955423f },
  { -0.162460f,  0.262866f, -0.951056f },
  {  0.000000f,  0.000000f, -1.000000f },
  {  0.295242f,  0.000000f, -0.955423f },
  {  0.162460f,  0.262866f, -0.951056f },
  { -0.442863f, -0.238856f, -0.864188f },
  { -0.309017f, -0.500000f, -0.809017f },
  { -0.162460f, -0.262866f, -0.951056f },
  {  0.000000f, -0.850651f, -0.525731f },
  { -0.147621f, -0.716567f, -0.681718f },
  {  0.147621f, -0.716567f, -0.681718f },
  {  0.000000f, -0.525731f, -0.850651f },
  {  0.309017f, -0.500000f, -0.809017f },
  {  0.442863f, -0.238856f, -0.864188f },
  {  0.162460f, -0.262866f, -0.951056f },
  {  0.238856f, -0.864188f, -0.442863f },
  {  0.500000f, -0.809017f, -0.309017f },
  {  0.425325f, -0.688191f, -0.587785f },
  {  0.716567f, -0.681718f, -0.147621f },
  {  0.688191f, -0.587785f, -0.425325f },
  {  0.587785f, -0.425325f, -0.688191f },
  {  0.000000f, -0.955423f, -0.295242f },
  {  0.000000f, -1.000000f,  0.000000f },
  {  0.262866f, -0.951056f, -0.162460f },
  {  0.000000f, -0.850651f,  0.525731f },
  {  0.000000f, -0.955423f,  0.295242f },
  {  0.238856f, -0.864188f,  0.442863f },
  {  0.262866f, -0.951056f,  0.162460f },
  {  0.500000f, -0.809017f,  0.309017f },
  {  0.716567f, -0.681718f,  0.147621f },
  {  0.525731f, -0.850651f,  0.000000f },
  { -0.238856f, -0.864188f, -0.442863f },
  { -0.500000f, -0.809017f, -0.309017f },
  { -0.262866f, -0.951056f, -0.162460f },
  { -0.850651f, -0.525731f,  0.000000f },
  { -0.716567f, -0.681718f, -0.147621f },
  { -0.716567f, -0.681718f,  0.147621f },
  { -0.525731f, -0.850651f,  0.000000f },
  { -0.500000f, -0.809017f,  0.309017f },
  { -0.238856f, -0.864188f,  0.442863f },
  { -0.262866f, -0.951056f,  0.162460f },
  { -0.864188f, -0.442863f,  0.238856f },
  { -0.809017f, -0.309017f,  0.500000f },
  { -0.688191f, -0.587785f,  0.425325f },
  { -0.681718f, -0.147621f,  0.716567f },
  { -0.442863f, -0.238856f,  0.864188f },
  { -0.587785f, -0.425325f,  0.688191f },
  { -0.309017f, -0.500000f,  0.809017f },
  { -0.147621f, -0.716567f,  0.681718f },
  { -0.425325f, -0.688191f,  0.587785f },
  { -0.162460f, -0.262866f,  0.951056f },
  {  0.442863f, -0.238856f,  0.864188f },
  {  0.162460f, -0.262866f,  0.951056f },
  {  0.309017f, -0.500000f,  0.809017f },
  {  0.147621f, -0.716567f,  0.681718f },
  {  0.000000f, -0.525731f,  0.850651f },
  {  0.425325f, -0.688191f,  0.587785f },
  {  0.587785f, -0.425325f,  0.688191f },
  {  0.688191f, -0.587785f,  0.425325f },
  { -0.955423f,  0.295242f,  0.000000f },
  { -0.951056f,  0.162460f,  0.262866f },
  { -1.000000f,  0.000000f,  0.000000f },
  { -0.850651f,  0.000000f,  0.525731f },
  { -0.955423f, -0.295242f,  0.000000f },
  { -0.951056f, -0.162460f,  0.262866f },
  { -0.864188f,  0.442863f, -0.238856f },
  { -0.951056f,  0.162460f, -0.262866f },
  { -0.809017f,  0.309017f, -0.500000f },
  { -0.864188f, -0.442863f, -0.238856f },
  { -0.951056f, -0.162460f, -0.262866f },
  { -0.809017f, -0.309017f, -0.500000f },
  { -0.681718f,  0.147621f, -0.716567f },
  { -0.681718f, -0.147621f, -0.716567f },
  { -0.850651f,  0.000000f, -0.525731f },
  { -0.688191f,  0.587785f, -0.425325f },
  { -0.587785f,  0.425325f, -0.688191f },
  { -0.425325f,  0.688191f, -0.587785f },
  { -0.425325f, -0.688191f, -0.587785f },
  { -0.587785f, -0.425325f, -0.688191f },
  { -0.688191f, -0.587785f, -0.425325f }
};

#endif