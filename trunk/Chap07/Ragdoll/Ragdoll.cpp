#include "Ragdoll.h"

///////////////////////////////////////////////////////////
//
// Protected functions
//
///////////////////////////////////////////////////////////
D3DXVECTOR3 cRagdoll::CrossProduct(D3DXVECTOR3 *v1, 
                                   D3DXVECTOR3 *v2)
{
  D3DXVECTOR3 vecResult;

  // Calculate the cross product
  D3DXVec3Cross(&vecResult, v1, v2);

  // Return the result
  return vecResult;
}

D3DXVECTOR3 cRagdoll::Transform(D3DXVECTOR3 *vecSrc, 
                                D3DXMATRIX  *matSrc,
                                D3DXVECTOR3 *vecTranslate)
{
  D3DXVECTOR3 vecResult;

  // Perform transformation
  D3DXVec3TransformCoord(&vecResult, vecSrc, matSrc);

  // Translate if vector passed
  if(vecTranslate)
    vecResult += (*vecTranslate);

  // Return result
  return vecResult;
}

void cRagdoll::GetBoundingBoxSize(D3DXFRAME_EX *pFrame,
                                  D3DXMESHCONTAINER_EX *pMesh,
                                  D3DXVECTOR3 *vecSize,
                                  D3DXVECTOR3 *vecJointOffset)
{
  // Set default min and max coordinates
  D3DXVECTOR3 vecMin = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  D3DXVECTOR3 vecMax = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Only process bone vertices if there is a bone to work with
  if(pFrame->Name) {

    // Get a pointer to ID3DXSkinInfo interface for
    // easier handling.
    ID3DXSkinInfo *pSkin = pMesh->pSkinInfo;

    // Search for a bone by same name as frame
    DWORD BoneNum = -1;
    for(DWORD i=0;i<pSkin->GetNumBones();i++) {
      if(!strcmp(pSkin->GetBoneName(i), pFrame->Name)) {
        BoneNum = i;
        break;
      }
    }

    // Process vertices if a bone was found
    if(BoneNum != -1) {

      // Get the number of vertices attached
      DWORD NumVertices = pSkin->GetNumBoneInfluences(BoneNum);
      if(NumVertices) {

        // Get the bone influcences
        DWORD *Vertices = new DWORD[NumVertices];
        float *Weights  = new float[NumVertices];
        pSkin->GetBoneInfluence(BoneNum, Vertices, Weights);

        // Get stride of vertex data
        DWORD Stride = D3DXGetFVFVertexSize(pMesh->MeshData.pMesh->GetFVF());

        // Get bone's offset inversed transformation matrix
        D3DXMATRIX *matInvBone = pSkin->GetBoneOffsetMatrix(BoneNum);

        // Lock vertex buffer and go through all of
        // the vertices that are connected to bone
        char *pVertices;
        pMesh->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&pVertices);
        for(i=0;i<NumVertices;i++) {

          // Get pointer to vertex coordinates
          D3DXVECTOR3 *vecPtr = (D3DXVECTOR3*)(pVertices+Vertices[i]*Stride);

          // Transform vertex by bone offset transformation
          D3DXVECTOR3 vecPos;
          D3DXVec3TransformCoord(&vecPos, vecPtr, matInvBone);
    
          // Get min/max values
          vecMin.x = min(vecMin.x, vecPos.x);
          vecMin.y = min(vecMin.y, vecPos.y);
          vecMin.z = min(vecMin.z, vecPos.z);

          vecMax.x = max(vecMax.x, vecPos.x);
          vecMax.y = max(vecMax.y, vecPos.y);
          vecMax.z = max(vecMax.z, vecPos.z);
        }
        pMesh->MeshData.pMesh->UnlockVertexBuffer();

        // Free resource
        delete [] Vertices;
        delete [] Weights;
      }
    }
  }

  // Factor in child bone connection points to size
  if(pFrame->pFrameFirstChild) {
  
    // Get the bone's inverse transformation to 
    // position child connections.
    D3DXMATRIX matInvFrame;
    D3DXMatrixInverse(&matInvFrame,NULL,&pFrame->matCombined);
  
    // Go through all child frames connected to this frame
    D3DXFRAME_EX *pFrameChild = (D3DXFRAME_EX*)pFrame->pFrameFirstChild;
    while(pFrameChild) {
      // Get the frame's vertex coordinates and transform it
      D3DXVECTOR3 vecPos;
      vecPos = D3DXVECTOR3(pFrameChild->matCombined._41,
                           pFrameChild->matCombined._42,
                           pFrameChild->matCombined._43);
      D3DXVec3TransformCoord(&vecPos, &vecPos, &matInvFrame);

      // Get min/max values
      vecMin.x = min(vecMin.x, vecPos.x);
      vecMin.y = min(vecMin.y, vecPos.y);
      vecMin.z = min(vecMin.z, vecPos.z);

      vecMax.x = max(vecMax.x, vecPos.x);
      vecMax.y = max(vecMax.y, vecPos.y);
      vecMax.z = max(vecMax.z, vecPos.z);

      // Go to next child bone
      pFrameChild = (D3DXFRAME_EX*)pFrameChild->pFrameSibling;
    }
  }

  // Set the bounding box size
  vecSize->x = (float)fabs(vecMax.x - vecMin.x);
  vecSize->y = (float)fabs(vecMax.y - vecMin.y);
  vecSize->z = (float)fabs(vecMax.z - vecMin.z);

  // Make sure each bone has a minimal size
  if(vecSize->x < MINIMUM_BONE_SIZE) {
    vecSize->x = MINIMUM_BONE_SIZE; 
    vecMax.x = MINIMUM_BONE_SIZE*0.5f;
  }
  if(vecSize->y < MINIMUM_BONE_SIZE) {
    vecSize->y = MINIMUM_BONE_SIZE; 
    vecMax.y = MINIMUM_BONE_SIZE*0.5f;
  }
  if(vecSize->z < MINIMUM_BONE_SIZE) {
    vecSize->z = MINIMUM_BONE_SIZE; 
    vecMax.z = MINIMUM_BONE_SIZE*0.5f;
  }

  // Set the bone's offset to center based on half the size 
  // of the bounding box and the max position
  (*vecJointOffset) = ((*vecSize) * 0.5f) - vecMax;
}

