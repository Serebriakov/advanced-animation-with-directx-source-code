#include "SkeletalAnim.h"

cAnimation::cAnimation()
{
  m_Name = NULL;
  m_Bone = NULL;
  m_Next = NULL;

  m_NumTranslationKeys = 0;
  m_NumScaleKeys       = 0;
  m_NumRotationKeys    = 0;
  m_NumMatrixKeys      = 0;

  m_TranslationKeys = NULL;
  m_ScaleKeys       = NULL;
  m_RotationKeys    = NULL;
  m_MatrixKeys      = NULL;
}

cAnimation::~cAnimation()
{
  delete [] m_Name; m_Name = NULL;
  delete m_Next;    m_Next = NULL;
  delete [] m_TranslationKeys;
  delete [] m_ScaleKeys;
  delete [] m_RotationKeys;
  delete [] m_MatrixKeys;
}

cAnimationSet::cAnimationSet()
{
  m_Name          = NULL;
  m_Length        = 0;
  m_Next          = NULL;
  m_NumAnimations = 0;
  m_Animations    = NULL;
}

cAnimationSet::~cAnimationSet()
{
  delete [] m_Name;    m_Name = NULL;
  delete m_Next;       m_Next = NULL;
  delete m_Animations; m_Animations = NULL;
}

cAnimationCollection::cAnimationCollection()
{
  m_NumAnimationSets = 0;
  m_AnimationSets    = NULL;
}

cAnimationCollection::~cAnimationCollection()
{
  Free();
}

