#include "Softbody.h"

void cSoftbodyMesh::Revert(float Stiffness, D3DXMATRIX *matTransform)
{
  // Error checking
  if(!m_NumPoints || m_Points == NULL)
    return;

  // Process softbody forces (revert shape)
  for(DWORD i=0;i<m_NumPoints;i++) {

    // Only process points that can move
    if(m_Points[i].m_Mass != 0.0f) {

      // Transform original coordinates if needed
      D3DXVECTOR3 vecPos = m_Points[i].m_vecOriginalPos;
      if(matTransform)
        D3DXVec3TransformCoord(&vecPos, &vecPos, matTransform);

      // Create a spring vector from original position
      // of point (transformed) to its current position
      D3DXVECTOR3 vecSpring = vecPos - m_Points[i].m_vecPos;

      // Scale spring by stiffness value
      vecSpring *= Stiffness;

      // Directly modify velocity and position
      m_Points[i].m_vecVelocity += vecSpring;
      m_Points[i].m_vecPos += vecSpring;
    }
  }
}