void cRagdoll::BuildBoneData(DWORD *BoneNum, 
                             D3DXFRAME_EX *Frame, 
                             D3DXMESHCONTAINER_EX *pMesh,
                             cRagdollBone *ParentBone)
{
  // Don't handle NULL frames
  if(!Frame)
    return;

  // Get pointer to bone for easier handling
  cRagdollBone *Bone = &m_Bones[(*BoneNum)];

  // Set pointer to frame
  Bone->m_Frame = Frame;

  // Get the size of this bone and its joint offset
  GetBoundingBoxSize(Bone->m_Frame,
                     pMesh,
                     &Bone->m_vecSize, 
                     &Bone->m_vecJointOffset);

  // Set mass
  Bone->m_Mass = Bone->m_vecSize.x * Bone->m_vecSize.y * Bone->m_vecSize.z;

  // Mass must be > 0.0
  if(Bone->m_Mass == 0.0f)
    Bone->m_Mass = 1.0f;

  // Set the coefficient of restitution (higher = more bounce)
  Bone->m_Coefficient = 0.4f;

  // Clear force and torque vectors
  Bone->m_vecForce        = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  Bone->m_vecTorque       = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Set the angular resolution rate
  Bone->m_ResolutionRate = 0.05f;

  // Calculate the inverse body inertia tensor
  float xScalar = Bone->m_vecSize.x * Bone->m_vecSize.x;
  float yScalar = Bone->m_vecSize.y * Bone->m_vecSize.y;
  float zScalar = Bone->m_vecSize.z * Bone->m_vecSize.z;
  D3DXMatrixIdentity(&Bone->m_matInvInertiaTensor);
  Bone->m_matInvInertiaTensor._11 = 1.0f / (Bone->m_Mass * (yScalar + zScalar));
  Bone->m_matInvInertiaTensor._22 = 1.0f / (Bone->m_Mass * (xScalar + zScalar));
  Bone->m_matInvInertiaTensor._33 = 1.0f / (Bone->m_Mass * (xScalar + yScalar));

  // Setup the point's coordinates based on half 
  // the bounding box size
  D3DXVECTOR3 vecHalfSize = Bone->m_vecSize * 0.5f;
  Bone->m_vecPoints[0] = D3DXVECTOR3(-vecHalfSize.x,  vecHalfSize.y, -vecHalfSize.z);
  Bone->m_vecPoints[1] = D3DXVECTOR3(-vecHalfSize.x,  vecHalfSize.y,  vecHalfSize.z);
  Bone->m_vecPoints[2] = D3DXVECTOR3( vecHalfSize.x,  vecHalfSize.y,  vecHalfSize.z);
  Bone->m_vecPoints[3] = D3DXVECTOR3( vecHalfSize.x,  vecHalfSize.y, -vecHalfSize.z);
  Bone->m_vecPoints[4] = D3DXVECTOR3(-vecHalfSize.x, -vecHalfSize.y, -vecHalfSize.z);
  Bone->m_vecPoints[5] = D3DXVECTOR3(-vecHalfSize.x, -vecHalfSize.y,  vecHalfSize.z);
  Bone->m_vecPoints[6] = D3DXVECTOR3( vecHalfSize.x, -vecHalfSize.y,  vecHalfSize.z);
  Bone->m_vecPoints[7] = D3DXVECTOR3( vecHalfSize.x, -vecHalfSize.y, -vecHalfSize.z);

  // Set the joint offset (offset to parent bone connection)
  Bone->m_vecPoints[8] = Bone->m_vecJointOffset;

  // Set the bone's center position based on transformed 
  // joint offset coordinates
  D3DXVec3TransformCoord(&Bone->m_State.m_vecPosition,
                         &(-1.0f * Bone->m_vecJointOffset),
                         &Bone->m_Frame->matCombined);

  // Store state matrix-based orientation. Be sure
  // to remove translation values.
  Bone->m_State.m_matOrientation = Bone->m_Frame->matCombined;
  Bone->m_State.m_matOrientation._41 = 0.0f;
  Bone->m_State.m_matOrientation._42 = 0.0f;
  Bone->m_State.m_matOrientation._43 = 0.0f;

  // Store state quaternion-based orientation
  // We need to inverse it the quaternion due to the fact 
  // that we're using a left-handed coordinates system
  D3DXQuaternionRotationMatrix(&Bone->m_State.m_quatOrientation, 
                               &Bone->m_Frame->matCombined);
  D3DXQuaternionInverse(&Bone->m_State.m_quatOrientation, 
                        &Bone->m_State.m_quatOrientation);

  // Clear angular momentum
  Bone->m_State.m_vecAngularMomentum = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Clear force and angular velocities
  Bone->m_State.m_vecLinearVelocity  = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  Bone->m_State.m_vecAngularVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Clear inverse world intertia tensor matrix
  D3DXMatrixIdentity(&Bone->m_State.m_matInvWorldInertiaTensor);

  // Transform points
  for(DWORD j=0;j<9;j++)
    Bone->m_State.m_vecPoints[j] = Transform(&Bone->m_vecPoints[j],
                                             &Bone->m_State.m_matOrientation,
                                             &Bone->m_State.m_vecPosition);

  // Get the difference in orientations from parent bone
  // to bone. This is used to later slerp for angular 
  // resolution. The difference (C) from A to B 
  // is calculated as: C = inverse(A) * B
  if(ParentBone) {
    D3DXQUATERNION quatInv;
    D3DXQuaternionInverse(&quatInv, &ParentBone->m_State.m_quatOrientation);
    Bone->m_quatOrientation = quatInv * Bone->m_State.m_quatOrientation;
  }

  // If there is a parent bone, then set connection position
  if((Bone->m_ParentBone = ParentBone)) {

    // Get the inversed coordinates from the joint connection point
    // to the center of the parent's bone.
    D3DXMATRIX matInv;
    D3DXMatrixInverse(&matInv, NULL, &ParentBone->m_State.m_matOrientation);
    D3DXVECTOR3 vecDiff = Bone->m_State.m_vecPoints[8] - 
                          ParentBone->m_State.m_vecPosition;
    Bone->m_vecParentOffset = Transform(&vecDiff, &matInv);
  }

  // Go to next bone
  (*BoneNum)+=1;

  // Process sibling frames
  if(Frame->pFrameSibling)
    BuildBoneData(BoneNum, (D3DXFRAME_EX*)Frame->pFrameSibling, pMesh, ParentBone);

  // Process child frames
  if(Frame->pFrameFirstChild)
    BuildBoneData(BoneNum, (D3DXFRAME_EX*)Frame->pFrameFirstChild, pMesh, Bone);
}