BOOL cAnimationCollection::ParseObject(                     \
                   IDirectXFileData *pDataObj,                \
                   IDirectXFileData *pParentDataObj,          \
                   DWORD Depth,                               \
                   void **Data, BOOL Reference)
{
  const GUID *Type = GetObjectGUID(pDataObj);
  DWORD i;

  // Check if template is AnimationSet type
  if(*Type == TID_D3DRMAnimationSet) {

    // Create and link in a cAnimationSet object
    cAnimationSet *AnimSet = new cAnimationSet();
    AnimSet->m_Next = m_AnimationSets;
    m_AnimationSets = AnimSet;

    // Increase # of animation sets
    m_NumAnimationSets++;

    // Set animation set name
    AnimSet->m_Name = GetObjectName(pDataObj);
  }

  // Check if template is Animation type
  if(*Type == TID_D3DRMAnimation && m_AnimationSets) {

    // Add a cAnimation class to top-level cAnimationSet
    cAnimation *Anim = new cAnimation();
    Anim->m_Next = m_AnimationSets->m_Animations;
    m_AnimationSets->m_Animations = Anim;

    // Increase # of animations
    m_AnimationSets->m_NumAnimations++;
  }

  // Check if a frame reference inside animation template
  if(*Type == TID_D3DRMFrame && Reference == TRUE &&          \
                             m_AnimationSets &&               \
                             m_AnimationSets->m_Animations) {

    // Make sure parent object is an Animation template
    if(pParentDataObj && *GetObjectGUID(pParentDataObj) ==  \
                                        TID_D3DRMAnimation) {

      // Get name of frame and store it as animation
      m_AnimationSets->m_Animations->m_Name =                 \
                                  GetObjectName(pDataObj);
    }

    // Don't process child of reference frames
    return TRUE;
  }

  // Check if template is AnimationKey type
  if(*Type == TID_D3DRMAnimationKey && m_AnimationSets &&     \
                            m_AnimationSets->m_Animations) {

    // Get a pointer to top-level animation object
    cAnimation *Anim = m_AnimationSets->m_Animations;

    // Get a data pointer
    DWORD *DataPtr = (DWORD*)GetObjectData(pDataObj, NULL);

    // Get key type
    DWORD Type = *DataPtr++;

    // Get # of keys to follow
    DWORD NumKeys = *DataPtr++;

    // Branch based on key type
    switch(Type) {
      case 0: // Rotation
        delete [] Anim->m_RotationKeys;
        Anim->m_NumRotationKeys = NumKeys;
        Anim->m_RotationKeys = new                            \
                             cAnimationQuaternionKey[NumKeys];
        for(i=0;i<NumKeys;i++) {
          // Get time
          Anim->m_RotationKeys[i].m_Time = *DataPtr++;
          if(Anim->m_RotationKeys[i].m_Time >                 \
                                   m_AnimationSets->m_Length)
            m_AnimationSets->m_Length =                       \
                              Anim->m_RotationKeys[i].m_Time;

          // Skip # keys to follow (should be 4)
          DataPtr++;

          // Get rotational values
          float *fPtr = (float*)DataPtr;
          Anim->m_RotationKeys[i].m_quatKey.w = *fPtr++;
          Anim->m_RotationKeys[i].m_quatKey.x = *fPtr++;
          Anim->m_RotationKeys[i].m_quatKey.y = *fPtr++;
          Anim->m_RotationKeys[i].m_quatKey.z = *fPtr++;
          DataPtr+=4;
        }
        break;

      case 1: // Scaling
        delete [] Anim->m_ScaleKeys;
        Anim->m_NumScaleKeys = NumKeys;
        Anim->m_ScaleKeys = new cAnimationVectorKey[NumKeys];
        for(i=0;i<NumKeys;i++) {
          // Get time
          Anim->m_ScaleKeys[i].m_Time = *DataPtr++;
          if(Anim->m_ScaleKeys[i].m_Time >                    \
                                   m_AnimationSets->m_Length)
            m_AnimationSets->m_Length =                       \
                                 Anim->m_ScaleKeys[i].m_Time;

          // Skip # keys to follow (should be 3)
          DataPtr++;

          // Get scale values
          D3DXVECTOR3 *vecPtr = (D3DXVECTOR3*)DataPtr;
          Anim->m_ScaleKeys[i].m_vecKey = *vecPtr;
          DataPtr+=3;
        }
        break;

      case 2: // Translation
        delete [] Anim->m_TranslationKeys;
        Anim->m_NumTranslationKeys = NumKeys;
        Anim->m_TranslationKeys = new                         \
                                cAnimationVectorKey[NumKeys];
        for(i=0;i<NumKeys;i++) {
          // Get time
          Anim->m_TranslationKeys[i].m_Time = *DataPtr++;
          if(Anim->m_TranslationKeys[i].m_Time >              \
                                   m_AnimationSets->m_Length)
            m_AnimationSets->m_Length =                       \
                           Anim->m_TranslationKeys[i].m_Time;

          // Skip # keys to follow (should be 3)
          DataPtr++;

          // Get translation values
          D3DXVECTOR3 *vecPtr = (D3DXVECTOR3*)DataPtr;
          Anim->m_TranslationKeys[i].m_vecKey = *vecPtr;
          DataPtr+=3;
        }
        break;

      case 4: // Transformation matrix
        delete [] Anim->m_MatrixKeys;
        Anim->m_NumMatrixKeys = NumKeys;
        Anim->m_MatrixKeys = new cAnimationMatrixKey[NumKeys];
        for(i=0;i<NumKeys;i++) {
          // Get time
          Anim->m_MatrixKeys[i].m_Time = *DataPtr++;

          if(Anim->m_MatrixKeys[i].m_Time >                   \
                                  m_AnimationSets->m_Length)
            m_AnimationSets->m_Length =                       \
                               Anim->m_MatrixKeys[i].m_Time;

          // Skip # keys to follow (should be 16)
          DataPtr++;

          // Get matrix values
          D3DXMATRIX *mPtr = (D3DXMATRIX *)DataPtr;
          Anim->m_MatrixKeys[i].m_matKey = *mPtr;
          DataPtr += 16;
        }
        break;
    }
  }

  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}

