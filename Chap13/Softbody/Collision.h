#ifndef _COLLISION_H_
#define _COLLISION_H_

#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"

#define TOLERANCE_SPHERE 1.0f
#define TOLERANCE_PLANE  1.0f

#define COLLISION_SPHERE    0
#define COLLISION_PLANE     1

class cCollisionObject {
  public:
    DWORD       m_Type;       // Type of object

    D3DXVECTOR3 m_vecPos;     // Sphere coordinates
    float       m_Radius;     // Sphere radius
    D3DXPLANE   m_Plane;      // Plane values

    cCollisionObject *m_Next;    // Next in linked list

  public:
    cCollisionObject()  { m_Next = NULL;                }
    ~cCollisionObject() { delete m_Next; m_Next = NULL; }
};

class cCollision {
  public:
    DWORD             m_NumObjects;  // # of objects in collection
    cCollisionObject *m_Objects;     // Object list

  public:
    cCollision();
    ~cCollision();

    void Free();
    void AddSphere(D3DXVECTOR3 *vecPos, float Radius);
    void AddPlane(D3DXPLANE *PlaneParam);
};

#endif
