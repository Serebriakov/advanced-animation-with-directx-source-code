#ifndef _MORPHANIM_H_
#define _MORPHANIM_H_

#include <windows.h>
#include "Direct3D.h"
#include "dxfile.h"
#include "XParser.h"
#include "XFile.h"
#include "initguid.h"

class cMorphAnimationKey
{
  public:
    DWORD m_Time;      // Time of key
    char *m_MeshName;  // Name of mesh to use
    D3DXMESHCONTAINER_EX *m_MeshPtr;   // Internal pointer to mesh data

  public:
    cMorphAnimationKey();
    ~cMorphAnimationKey();
};

class cMorphAnimationSet
{
  public:
    char               *m_Name;    // Name of animation
    DWORD               m_Length;  // Length of animation
    cMorphAnimationSet *m_Next;    // Next animation in linked list

    DWORD               m_NumKeys; // # keys in animation
    cMorphAnimationKey *m_Keys;    // Array of keys

  public:
    cMorphAnimationSet();
    ~cMorphAnimationSet();
};

class cMorphAnimationCollection : public cXParser
{
  protected:
    DWORD               m_NumAnimationSets;  // # animation sets
    cMorphAnimationSet *m_AnimationSets;     // Animation sets

  protected:
    // Parse an .X file for mass and spring data
    BOOL ParseObject(IDirectXFileData *pDataObj,
                       IDirectXFileData *pParentDataObj,
                       DWORD Depth,
                       void **Data, BOOL Reference);

  public:
    cMorphAnimationCollection();
    ~cMorphAnimationCollection();

    BOOL Load(char *Filename);
    void Free();

    void Map(D3DXMESHCONTAINER_EX *RootMesh);
    void Update(char *AnimationSetName,                       \
                DWORD Time, BOOL Loop,                        \
                D3DXMESHCONTAINER_EX **ppSource,              \
                D3DXMESHCONTAINER_EX **ppTarget,              \
                float *Scalar);
};

#endif
