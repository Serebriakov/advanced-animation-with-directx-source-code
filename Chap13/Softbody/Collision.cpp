#include "stdio.h"
#include "Collision.h"

//////////////////////////////////////////////////////////////
// cCollision class functions
//////////////////////////////////////////////////////////////
cCollision::cCollision()
{
  m_NumObjects = 0;
  m_Objects    = NULL;
}

cCollision::~cCollision()
{
  Free();
}

void cCollision::Free()
{
  // Delete linked list of objects
  delete m_Objects; m_Objects = NULL;
  m_NumObjects = 0;
}

void cCollision::AddSphere(D3DXVECTOR3 *vecPos, float Radius)
{
  // Allocate a new object
  cCollisionObject *Sphere = new cCollisionObject();

  // Set sphere data
  Sphere->m_Type   = COLLISION_SPHERE;
  Sphere->m_vecPos = (*vecPos);
  Sphere->m_Radius = Radius;

  // Link sphere into list and increase count
  Sphere->m_Next = m_Objects;
  m_Objects = Sphere;
  m_NumObjects++;
}

void cCollision::AddPlane(D3DXPLANE *PlaneParam)
{
  // Allocate a new object
  cCollisionObject *Plane = new cCollisionObject();

  // Set plane data
  Plane->m_Type  = COLLISION_PLANE;
  Plane->m_Plane = (*PlaneParam);

  // Link plane into list and increase count
  Plane->m_Next = m_Objects;
  m_Objects = Plane;
  m_NumObjects++;
}
