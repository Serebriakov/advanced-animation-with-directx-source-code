#ifndef _SKELETALANIM_H_
#define _SKELETALANIM_H_

#include <windows.h>
#include "Direct3D.h"
#include "dxfile.h"
#include "XParser.h"
#include "XFile.h"

///////////////////////////////////////////////////////////
//
// Animation key type class containers
//
///////////////////////////////////////////////////////////
class cAnimationVectorKey
{
  public:
    DWORD       m_Time;
    D3DXVECTOR3 m_vecKey;
};

class cAnimationQuaternionKey
{
  public:
    DWORD          m_Time;
    D3DXQUATERNION m_quatKey;
};

class cAnimationMatrixKey
{
  public:
    DWORD      m_Time;
    D3DXMATRIX m_matKey;
};


///////////////////////////////////////////////////////////
//
// Animation class container
//
///////////////////////////////////////////////////////////
class cAnimation
{
  public:
    char          *m_Name;  // Bone's name
    D3DXFRAME_EX  *m_Bone;  // Pointer to bone frame
    cAnimation    *m_Next;  // Next animation object in list

    // # each key type and array of each type's keys
    DWORD                    m_NumTranslationKeys;
    cAnimationVectorKey     *m_TranslationKeys;
    DWORD                    m_NumScaleKeys;
    cAnimationVectorKey     *m_ScaleKeys;
    DWORD                    m_NumRotationKeys;
    cAnimationQuaternionKey *m_RotationKeys;
    DWORD                    m_NumMatrixKeys;
    cAnimationMatrixKey     *m_MatrixKeys;

  public:
    cAnimation();
    ~cAnimation();
};


///////////////////////////////////////////////////////////
//
// AnimationSet class container
//
///////////////////////////////////////////////////////////
class cAnimationSet
{
  public:
    char          *m_Name;   // Name of animation
    DWORD          m_Length; // Length of animation
    cAnimationSet *m_Next;   // Next set in linked list

    DWORD          m_NumAnimations; // # animations in set
    cAnimation    *m_Animations;    // Animations

  public:
    cAnimationSet();
    ~cAnimationSet();
};


///////////////////////////////////////////////////////////
//
// AnimationCollection class container
//
///////////////////////////////////////////////////////////
class cAnimationCollection : public cXParser
{
  protected:
    DWORD          m_NumAnimationSets;  // # animation sets
    cAnimationSet *m_AnimationSets;     // Animation sets

  protected:
    // Parse an .X file for mass and spring data
    BOOL ParseObject(IDirectXFileData *pDataObj,
                       IDirectXFileData *pParentDataObj,
                       DWORD Depth,
                       void **Data, BOOL Reference);

    // Find a frame by name
    D3DXFRAME_EX *FindFrame(D3DXFRAME *Frame, char *Name);

  public:
    cAnimationCollection();
    ~cAnimationCollection();

    BOOL Load(char *Filename);
    void Free();

    void Map(D3DXFRAME_EX *RootFrame);
    void Update(char *AnimationSetName, DWORD Time, BOOL Loop);
};

#endif