void cRagdoll::SetForces(DWORD BoneNum,
                         D3DXVECTOR3 *vecGravity, 
                         float LinearDamping, 
                         float AngularDamping)
{
  // Get a pointer to the bone for easier handling
  cRagdollBone *Bone = &m_Bones[BoneNum];

  // Get the pointer to the current state for easier handling
  cRagdollBoneState *BCState = &Bone->m_State;

  // Set gravity and clear torque
  Bone->m_vecForce  = ((*vecGravity) * Bone->m_Mass);
  Bone->m_vecTorque = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Apply damping on force and torque
  Bone->m_vecForce  += (BCState->m_vecLinearVelocity  * LinearDamping);
  Bone->m_vecTorque += (BCState->m_vecAngularVelocity * AngularDamping);
}

void cRagdoll::Integrate(DWORD BoneNum, float Elapsed)
{
  // Get pointer to bone
  cRagdollBone *Bone = &m_Bones[BoneNum];

  // Get pointers to states for easier handling
  cRagdollBoneState *State = &Bone->m_State;

  // Integrate position
  State->m_vecPosition += (Elapsed * State->m_vecLinearVelocity);
    
  // Integrate angular momentum
  State->m_vecAngularMomentum += (Elapsed * Bone->m_vecTorque);

  // Integrate linear velocity
  State->m_vecLinearVelocity += Elapsed * Bone->m_vecForce / Bone->m_Mass;

  // Integrate quaternion orientation
  D3DXVECTOR3 vecVelocity = Elapsed * State->m_vecAngularVelocity;
  State->m_quatOrientation.w -= 0.5f * 
                     (State->m_quatOrientation.x * vecVelocity.x + 
                      State->m_quatOrientation.y * vecVelocity.y + 
                      State->m_quatOrientation.z * vecVelocity.z);
  State->m_quatOrientation.x += 0.5f * 
                     (State->m_quatOrientation.w * vecVelocity.x - 
                      State->m_quatOrientation.z * vecVelocity.y + 
                      State->m_quatOrientation.y * vecVelocity.z);
  State->m_quatOrientation.y += 0.5f * 
                     (State->m_quatOrientation.z * vecVelocity.x + 
                      State->m_quatOrientation.w * vecVelocity.y - 
                      State->m_quatOrientation.x * vecVelocity.z);
  State->m_quatOrientation.z += 0.5f * 
                     (State->m_quatOrientation.x * vecVelocity.y - 
                      State->m_quatOrientation.y * vecVelocity.x + 
                      State->m_quatOrientation.w * vecVelocity.z);

  // Normalize the quaternion (creates a unit quaternion)
  D3DXQuaternionNormalize(&State->m_quatOrientation, 
                          &State->m_quatOrientation);

  // Force rotation resolution
  if(BoneNum && Bone->m_ResolutionRate != 0.0f) {

    // Slerp from current orientation to beginning orientation
    D3DXQUATERNION quatOrientation = Bone->m_ParentBone->m_State.m_quatOrientation *
                                     Bone->m_quatOrientation;
    D3DXQuaternionSlerp(&State->m_quatOrientation, 
                        &State->m_quatOrientation, 
                        &quatOrientation, Bone->m_ResolutionRate);
  }

  // Compute the new matrix-based orientation transformation 
  // based on the quaternion just computed
  D3DXMatrixRotationQuaternion(&State->m_matOrientation, 
                               &State->m_quatOrientation);
  D3DXMatrixTranspose(&State->m_matOrientation,
                      &State->m_matOrientation);

  // Calculate the integrated inverse world inertia tensor 
  D3DXMATRIX matTransposedOrientation;
  D3DXMatrixTranspose(&matTransposedOrientation, &State->m_matOrientation);
  State->m_matInvWorldInertiaTensor = State->m_matOrientation * 
                                      Bone->m_matInvInertiaTensor *
                                      matTransposedOrientation;

  // Calculate new angular velocity 
  State->m_vecAngularVelocity = Transform(&State->m_vecAngularMomentum, 
                                          &State->m_matInvWorldInertiaTensor);
}

