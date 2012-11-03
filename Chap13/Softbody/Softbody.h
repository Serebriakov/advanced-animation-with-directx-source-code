#ifndef _SOFTBODY_H_
#define _SOFTBODY_H_

#include <windows.h>
#include <stdio.h>
#include "Direct3D.h"
#include "XParser.h"
#include "XFile.h"
#include "initguid.h"
#include "Cloth.h"
#include "Collision.h"

class cSoftbodyMesh : public cClothMesh
{
  public:
    void Revert(float Stiffness, D3DXMATRIX *matTransform);
};

#endif
