#ifndef _CLOTH_H_
#define _CLOTH_H_

#include <windows.h>
#include <stdio.h>
#include "Direct3D.h"
#include "XParser.h"
#include "XFile.h"
#include "initguid.h"
#include "Collision.h"

// Generic vertex structure that contains coordinates
typedef struct {
  D3DXVECTOR3 vecPos;
} sClothVertexPos;

// Class to contain information about cloth points
class cClothPoint
{
  public:
    D3DXVECTOR3 m_vecOriginalPos;  // Original position of point
    D3DXVECTOR3 m_vecPos;          // Current point coords

    D3DXVECTOR3 m_vecForce;        // Force applied to point
    D3DXVECTOR3 m_vecVelocity;     // Velocity of point

    float       m_Mass;         // Mass of object (0=pinned)
    float       m_OneOverMass;  // 1/Mass
};

// Class to contain information about springs
class cClothSpring
{
  public:
    DWORD         m_Point1;  // First point in spring
    DWORD         m_Point2;  // Second point in spring
    float         m_RestingLength;  // Resting length of spring

    float         m_Ks;      // Spring constant value
    float         m_Kd;      // Spring damping value

    cClothSpring *m_Next;    // Next in linked list

  public:
    cClothSpring()  { m_Next = NULL;                }
    ~cClothSpring() { delete m_Next; m_Next = NULL; }
};

// Class to contain a complete cloth mesh (w/.X parser)
class cClothMesh : public cXParser
{
  protected:
    DWORD             m_NumPoints;     // # points in cloth
    cClothPoint      *m_Points;        // Points

    DWORD             m_NumSprings;    // # springs in cloth
    cClothSpring     *m_Springs;       // Springs

    DWORD             m_NumFaces;      // # faces in mesh
    DWORD            *m_Faces;         // Faces

    DWORD             m_VertexStride;  // Size of a vertex

  protected:
    // Parse an .X file for mass and spring data
    BOOL ParseObject(IDirectXFileData *pDataObj,
                       IDirectXFileData *pParentDataObj,
                       DWORD Depth,
                       void **Data, BOOL Reference);

  public:
    cClothMesh();
    ~cClothMesh();

    // Create cloth from supplied mesh pointer
    BOOL Create(ID3DXMesh *Mesh, char *PointSpringXFile = NULL);

    // Free cloth data
    void Free();

    // Set forces to apply to points
    void SetForces(float LinearDamping,
                   D3DXVECTOR3 *vecGravity,
                   D3DXVECTOR3 *vecWind,
                   D3DXMATRIX *matTransform,
                   BOOL TransformAllPoints);

    // Process forces
    void ProcessForces(float Elapsed);

    // Process collisions
    void ProcessCollisions(cCollision *Collision, 
                           D3DXMATRIX *matTransform);

    // Rebuild cloth mesh
    void RebuildMesh(ID3DXMesh *Mesh);

    // Reset points to original pose and reset forces
    void Reset();

    // Add a spring to list
    void AddSpring(DWORD Point1, DWORD Point2, 
                   float Ks = 8.0f, float Kd = 0.5f);

    // Set a point's mass
    void SetMass(DWORD Point, float Mass);

    // Functions to get point/spring/face data
    DWORD           GetNumPoints();
    cClothPoint    *GetPoints();

    DWORD           GetNumSprings();
    cClothSpring   *GetSprings();

    DWORD           GetNumFaces();
    DWORD          *GetFaces();
};

#endif