DWORD cRagdoll::ProcessCollisions(DWORD BoneNum, cCollision *pCollision)
{
  // Error checking
  if(!pCollision || !pCollision->m_NumObjects || !pCollision->m_Objects)
    return TRUE;

  // Get a pointer to the bone for easier handling
  cRagdollBone *Bone = &m_Bones[BoneNum];

  // Get a pointer to the integrated state for easier handling
  cRagdollBoneState *State = &Bone->m_State;

  // Keep count of number of collisions
  DWORD CollisionCount = 0;

  // Keep tally of collision forces
  D3DXVECTOR3 vecLinearVelocity  = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  D3DXVECTOR3 vecAngularMomentum = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

  // Go through all bone vertices looking for a collision
  for(DWORD i=0;i<8;i++) {

    // Loop through all collision objects
    cCollisionObject *pObj = pCollision->m_Objects;
    while(pObj) {

      // Flag if a collision was detected
      BOOL Collision = FALSE;

      // Normal of collision object
      D3DXVECTOR3 vecCollisionNormal;

      // Distance to push point out of collision object
      float CollisionDistance = 0.0f;

      // Process sphere collision object
      if(pObj->m_Type == COLLISION_SPHERE) {

        // Get position of sphere
        D3DXVECTOR3 vecSphere = pObj->m_vecPos;
  
        // Get distance from center of sphere to point
        D3DXVECTOR3 vecDiff = State->m_vecPoints[i] - vecSphere;
        float Dist = vecDiff.x * vecDiff.x + 
                     vecDiff.y * vecDiff.y + 
                     vecDiff.z * vecDiff.z;

        // Process collision if point within radius
        if(Dist <= (pObj->m_Radius * pObj->m_Radius)) {

          // Compute real distance
          Dist = (float)sqrt(Dist);

          // Store the normalize difference vector as the collision normal
          vecCollisionNormal = vecDiff / Dist;

          // Calculate the distance needed to push point out
          CollisionDistance = pObj->m_Radius - Dist;

          // Mark there was a collision
          Collision = TRUE;
        }
      }

      // Process plane collision object
      if(pObj->m_Type == COLLISION_PLANE) {

        // Get position of plane 
        D3DXPLANE Plane = pObj->m_Plane;

        // Get the plane's normal
        D3DXVECTOR3 vecPlane = D3DXVECTOR3(Plane.a, Plane.b, Plane.c);

        // Get distance from plane to point
        float Dist = D3DXVec3Dot(&State->m_vecPoints[i], 
                                 &vecPlane) + Plane.d;

        // Process collision if point on backside of plane
        if(Dist < 0.0f) {

          // Calculate the distance needed to push point out
          CollisionDistance = -Dist;

          // Set plane's normal 
          vecCollisionNormal = vecPlane;

          // Mark there was a collision
          Collision = TRUE;
        }
      }

      // Process a collision if detected
      if(Collision == TRUE) {

        // Push the object onto the collision object's surface 
        State->m_vecPosition += (vecCollisionNormal * CollisionDistance);

        // Get the point's position and velocity
        D3DXVECTOR3 vecPtoP = State->m_vecPosition - State->m_vecPoints[i];
        D3DXVECTOR3 vecPtoPVelocity = State->m_vecLinearVelocity + 
                                      CrossProduct(&State->m_vecAngularVelocity, 
                                                   &vecPtoP);

        // Get the point's speed relative to the surface
        float PointSpeed = D3DXVec3Dot(&vecCollisionNormal, &vecPtoPVelocity);

        // Increase number of collisions
        CollisionCount++;

        // Calculate the impulse force based on the coefficient
        // of restitution, the speed of the point, and the
        // normal of the colliding object.
        float ImpulseForce = PointSpeed * (-(1.0f + Bone->m_Coefficient));
        float ImpulseDamping = (1.0f / Bone->m_Mass) +
                                   D3DXVec3Dot(&CrossProduct(&Transform(&CrossProduct(&vecPtoP, 
                                                                                      &vecCollisionNormal),
                                                                        &State->m_matInvWorldInertiaTensor),
                                                              &vecPtoP), 
                                               &vecCollisionNormal);
        D3DXVECTOR3 vecImpulse = (ImpulseForce/ImpulseDamping) * vecCollisionNormal;

        // Add forces to running total
        vecLinearVelocity  += vecImpulse;
        vecAngularMomentum += CrossProduct(&vecPtoP, &vecImpulse);
      }

      // Go to next collision object to check
      pObj = pObj->m_Next;
    }
  }

  // Was there any collisions
  if(CollisionCount) {

    // Add averaged forces to integrated state
    State->m_vecLinearVelocity  += ((vecLinearVelocity / Bone->m_Mass) / (float)CollisionCount);
    State->m_vecAngularMomentum += (vecAngularMomentum / (float)CollisionCount);

    // Calculate angular velocity
    State->m_vecAngularVelocity = Transform(&State->m_vecAngularMomentum,
                                             &State->m_matInvWorldInertiaTensor);
  }

  // Return that everything processed okay
  return TRUE;
}