BOOL cAnimationCollection::Load(char *Filename)
{
  // Free a prior loaded collection
  Free();

  // Parse the file
  return Parse(Filename);
}

void cAnimationCollection::Free()
{
  m_NumAnimationSets = 0;
  delete m_AnimationSets; m_AnimationSets = NULL;
}

void cAnimationCollection::Map(D3DXFRAME_EX *RootFrame)
{
  // Go through each animation set
  cAnimationSet *AnimSet = m_AnimationSets;
  while(AnimSet != NULL) {

    // Go through each animation object
    cAnimation *Anim = AnimSet->m_Animations;
    while(Anim != NULL) {

      // Go through all frames and look for match
      Anim->m_Bone = RootFrame->Find(Anim->m_Name);

      // Go to next animation object
      Anim = Anim->m_Next;
    }

    // Go to next animation set object
    AnimSet = AnimSet->m_Next;
  }
}

void cAnimationCollection::Update(char *AnimationSetName,     \
                                  DWORD Time, BOOL Loop)
{
  cAnimationSet *AnimSet = m_AnimationSets;

  // Look for matching animation set name if used
  if(AnimationSetName) {

    // Find matching animation set name
    while(AnimSet != NULL) {

      // Break when match found
      if(!stricmp(AnimSet->m_Name, AnimationSetName))
        break;

      // Go to next animation set object
      AnimSet = AnimSet->m_Next;
    }
  }

  // Return no set found
  if(AnimSet == NULL)
    return;

  // Bounds time to animation length
  if(Time > AnimSet->m_Length)
    Time = (Loop==TRUE)?Time%(AnimSet->m_Length+1):AnimSet->m_Length;

  // Go through each animation
  cAnimation *Anim = AnimSet->m_Animations;
  while(Anim) {

    // Only process if it's attached to a bone
    if(Anim->m_Bone) {

      // Reset transformation
      D3DXMatrixIdentity(&Anim->m_Bone->TransformationMatrix);

      // Apply various matrices to transformation

      // Scaling
      if(Anim->m_NumScaleKeys && Anim->m_ScaleKeys) {

        // Loop for matching scale key
        DWORD Key1 = 0, Key2 = 0;
        for(DWORD i=0;i<Anim->m_NumScaleKeys;i++) {
          if(Time >= Anim->m_ScaleKeys[i].m_Time)
            Key1 = i;
        }

        // Get 2nd key number
        Key2 = (Key1>=(Anim->m_NumScaleKeys-1))?Key1:Key1+1;

        // Get difference in keys' times
        DWORD TimeDiff = Anim->m_ScaleKeys[Key2].m_Time-
                         Anim->m_ScaleKeys[Key1].m_Time;
        if(!TimeDiff)
          TimeDiff = 1;

        // Calculate a scalar value to use
        float Scalar = (float)(Time - Anim->m_ScaleKeys[Key1].m_Time) / (float)TimeDiff;

        // Calculate interpolated scale values
        D3DXVECTOR3 vecScale = Anim->m_ScaleKeys[Key2].m_vecKey - 
                               Anim->m_ScaleKeys[Key1].m_vecKey;
        vecScale *= Scalar;
        vecScale += Anim->m_ScaleKeys[Key1].m_vecKey;

        // Create scale matrix and combine with transformation
        D3DXMATRIX matScale;
        D3DXMatrixScaling(&matScale, vecScale.x, vecScale.y, vecScale.z);
        Anim->m_Bone->TransformationMatrix *= matScale;
      }

      // Rotation
      if(Anim->m_NumRotationKeys && Anim->m_RotationKeys) {

        // Loop for matching rotation key
        DWORD Key1 = 0, Key2 = 0;
        for(DWORD i=0;i<Anim->m_NumRotationKeys;i++) {
          if(Time >= Anim->m_RotationKeys[i].m_Time)
            Key1 = i;
        }

        // Get 2nd key number
        Key2 = (Key1>=(Anim->m_NumRotationKeys-1))?Key1:Key1+1;

        // Get difference in keys' times
        DWORD TimeDiff = Anim->m_RotationKeys[Key2].m_Time-
                         Anim->m_RotationKeys[Key1].m_Time;
        if(!TimeDiff)
          TimeDiff = 1;

        // Calculate a scalar value to use
        float Scalar = (float)(Time - Anim->m_RotationKeys[Key1].m_Time) / (float)TimeDiff;

        // slerp rotation values
        D3DXQUATERNION quatRotation;
        D3DXQuaternionSlerp(&quatRotation,
                            &Anim->m_RotationKeys[Key1].m_quatKey,
                            &Anim->m_RotationKeys[Key2].m_quatKey,
                            Scalar);

        // Create rotation matrix and combine with transformation
        D3DXMATRIX matRotation;
        D3DXMatrixRotationQuaternion(&matRotation, &quatRotation);
        Anim->m_Bone->TransformationMatrix *= matRotation;
      }

      // Translation
      if(Anim->m_NumTranslationKeys && Anim->m_TranslationKeys) {

        // Loop for matching translation key
        DWORD Key1 = 0, Key2 = 0;
        for(DWORD i=0;i<Anim->m_NumTranslationKeys;i++) {
          if(Time >= Anim->m_TranslationKeys[i].m_Time)
            Key1 = i;
        }

        // Get 2nd key number
        Key2 = (Key1>=(Anim->m_NumTranslationKeys-1))?Key1:Key1+1;

        // Get difference in keys' times
        DWORD TimeDiff = Anim->m_TranslationKeys[Key2].m_Time-
                         Anim->m_TranslationKeys[Key1].m_Time;
        if(!TimeDiff)
          TimeDiff = 1;

        // Calculate a scalar value to use
        float Scalar = (float)(Time - Anim->m_TranslationKeys[Key1].m_Time) / (float)TimeDiff;

        // Calculate interpolated vector values
        D3DXVECTOR3 vecPos = Anim->m_TranslationKeys[Key2].m_vecKey - 
                             Anim->m_TranslationKeys[Key1].m_vecKey;
        vecPos *= Scalar;
        vecPos += Anim->m_TranslationKeys[Key1].m_vecKey;

        // Create translation matrix and combine with transformation
        D3DXMATRIX matTranslation;
        D3DXMatrixTranslation(&matTranslation, vecPos.x, vecPos.y, vecPos.z);
        Anim->m_Bone->TransformationMatrix *= matTranslation;
      }

      // Matrix
      if(Anim->m_NumMatrixKeys && Anim->m_MatrixKeys) {
        // Loop for matching matrix key
        DWORD Key1 = 0, Key2 = 0;
        for(DWORD i=0;i<Anim->m_NumMatrixKeys;i++) {
          if(Time >= Anim->m_MatrixKeys[i].m_Time)
            Key1 = i;
        }

        // Get 2nd key number
        Key2 = (Key1>=(Anim->m_NumMatrixKeys-1))?Key1:Key1+1;

        // Get difference in keys' times
        DWORD TimeDiff = Anim->m_MatrixKeys[Key2].m_Time-
                         Anim->m_MatrixKeys[Key1].m_Time;
        if(!TimeDiff)
          TimeDiff = 1;

        // Calculate a scalar value to use
        float Scalar = (float)(Time - Anim->m_MatrixKeys[Key1].m_Time) / (float)TimeDiff;

        // Calculate interpolated matrix
        D3DXMATRIX matDiff = Anim->m_MatrixKeys[Key2].m_matKey - 
                             Anim->m_MatrixKeys[Key1].m_matKey;
        matDiff *= Scalar;
        matDiff += Anim->m_MatrixKeys[Key1].m_matKey;

        // Combine with transformation
        Anim->m_Bone->TransformationMatrix *= matDiff;
      }
    }

    // Go to next animation
    Anim = Anim->m_Next;
  }
}
