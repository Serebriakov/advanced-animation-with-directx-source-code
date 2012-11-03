/*
  Milkshape 3D Model Loading Class by Jim Adams

  Milkshape 3D .ms3d file format:
    Header
    # Vertices
    Vertex data
    # Faces
    Face data
    # Groups
    Group data
    # Materials
    Material data
    Animation FPS
    CurrentTime
    Total Frames
    # Bones
    Bone Data
*/
#include <windows.h>
#include <stdio.h>
#include "d3d8.h"
#include "d3dx8.h"
#include "texture.h"
#include "mesh.h"
#include "MS3D.h"

BOOL LoadMS3D(cMeshGroup *MeshGroup, char *Filename,
              char *TexturePath, char *DefaultTexture, 
              IDirect3DDevice8 *pDevice, cTextureManager *pManager)
{
  char *Names[2];
  Names[0] = Filename;
  Names[1] = Filename;

  return LoadMS3D(MeshGroup, Filename, 1, (char**)&Names, TexturePath, DefaultTexture, pDevice, pManager);
}

BOOL LoadMS3D(cMeshGroup *MeshGroup, char *Filename,
              DWORD NumAnimations, char **AnimFilenames,
              char *TexturePath, char *DefaultTexture, 
              IDirect3DDevice8 *pDevice, cTextureManager *pManager)
{
  FILE           *fp;
  sMS3DHeader     Header;

  DWORD           i, j, k, Num, Pos, AnimPos;
  
  unsigned short  msNumVertices;
  sMS3DVertex    *msVertices = NULL;
  sMeshVertex    *Vertices;

  unsigned short  msNumFaces;
  sMS3DFace      *msFaces = NULL;
  unsigned short *Indices;

  unsigned short  msNumGroups;
  sMS3DGroup     *msGroups = NULL;

  unsigned short  msNumMaterials;
  sMS3DMaterial  *msMaterials = NULL;

  unsigned short  msNumBones;
  sMS3DBone      *msBones = NULL;

  float           msAnimationFPS;
  float           msCurrentTime;
  int             msTotalFrames;

  cMeshContainer      *Mesh = NULL;
  cAnimationContainer *Anim = NULL;

  // Error checking
  if(MeshGroup == NULL || Filename == NULL || pDevice == NULL || pManager == NULL)
    return FALSE;

  // Free a prior model
  MeshGroup->Free();

  // Assign mesh type
  MeshGroup->m_Type = MESH_BONES;

  // Assign device and texture manager
  MeshGroup->m_Device  = pDevice;
  MeshGroup->m_Manager = pManager;

  // Open file and read header
  if((fp=fopen(Filename, "rb"))==NULL)
    return FALSE;
  fread(&Header, 1, sizeof(sMS3DHeader), fp);

  // Make sure header is valid
  if(memcmp(Header.Signature, "MS3D000000", 10)) {
    fclose(fp);
    return FALSE;
  }

  // Read data into some temporary buffers

  // Read vertex data
  fread(&msNumVertices, 1, sizeof(short), fp);
  if(msNumVertices) {
    msVertices = new sMS3DVertex[msNumVertices];
    for(i=0;i<msNumVertices;i++) {
      fread(&msVertices[i], 1, sizeof(sMS3DVertex), fp);

      // Assign to bone # 0 if none assigned
      if(msVertices[i].Bone == -1)
        msVertices[i].Bone = 0;
    }
  }

  // Read face data
  fread(&msNumFaces, 1, sizeof(short), fp);
  if(msNumFaces) {
    msFaces = new sMS3DFace[msNumFaces];
    for(i=0;i<msNumFaces;i++)
      fread(&msFaces[i], 1, sizeof(sMS3DFace), fp);
  }

  // Read groups
  fread(&msNumGroups, 1, sizeof(short), fp);
  if(msNumGroups) {
    msGroups = new sMS3DGroup[msNumGroups];
    for(i=0;i<msNumGroups;i++) {
      fread(&msGroups[i], 1, sizeof(msGroups[i].Header), fp);
      if((Num = msGroups[i].Header.NumFaces)) {
        msGroups[i].Indices = new unsigned short[Num];
        for(j=0;j<Num;j++)
          fread(&msGroups[i].Indices[j], 1, sizeof(short), fp);
      }
      fread(&msGroups[i].Material, 1, sizeof(char), fp);

      // Change group material to 0 if none assigned (-1)
      if(msGroups[i].Material == -1)
        msGroups[i].Material = 0;
    }
  }

  // Read materials, creating a default one if none in file
  fread(&msNumMaterials, 1, sizeof(short), fp);
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
      fread(&msMaterials[i], 1, sizeof(sMS3DMaterial), fp);
  }

  // Record position of animation data in file (for loading animations)
  AnimPos = ftell(fp);

  // Read in FPS, current editor time, and # frames
  fread(&msAnimationFPS, 1, sizeof(float), fp);
  fread(&msCurrentTime, 1, sizeof(float), fp);
  fread(&msTotalFrames, 1, sizeof(int), fp);

  // Read in bones, skipping keyframe data
  fread(&msNumBones, 1, sizeof(short), fp);
  if(msNumBones) {
    msBones = new sMS3DBone[msNumBones];
    for(i=0;i<msNumBones;i++) {
      fread(&msBones[i], 1, sizeof(msBones[i].Header), fp);

      // Skip by position key frames
      if((Num = msBones[i].Header.NumRotFrames))
        fseek(fp, Num * sizeof(sMS3DKeyFrame), SEEK_CUR);

      // Skip by animation key frames
      if((Num=msBones[i].Header.NumPosFrames))
        fseek(fp, Num * sizeof(sMS3DKeyFrame), SEEK_CUR);
    }
  }

  // Close file
  fclose(fp);

  // At this point, the entire Milkshape 3D model is loaded
  // in memory. You need to parse this data into a
  // cMeshContainer object now.

  // Create a single mesh to use
  MeshGroup->m_NumMeshes = 1;
  MeshGroup->m_Meshes = Mesh = new cMeshContainer();
  
  // Name the mesh by clipping out mesh filename (without path and extension)
  Mesh->m_Name = CleanMS3DName(Filename);

  // Store mesh values ////////////////////////////////////
  // Notice that the number of vertices is based on the number
  // of faces. The reason being is that Milkshape 3D stores
  // texture coordinates based on faces, so a single vertex
  // may have multiple texture coordinates. Same goes for normals.
  Mesh->m_NumVertices  = (DWORD)msNumFaces * 3;
  Mesh->m_NumFaces     = (DWORD)msNumFaces;
  Mesh->m_NumBones     = (DWORD)msNumBones;
  Mesh->m_NumMaterials = (DWORD)msNumMaterials;

  // Create the bone hierarchy ////////////////////////////
  if(Mesh->m_NumBones) {
    Mesh->m_Bones = new cBoneHierarchy();

    // Create each bone in the hierarchy
    Mesh->m_Bones->m_Bones = new cBoneContainer[Mesh->m_NumBones]();

    // Create the bone matrices
    Mesh->m_Bones->m_matCombined        = new D3DXMATRIX[Mesh->m_NumBones];
    Mesh->m_Bones->m_matTransformations = new D3DXMATRIX[Mesh->m_NumBones];
    Mesh->m_Bones->m_matInversed        = new D3DXMATRIX[Mesh->m_NumBones];

    for(i=0;i<Mesh->m_NumBones;i++) {

      // Point bones to matrices
      Mesh->m_Bones->m_Bones[i].m_matCombined       = &Mesh->m_Bones->m_matCombined[i];
      Mesh->m_Bones->m_Bones[i].m_matTransformation = &Mesh->m_Bones->m_matTransformations[i];

      // Build the hierarchy
      Mesh->m_Bones->m_Bones[i].m_Name = strdup(msBones[i].Header.Name);
      for(j=0;j<Mesh->m_NumBones;j++) {

        // Match parent
        if(i != j && msBones[i].Header.Parent[0] && !stricmp(msBones[i].Header.Parent, msBones[j].Header.Name)) {
          // i = current bone, j = parent bone

          // Link sibling
          Mesh->m_Bones->m_Bones[i].m_Sibling = Mesh->m_Bones->m_Bones[j].m_Child;

          // Link child
          Mesh->m_Bones->m_Bones[j].m_Child = &Mesh->m_Bones->m_Bones[i];
        }
      }

      // Create default pose and store in transformation matrix
      D3DXMATRIX matX, matY, matZ, matXYZ, matWorld;

      D3DXMatrixRotationX(&matX, msBones[i].Header.Rotation[0]);
      D3DXMatrixRotationY(&matY, msBones[i].Header.Rotation[1]);
      D3DXMatrixRotationZ(&matZ, msBones[i].Header.Rotation[2]);
      D3DXMatrixTranslation(&matXYZ, 
                             msBones[i].Header.Position[0], 
                             msBones[i].Header.Position[1], 
                             msBones[i].Header.Position[2]);
      D3DXMatrixMultiply(&matWorld, &matX, &matY);
      D3DXMatrixMultiply(&matWorld, &matWorld, &matZ);
      D3DXMatrixMultiply(&Mesh->m_Bones->m_matTransformations[i], &matWorld, &matXYZ);
    }

    // Since the mesh has been converted from right-handed
    // to left-handed coordinates, the bones than also need
    // to be flipped. Since the bones are based off
    // transformation matrices, than the root bone matrix
    // must be 'reflected' in order to be correct.
    // To reflect a matrix, you need to construct a splitting
    // plane that defines where to reflect.
    D3DXPLANE  planeReflect;
    D3DXMATRIX matReflect;
    D3DXPlaneFromPoints(&planeReflect, 
                        &D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                        &D3DXVECTOR3(0.0f, 0.0f, 1.0f),
                        &D3DXVECTOR3(0.0f, 1.0f, 1.0f));
    D3DXMatrixReflect(&matReflect, &planeReflect);
    D3DXMatrixMultiply(&Mesh->m_Bones->m_matTransformations[0],
                       &Mesh->m_Bones->m_matTransformations[0],
                       &matReflect);

    // Update the bone hierarchy and store inversed matrices
    Mesh->m_Bones->m_Bones[0].UpdateHierarchy();

    // Inverse the combined matrices and store
    for(i=0;i<Mesh->m_NumBones;i++)
      D3DXMatrixInverse(&Mesh->m_Bones->m_matInversed[i], NULL, &Mesh->m_Bones->m_matCombined[i]);
  }

  // Create materials and ranges //////////////////////////
  Mesh->m_Ranges    = new cMeshRange[Mesh->m_NumMaterials]();
  Mesh->m_Materials = new D3DMATERIAL8[Mesh->m_NumMaterials];
  Mesh->m_Textures  = new IDirect3DTexture8*[Mesh->m_NumMaterials];
  for(i=0;i<Mesh->m_NumMaterials;i++) {
    Mesh->m_Materials[i].Diffuse.r  = msMaterials[i].Diffuse[0];
    Mesh->m_Materials[i].Diffuse.g  = msMaterials[i].Diffuse[1];
    Mesh->m_Materials[i].Diffuse.b  = msMaterials[i].Diffuse[2];

    Mesh->m_Materials[i].Ambient.r  = msMaterials[i].Ambient[0];
    Mesh->m_Materials[i].Ambient.g  = msMaterials[i].Ambient[1];
    Mesh->m_Materials[i].Ambient.b  = msMaterials[i].Ambient[2];

    Mesh->m_Materials[i].Specular.r = msMaterials[i].Specular[0];
    Mesh->m_Materials[i].Specular.g = msMaterials[i].Specular[1];
    Mesh->m_Materials[i].Specular.b = msMaterials[i].Specular[2];

    Mesh->m_Materials[i].Emissive.r = msMaterials[i].Emissive[0];
    Mesh->m_Materials[i].Emissive.g = msMaterials[i].Emissive[1];
    Mesh->m_Materials[i].Emissive.b = msMaterials[i].Emissive[2];

    Mesh->m_Materials[i].Power = msMaterials[i].Shininess;

    Mesh->m_Textures[i] = NULL;
    if(msMaterials[i].Texture[0]) {
      char TextureFilename[MAX_PATH];
      sprintf(TextureFilename, "%s%s", TexturePath, msMaterials[i].Texture);
      Mesh->m_Textures[i] = pManager->Get(TextureFilename, 0, pDevice);
    }
  }

  // Build vertex data ////////////////////////////////////
  pDevice->CreateVertexBuffer(Mesh->m_NumVertices * sizeof(sMeshVertex), 0, MESHFVF, D3DPOOL_MANAGED, &Mesh->m_VertexBuffer);
  Mesh->m_VertexBuffer->Lock(0, 0, (BYTE**)&Vertices, 0);
  Pos = 0;
  for(i=0;i<Mesh->m_NumFaces;i++) {
    for(j=0;j<3;j++) {
      // Store vertex coordinates
      // Notice that x is reversed to change
      // from right-handed to left-handed coordinates
      Vertices[Pos].x = -msVertices[msFaces[i].Indices[j]].Pos[0];
      Vertices[Pos].y =  msVertices[msFaces[i].Indices[j]].Pos[1];
      Vertices[Pos].z =  msVertices[msFaces[i].Indices[j]].Pos[2];

      // Store vertex normals
      // Ditto on reversal of normal's x
      Vertices[Pos].nx = -msFaces[i].Normal[j][0];
      Vertices[Pos].ny =  msFaces[i].Normal[j][1];
      Vertices[Pos].nz =  msFaces[i].Normal[j][2];

      // Store vertex texture coordinates
      Vertices[Pos].u = msFaces[i].u[j];
      Vertices[Pos].v = msFaces[i].v[j];

      // Assign bone, defaulting to #0 if none assigned.
      // Multiply bone index by 3 to put in constant memory
      Vertices[Pos].Bone = (float)msVertices[msFaces[i].Indices[j]].Bone * 3.0f;

      // Go to next vertex/index
      Pos++;
    }
  }
  Mesh->m_VertexBuffer->Unlock();

  // Indices are built by materials and groups - not by the
  // face array. Go through each material and looking for 
  // matching groups. For each matching group, add those
  // indices belonging to the group.
  pDevice->CreateIndexBuffer(Mesh->m_NumFaces * 3 * sizeof(short), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &Mesh->m_IndexBuffer);
  Mesh->m_IndexBuffer->Lock(0, 0, (BYTE**)&Indices, 0);
  Pos = 0;
  for(i=0;i<Mesh->m_NumMaterials;i++) {

    // Store starting position of material range
    Mesh->m_Ranges[i].m_StartFace = Pos;

    // Scan through each group looking for matching materials
    Num = 0;
    for(j=0;j<msNumGroups;j++) {
      if((DWORD)msGroups[j].Material == i) {

        // Store indices in index buffer.
        // Notice that order reversed, as Milkshape 3D
        // uses right-hand system, not left as we do here.
        for(k=0;k<msGroups[j].Header.NumFaces;k++) {
          DWORD Face = msGroups[j].Indices[k] * 3;
          Indices[Pos++] = msGroups[j].Indices[k] * 3 + 2;
          Indices[Pos++] = msGroups[j].Indices[k] * 3 + 1;
          Indices[Pos++] = msGroups[j].Indices[k] * 3 + 0;
        }

        // Increase face count
        Num += msGroups[j].Header.NumFaces;
      }
    }

    // Store # faces
    Mesh->m_Ranges[i].m_NumFaces = Num;
  }
  Mesh->m_IndexBuffer->Unlock();

  // Load animations //////////////////////////////////////
  for(i=0;i<NumAnimations;i++)
    LoadMS3DAnimation(MeshGroup, Mesh, AnimFilenames[i*2], AnimFilenames[i*2+1], AnimPos);

  // Release all used data
  delete [] msVertices;
  delete [] msFaces;
  delete [] msGroups;
  delete [] msMaterials;
  delete [] msBones;

  // Return success
  return TRUE;
}