void cRagdoll::ProcessConnections(DWORD BoneNum)
{
  // Get a pointer to the bone and 
  // parent bone for easier handling
  cRagdollBone *Bone = &m_Bones[BoneNum];
  cRagdollBone *ParentBone = Bone->m_ParentBone;

  // Don't continue if there's no parent bone
  if(!ParentBone)
    return;

  // Get the pointer to the bone's state
  cRagdollBoneState *BState = &Bone->m_State;

  // Get pointer to parent's state
  cRagdollBoneState *PState = &ParentBone->m_State;

  // Get joint connection position and vector to center
  D3DXVECTOR3 vecBonePos = BState->m_vecPoints[8];
  D3DXVECTOR3 vecBtoC    = BState->m_vecPosition - vecBonePos;

  // Get the parent connection point coordinates
  D3DXVECTOR3 vecParentPos = BState->m_vecPoints[9];

  // Calculate a spring vector from point to parent's point
  D3DXVECTOR3 vecSpring = vecBonePos - vecParentPos;

  // Move point to match parent's point and adjust 
  // the angular velocity and momentum
  BState->m_vecPosition -= vecSpring;
  BState->m_vecAngularMomentum -= CrossProduct(&vecBtoC, &vecSpring);
  BState->m_vecAngularVelocity = Transform(&BState->m_vecAngularMomentum,
                                           &BState->m_matInvWorldInertiaTensor);
}

