#ifndef _SKELETALANIMBLEND_H_
#define _SKELETALANIMBLEND_H_

#include <windows.h>
#include "Direct3D.h"
#include "SkeletalAnim.h"

class cBlendedAnimationCollection : public cAnimationCollection
{
  public:
    void Blend(char *AnimationSetName, 
               DWORD Time, BOOL Loop,
               float Blend = 1.0f);
};

#endif
