#include "SkeletalAnimBlend.h"

void cBlendedAnimationCollection::Blend(                      \
                         char *AnimationSetName,              \
                         DWORD Time, BOOL Loop,               \
                         float Blend)
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
      D3DXMATRIX matAnimation;
      D3DXMatrixIdentity(&matAnimation);

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
        matAnimation *= matScale;
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
        matAnimation *= matRotation;
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
        matAnimation *= matTranslation;
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
        matAnimation *= matDiff;
      }

      // Get the difference in transformations 
      D3DXMATRIX matDiff = matAnimation - Anim->m_Bone->matOriginal;

      // Adjust by blending amount
      matDiff *= Blend;

      // Add to transformation matrix
      Anim->m_Bone->TransformationMatrix += matDiff;
    }

    // Go to next animation
    Anim = Anim->m_Next;
  }
}