void cRagdoll::TransformPoints(DWORD BoneNum)
{
  // Get pointer to bone and bone's state
  cRagdollBone *Bone = &m_Bones[BoneNum];
  cRagdollBoneState *State = &Bone->m_State;

  // Transform all points
  for(DWORD i=0;i<9;i++)
    State->m_vecPoints[i] = Transform(&Bone->m_vecPoints[i], 
                                      &State->m_matOrientation,
                                      &State->m_vecPosition);

  // Calculate parent connection point coordinates (in world space)
  if(Bone->m_ParentBone) {
    State->m_vecPoints[9] = Transform(&Bone->m_vecParentOffset,
                                      &Bone->m_ParentBone->m_State.m_matOrientation,
                                      &Bone->m_ParentBone->m_State.m_vecPosition);
  }
}

///////////////////////////////////////////////////////////
//
// Public functions
//
///////////////////////////////////////////////////////////
cRagdoll::cRagdoll()
{
  m_pFrame = NULL;

  m_NumBones = 0;
  m_Bones    = NULL;
}

cRagdoll::~cRagdoll()
{
  Free();
}

BOOL cRagdoll::Create(D3DXFRAME_EX *Frame, 
                      D3DXMESHCONTAINER_EX *Mesh,
                      D3DXMATRIX *matInitialTransformation)
{
  // Error checking
  if(!(m_pFrame = Frame))
    return FALSE;
  if(!Mesh->pSkinInfo || !Mesh->pSkinInfo->GetNumBones())
    return FALSE;

  // Update the frame hierarchy using transformation passed
  m_pFrame->UpdateHierarchy(matInitialTransformation);

  // Get the number of bones (frames) and allocate the ragdoll bone structures
  m_pFrame->Count(&m_NumBones);
  if(!m_NumBones)
    return FALSE;
  m_Bones = new cRagdollBone[m_NumBones]();

  // Go through and setup each bone's data
  DWORD BoneNum = 0;
  BuildBoneData(&BoneNum, m_pFrame, Mesh);

  m_Bones[0].m_State.m_vecAngularMomentum = D3DXVECTOR3(-1.0f, 0.0f, 0.0f);

  // Return a success
  return TRUE;
}