BOOL LoadMS3DAnimation(cMeshGroup *MeshGroup, 
                       cMeshContainer *Mesh,
                       char *Filename, char *Name,
                       DWORD Pos)
{
  FILE                *fp;
  cAnimationContainer *Anim = NULL;
  float                msAnimationFPS, msCurrentTime;
  int                  msTotalFrames;
  unsigned short       msNumBones;
  sMS3DBone           *msBones = NULL;
  DWORD                i, j, Num, NumFrames=0;

  // Error checking
  if(MeshGroup == NULL || Filename == NULL || Name == NULL || !Mesh->m_NumBones)
    return FALSE;

  // Open file and seek to start of bone data
  if((fp=fopen(Filename, "rb"))==NULL)
    return FALSE;
  fseek(fp, Pos, SEEK_SET);

  // Get animation FPS, editor time, and total frames
  fread(&msAnimationFPS, 1, sizeof(float), fp);
  fread(&msCurrentTime, 1, sizeof(float), fp);
  fread(&msTotalFrames, 1, sizeof(int), fp);

  // Read in # of bones and compare to # in mesh
  fread(&msNumBones, 1, sizeof(short), fp);
  if((DWORD)msNumBones != Mesh->m_NumBones) {
    fclose(fp);
    return FALSE;
  }
    
  // Read in bone data
  msBones = new sMS3DBone[Mesh->m_NumBones];
  for(i=0;i<Mesh->m_NumBones;i++) {
    fread(&msBones[i], 1, sizeof(msBones[i].Header), fp);

    // Read in animation rotation keys
    if((Num = msBones[i].Header.NumRotFrames)) {
      // Store highest keyframe #
      if(Num > NumFrames)
        NumFrames = Num;

      // Create and read in keyframe data
      msBones[i].RotKeyFrames = new sMS3DKeyFrame[Num];
      for(j=0;j<Num;j++)
        fread(&msBones[i].RotKeyFrames[j], 1, sizeof(sMS3DKeyFrame), fp);
    }

    // Read in animation position keys
    if((Num=msBones[i].Header.NumPosFrames)) {
      // Store highest keyframe #
      if(Num > NumFrames)
        NumFrames = Num;

      // Create and read in keyframe data
      msBones[i].PosKeyFrames = new sMS3DKeyFrame[Num];
      for(j=0;j<Num;j++)
        fread(&msBones[i].PosKeyFrames[j], 1, sizeof(sMS3DKeyFrame), fp);
    }
  }

  // Close file
  fclose(fp);

  // Don't go any further if there is no keyframes
  if(!NumFrames) {
    delete [] msBones;
    return FALSE;
  }

  // Build animation data
  Anim = new cAnimationContainer();

  // Assign a name to animation
  Anim->m_Name = strdup(Name);

  // Allocate the frames (using # of position keys as length)
  Anim->m_NumFrames = (DWORD)NumFrames;
  Anim->m_Frames = new cAnimationFrame[Anim->m_NumFrames]();

  // Go through each frame and create transformation matrices
  for(i=0;i<Anim->m_NumFrames;i++) {

    // Point frame to next (for linked list)
    if(i < Anim->m_NumFrames-1)
      Anim->m_Frames[i].m_Next = &Anim->m_Frames[i+1];

    // Allocate the matrices
    Anim->m_Frames[i].m_matTransformations = new D3DXMATRIX[Mesh->m_NumBones];

    // Store the time of this frame (from 1st bone Position key)
    Anim->m_Frames[i].m_Time = (DWORD)(msBones[0].PosKeyFrames[i].Time * msAnimationFPS) * 30;

    // Store length of animation based on last time
    Anim->m_Length = Anim->m_Frames[i].m_Time + 1;

    // Go through each bone and create transformation matrix
    for(j=0;j<Mesh->m_NumBones;j++) {

      // Calculate the bone's orientation 
      D3DXMATRIX matX, matY, matZ, matXYZ, matWorld;

      D3DXMatrixRotationX(&matX, msBones[j].RotKeyFrames[i].Value[0]);
      D3DXMatrixRotationY(&matY, msBones[j].RotKeyFrames[i].Value[1]);
      D3DXMatrixRotationZ(&matZ, msBones[j].RotKeyFrames[i].Value[2]);
      D3DXMatrixTranslation(&matXYZ,
                            msBones[j].PosKeyFrames[i].Value[0],
                            msBones[j].PosKeyFrames[i].Value[1],
                            msBones[j].PosKeyFrames[i].Value[2]);
      D3DXMatrixMultiply(&matWorld, &matX, &matY);
      D3DXMatrixMultiply(&matWorld, &matWorld, &matZ);
      D3DXMatrixMultiply(&matWorld, &matWorld, &matXYZ);

      // Combine the frame's transformation matrix to
      // the bone's transformation matrix to obtain 
      // the final transformation matrix.
      D3DXMatrixMultiply(&Anim->m_Frames[i].m_matTransformations[j], 
                         &matWorld,
                         &Mesh->m_Bones->m_matTransformations[j]);
    }
  }

  // Free used resources
  delete [] msBones;

  // Link animation into list (as last in list)
  if(MeshGroup->m_Animations == NULL)
    MeshGroup->m_Animations = Anim;
  else {
    // Scan to end of list and link in
    cAnimationContainer *AnimPtr = MeshGroup->m_Animations;
    while(AnimPtr->m_Next != NULL)
      AnimPtr = AnimPtr->m_Next;
    AnimPtr->m_Next = Anim;
  }
  MeshGroup->m_NumAnimations++;

  return TRUE;
}

char *CleanMS3DName(char *Name)
{
  char *NewName, *NamePtr, *Ptr;

  // Error checking
  if(Name == NULL)
    return NULL;

  // Find beginning of filename (remove path)
  if((Ptr = strrchr(Name, '\\')) == NULL)
    Ptr = Name;

  // Allocate some memory to save new name
  NamePtr = NewName = new char[strlen(Ptr)];

  // Copy over until a period or EOL found
  while(Ptr && *Ptr != '.')
    *NamePtr++ = *Ptr++;
  *NamePtr = 0;

  // Return resulting string
  return strdup(NewName);
}
