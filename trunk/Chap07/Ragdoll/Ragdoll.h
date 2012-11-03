#ifndef _RAGDOLL_H_
#define _RAGDOLL_H_

#include <stdio.h>
#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "Collision.h"

// Minimum size of bone
#define MINIMUM_BONE_SIZE  1.0f

// Maximum time-slice to use for processing (10ms)
#define MAXIMUM_TIME_SLICE 0.01f

class cRagdollBoneState 
{
  public:
    D3DXVECTOR3    m_vecPosition;        // Position
    D3DXQUATERNION m_quatOrientation;    // Orientation
    D3DXMATRIX     m_matOrientation;     // Orientation

    D3DXVECTOR3    m_vecAngularMomentum; // Angular momentum

    D3DXVECTOR3    m_vecLinearVelocity;  // Linear velocity
    D3DXVECTOR3    m_vecAngularVelocity; // Angular velocity
  
    // Transformed points, including connection-to-parent
    // position and parent-to-bone offset
    D3DXVECTOR3    m_vecPoints[10];

    // Body's inverse world moment of interia tensor matrix
    D3DXMATRIX     m_matInvWorldInertiaTensor;
};

class cRagdollBone
{
  public:
    // Frame that this bone is connected to
    D3DXFRAME_EX *m_Frame;
    
    // Size of bounding box
    D3DXVECTOR3 m_vecSize;

    // Mass and 1/Mass (one-over-mass)
    float m_Mass;
    float m_OneOverMass;

    // Coefficient of restitution value
    // 0  = no bounce
    // 1  = 'super' bounce
    // >1 = gain power in bounce
    float m_Coefficient;

    cRagdollBone *m_ParentBone;      // Pointer to parent bone

    // Connection-to-parent offset and 
    // parent-to-bone offset
    D3DXVECTOR3 m_vecJointOffset; 
    D3DXVECTOR3 m_vecParentOffset;

    // Linear force and angular momentum
    D3DXVECTOR3 m_vecForce;
    D3DXVECTOR3 m_vecTorque;

    // Original orientation of bone
    D3DXQUATERNION m_quatOrientation;

    // Rate of resolution (0-1) to resolve slerp interpolation
    // This is used to make bones return to their initial
    // orientation relative to its parent.
    float m_ResolutionRate;

    // Body's inverse moment of intertia tensor matrix
    D3DXMATRIX m_matInvInertiaTensor;

    // Points (in body space) that form bounding box 
    // and connection-to-parent offset position
    D3DXVECTOR3 m_vecPoints[9];
    
    // Bone state
    cRagdollBoneState m_State;
};

class cRagdoll
{
  protected:
    D3DXFRAME_EX         *m_pFrame;   // Frame hierarchy root

    DWORD                 m_NumBones; // # bones
    cRagdollBone         *m_Bones;    // Bone list

  protected:
    // Function to compute a cross product inline
    D3DXVECTOR3 CrossProduct(D3DXVECTOR3 *v1, D3DXVECTOR3 *v2);

    // Function to multiply a vector by a 3x3 matrix
    // and add an optional translation vector
    D3DXVECTOR3 Transform(D3DXVECTOR3 *vecSrc, 
                          D3DXMATRIX  *matSrc,
                          D3DXVECTOR3 *vecTranslate = NULL);

    // Get a frame's bone bounding box size and joint offset
    void GetBoundingBoxSize(D3DXFRAME_EX *pFrame,
                            D3DXMESHCONTAINER_EX *pMesh,
                            D3DXVECTOR3 *vecSize,
                            D3DXVECTOR3 *vecJointOffset);

    // Build a bone and set its data
    void BuildBoneData(DWORD *BoneNum, 
                       D3DXFRAME_EX *Frame,
                       D3DXMESHCONTAINER_EX *pMesh,
                       cRagdollBone *ParentBone = NULL);

    // Set gravity, damping, and joint forces
    void SetForces(DWORD BoneNum,
                   D3DXVECTOR3 *vecGravity, 
                   float LinearDamping, 
                   float AngularDamping);

    // Integrate bone motion for a time slice
    void Integrate(DWORD BoneNum, float Elapsed);

    // Process collisions
    DWORD ProcessCollisions(DWORD BoneNum, cCollision *pCollision);

    // Process bone connections
    void ProcessConnections(DWORD BoneNum);

    // Transform the state's points
    void TransformPoints(DWORD BoneNum);

  public:
    cRagdoll();
    ~cRagdoll();

    // Create ragdoll from supplied frame hierarchy pointer
    BOOL Create(D3DXFRAME_EX *Frame,
                D3DXMESHCONTAINER_EX *Mesh,
                D3DXMATRIX *matInitialTransformation = NULL);

    // Free ragdoll data
    void Free();

    // Resolve the ragdoll using gravity and damping
    void Resolve(float Elapsed,
                 float LinearDamping    = -0.04f, 
                 float AngularDamping   = -0.01f,
                 D3DXVECTOR3 *vecGravity  = &D3DXVECTOR3(0.0f, -9.8f, 0.0f),
                 cCollision *pCollision   = NULL);

    // Rebuild the frame hierarchy
    void RebuildHierarchy();

    // Functions to get the number of bones and 
    // retrieve a pointer to a specific bone
    DWORD GetNumBones();
    cRagdollBone *GetBone(DWORD BoneNum);
};

#endif