void cRagdoll::Free()
{
  m_pFrame = NULL;

  m_NumBones = 0;
  delete [] m_Bones; 
  m_Bones = NULL;
}

void cRagdoll::Resolve(float Elapsed, 
                       float LinearDamping, 
                       float AngularDamping,
                       D3DXVECTOR3 *vecGravity, 
                       cCollision  *pCollision)
{
  for(DWORD i=0;i<m_NumBones;i++) {
   
    float TimeToProcess = Elapsed;

    while(TimeToProcess > 0.0f) {

      // Set forces to prepare for integration
      SetForces(i, vecGravity, LinearDamping, AngularDamping);

      // Integrate bone movement for time slice
      DWORD NumSteps = 0;
      float TimeStep = TimeToProcess;

      // Dont process more than 10ms at a time
      if(TimeStep >= MAXIMUM_TIME_SLICE)
        TimeStep = MAXIMUM_TIME_SLICE;

      // Integrate the bone motion
      Integrate(i, TimeStep);

      // Transform points
      TransformPoints(i);

      // Check for collisions and resolve them, breaking if
      // all collisions could be handled
      ProcessCollisions(i, pCollision);

      // Transform points
      TransformPoints(i);

      // Process connections to ensure all bones
      // meet at their connection points
      ProcessConnections(i);

      // Go forward one time slice
      TimeToProcess -= TimeStep;
    }
  }
}

void cRagdoll::RebuildHierarchy()
{ 
  if(!m_pFrame)
    return;
  if(!m_NumBones || !m_Bones)
    return;

  // Apply bones' rotational matrices to frames
  for(DWORD i=0;i<m_NumBones;i++) {

    // Transform the joint offset in order to position frame
    D3DXVECTOR3 vecPos;
    D3DXVec3TransformCoord(&vecPos, 
                           &m_Bones[i].m_vecJointOffset, 
                           &m_Bones[i].m_State.m_matOrientation);

    // Add bone's position 
    vecPos += m_Bones[i].m_State.m_vecPosition;

    // Orient and position frame
    m_Bones[i].m_Frame->matCombined = m_Bones[i].m_State.m_matOrientation;
    m_Bones[i].m_Frame->matCombined._41 = vecPos.x;
    m_Bones[i].m_Frame->matCombined._42 = vecPos.y;
    m_Bones[i].m_Frame->matCombined._43 = vecPos.z;
  }
}

DWORD cRagdoll::GetNumBones() 
{ 
  return m_NumBones; 
}

cRagdollBone *cRagdoll::GetBone(DWORD BoneNum) 
{ 
  if(BoneNum < m_NumBones)
    return &m_Bones[BoneNum]; 
  return NULL;
}
