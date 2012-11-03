#include "Cloth.h"

// Define cloth point mass template GUID
// template ClothMasses {
//   <F5AD0F93-9BF2-4bcf-B7CF-D68CD6B54156>
//   DWORD NumPoints;
//   array FLOAT Mass[NumPoints];
// }
DEFINE_GUID(ClothMasses, 
            0xf5ad0f93, 0x9bf2, 0x4bcf, 
            0xb7, 0xcf, 0xd6, 0x8c, 0xd6, 0xb5, 0x41, 0x56);

// Define cloth spring template GUID
// template ClothSprings {
//   <8C08B088-728E-46c8-BE87-72672B81DB11>
//   DWORD NumSprings;
//   DWORD NumVertices;  // NumSprings * 2
//   array DWORD Vertex[NumVertices];
// }
DEFINE_GUID(ClothSprings, 
            0x8c08b088, 0x728e, 0x46c8, 
            0xbe, 0x87, 0x72, 0x67, 0x2b, 0x81, 0xdb, 0x11);

//////////////////////////////////////////////////////////////
// cClothMesh class functions
//////////////////////////////////////////////////////////////
cClothMesh::cClothMesh()
{
  // Clear all class data
  m_NumPoints     = 0;
  m_Points        = NULL;

  m_NumSprings    = 0;
  m_Springs       = NULL;

  m_NumFaces      = 0;
  m_Faces         = NULL;

  m_VertexStride   = 0;
}

cClothMesh::~cClothMesh()
{
  Free();
}

BOOL cClothMesh::ParseObject(IDirectXFileData *pDataObj,
                               IDirectXFileData *pParentDataObj,
                               DWORD Depth,
                               void **Data, BOOL Reference)
{
  const GUID *Type = GetObjectGUID(pDataObj);
  char       *Name = GetObjectName(pDataObj);
  DWORD       Size;
  DWORD      *DataPtr = (DWORD*)GetObjectData(pDataObj, &Size);
  float      *MassPtr;

  // Read in cloth point masses - apply to current mesh
  if(*Type == ClothMasses && m_NumPoints && m_Points) {

    // Get number of assignments to follow
    // and make sure it matches # of points in cloth
    DWORD NumPoints = *DataPtr++;
    if(NumPoints == m_NumPoints) {

      // Copy over mass values
      MassPtr = (float*)DataPtr;
      for(DWORD i=0;i<NumPoints;i++)
        SetMass(i, *MassPtr++);
    }
  }

  // If springs exist, wipe out current ones and add new ones
  if(*Type == ClothSprings) {

    // Clear current springs
    delete m_Springs;
    m_Springs = NULL;
    m_NumSprings = 0;

    // Get new number of springs and vertices
    DWORD NumSprings  = *DataPtr++;
    DWORD NumVertices = *DataPtr++;

    // Load each spring
    for(DWORD i=0;i<NumSprings;i++) {
      DWORD Vertex1 = *DataPtr++;
      DWORD Vertex2 = *DataPtr++;
      AddSpring(Vertex1, Vertex2);
    }
  }

  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}

void cClothMesh::SetForces(float LinearDamping, 
                           D3DXVECTOR3 *vecGravity, 
                           D3DXVECTOR3 *vecWind,
                           D3DXMATRIX  *matTransform,
                           BOOL TransformAllPoints)
{
  DWORD i;

  // Error checking
  if(!m_NumPoints || m_Points == NULL)
    return;

  // Clear forces, apply transformation, set gravity, and apply linear damping
  for(i=0;i<m_NumPoints;i++) {

    // Clear force
    m_Points[i].m_vecForce = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    // Move point using transformation if specified
    if(matTransform && (TransformAllPoints == TRUE || m_Points[i].m_Mass == 0.0f)) {
      D3DXVec3TransformCoord(&m_Points[i].m_vecPos, &m_Points[i].m_vecOriginalPos, matTransform);
    }

    // Only apply gravity and linear damping to points w/mass
    if(m_Points[i].m_Mass != 0.0f) {

      // Apply gravity if specified
      if(vecGravity != NULL)
        m_Points[i].m_vecForce += (*vecGravity) * m_Points[i].m_Mass;

      // Apply linear damping
      m_Points[i].m_vecForce += (m_Points[i].m_vecVelocity * LinearDamping);
    }
  }

  // Apply wind
  if(vecWind != NULL && m_NumFaces) {

    // Go through each face and apply wind vector to
    // vertex on all faces
    for(i=0;i<m_NumFaces;i++) {

      // Get three vertices that construct face
      DWORD Vertex1 = m_Faces[i*3];
      DWORD Vertex2 = m_Faces[i*3+1];
      DWORD Vertex3 = m_Faces[i*3+2];

      // Calculate face's normal
      D3DXVECTOR3 vecV12 = m_Points[Vertex2].m_vecPos - m_Points[Vertex1].m_vecPos;
      D3DXVECTOR3 vecV13 = m_Points[Vertex3].m_vecPos - m_Points[Vertex1].m_vecPos;
      D3DXVECTOR3 vecNormal;
      D3DXVec3Cross(&vecNormal, &vecV12, &vecV13);
      D3DXVec3Normalize(&vecNormal, &vecNormal);

      // Get dot product between normal and wind
      float Dot = D3DXVec3Dot(&vecNormal, vecWind);

      // Amplify normal by dot product
      vecNormal *= Dot;

      // Apply normal to point's force vector
      m_Points[Vertex1].m_vecForce += vecNormal;
      m_Points[Vertex2].m_vecForce += vecNormal;
      m_Points[Vertex3].m_vecForce += vecNormal;
    }
  }

  // Process springs
  cClothSpring *Spring = m_Springs;
  while(Spring) { 

    // Get the current spring vector
    D3DXVECTOR3 vecSpring = m_Points[Spring->m_Point2].m_vecPos - 
                            m_Points[Spring->m_Point1].m_vecPos;

    // Get the current length of the spring
    float SpringLength = D3DXVec3Length(&vecSpring);

    // Get the relative velocity of the points
    D3DXVECTOR3 vecVelocity = m_Points[Spring->m_Point2].m_vecVelocity - 
                              m_Points[Spring->m_Point1].m_vecVelocity;
    float Velocity = D3DXVec3Dot(&vecVelocity, &vecSpring) / SpringLength;

    // Calculate forces
    float SpringForce  = Spring->m_Ks * (SpringLength - Spring->m_RestingLength);
    float DampingForce = Spring->m_Kd * Velocity;

    // Normalize the spring
    vecSpring /= SpringLength;

    // Calculate force vector
    D3DXVECTOR3 vecForce = (SpringForce + DampingForce) * vecSpring;

    // Apply force to vectors
    if(m_Points[Spring->m_Point1].m_Mass != 0.0f)
      m_Points[Spring->m_Point1].m_vecForce += vecForce;

    if(m_Points[Spring->m_Point2].m_Mass != 0.0f)
      m_Points[Spring->m_Point2].m_vecForce -= vecForce;

    // Go to next spring
    Spring = Spring->m_Next;
  }
}     

void cClothMesh::ProcessForces(float Elapsed)
{
  // Error checking
  if(!m_NumPoints || !m_Points)
    return;

  // Resolve forces on points
  for(DWORD i=0;i<m_NumPoints;i++) {

    // Points w/0 mass don't move
    if(m_Points[i].m_Mass != 0.0f) {

      // Update velocity
      m_Points[i].m_vecVelocity += (m_Points[i].m_OneOverMass * Elapsed * m_Points[i].m_vecForce);

      // Update position
      m_Points[i].m_vecPos += (Elapsed * m_Points[i].m_vecVelocity);
    }
  }
}

void cClothMesh::ProcessCollisions(cCollision *pCollision,
                                   D3DXMATRIX *matTransform)
{
  D3DXMATRIX matITTransform;
  BOOL MatrixInversedAndTransposed = FALSE;

  // Error checking
  if(!pCollision || !pCollision->m_NumObjects || !pCollision->m_Objects)
    return;

  // Go through all points
  for(DWORD i=0;i<m_NumPoints;i++) {

    // Don't process points w/0 mass
    if(m_Points[i].m_Mass != 0.0f) {

      // Go through each collision object
      cCollisionObject *pObject = pCollision->m_Objects;
      while(pObject) {
 
        // Check if point collides with a sphere object
        if(pObject->m_Type == COLLISION_SPHERE) {

          // Put sphere coordinates into local vector
          D3DXVECTOR3 vecSphere = pObject->m_vecPos;

          // Translate sphere by transformation
          if(matTransform) {
            vecSphere.x += matTransform->_41; // Translate x
            vecSphere.y += matTransform->_42; // Translate y
            vecSphere.z += matTransform->_43; // Translate z
          }

          // Calculate a distance vector
          D3DXVECTOR3 vecDist = vecSphere - m_Points[i].m_vecPos;

          // Get the squared length of the difference
          float Length = vecDist.x * vecDist.x +              \
                         vecDist.y * vecDist.y +              \
                         vecDist.z * vecDist.z;

          // If the length of the difference less than radius?
          if(Length <= (pObject->m_Radius*pObject->m_Radius)) {

            // Collision occurred!

            // Normalize the distance vector
            Length = (float)sqrt(Length);
            vecDist /= Length;

            // Calculate the difference in distance from the 
            // point to the sphere's edge
            float Diff = pObject->m_Radius - Length;

            // Scale the vector by difference
            vecDist *= Diff;

            // Push the point out and adjust velocity
            m_Points[i].m_vecPos      -= vecDist;
            m_Points[i].m_vecVelocity -= vecDist;
          }
        }

        // Check if point collides with a plane object
        if(pObject->m_Type == COLLISION_PLANE) {

          // Put plane in a local variable
          D3DXPLANE Plane = pObject->m_Plane;

          // Transform plane if needed
          if(matTransform) {

            // Inverse and transpose the transformation
            // but do it only once to save time
            if(MatrixInversedAndTransposed == FALSE) {
              MatrixInversedAndTransposed = TRUE;
              D3DXMatrixInverse(&matITTransform, NULL, matTransform);
              D3DXMatrixTranspose(&matITTransform, &matITTransform);
            }

            // Transform the plane
            D3DXPlaneTransform(&Plane, &Plane, &matITTransform);
          } 

          // Get the normal vector
          D3DXVECTOR3 vecNormal = D3DXVECTOR3(Plane.a,        \
                                              Plane.b,        \
                                              Plane.c);

          // Get the dot product between the plane's normal
          // and the point's position
          float Dot = D3DXVec3Dot(&m_Points[i].m_vecPos,      \
                                  &vecNormal) + Plane.d;

          // Check if point is behind plane
          if(Dot < 0.0f) {

            // Scale the plane's normal by the 
            // absolute dot product.
            vecNormal *= (-Dot);

            // Move point and adjust velocity by normal vector
            m_Points[i].m_vecPos      += vecNormal;
            m_Points[i].m_vecVelocity += vecNormal;
          }
        }

        // Go to next collision object
        pObject = pObject->m_Next;
      }
    }
  }
}

void cClothMesh::RebuildMesh(ID3DXMesh *Mesh)
{
  DWORD i;

  // Error checking
  if(!m_NumPoints || !m_Points || !Mesh)
    return;

  // Lock vertex buffer
  char *Vertices;
  Mesh->LockVertexBuffer(0, (void**)&Vertices);

  // Store each point in list
  for(i=0;i<m_NumPoints;i++) {

    // Get pointer to vertex data in buffer
    sClothVertexPos *Vertex = (sClothVertexPos*)Vertices;
    
    // Store vertex coordinates
    Vertex->vecPos = m_Points[i].m_vecPos;

    // Go to next vertex
    Vertices += m_VertexStride;
  }

  // Unlock vertex buffer
  Mesh->UnlockVertexBuffer();

  // Recompute normals
  D3DXComputeNormals(Mesh, NULL);
}

BOOL cClothMesh::Create(ID3DXMesh *Mesh, char *PointSpringXFile)
{
  DWORD i;

  // Free a prior mesh
  Free();

  // Error checking
  if(!Mesh)
    return FALSE;

  // Calculate vertex pitch (size of vertex data)
  m_VertexStride = D3DXGetFVFVertexSize(Mesh->GetFVF());

  ////////////////////////////////////////////////////////////
  // Calculate the spring information from the loaded mesh
  ////////////////////////////////////////////////////////////

  // Get the # of faces and allocate array
  m_NumFaces = Mesh->GetNumFaces();
  m_Faces = new DWORD[m_NumFaces*3];

  // Lock index buffer and copy over data (16-bit indices)
  unsigned short *Indices;
  Mesh->LockIndexBuffer(0, (void**)&Indices);
  for(i=0;i<m_NumFaces*3;i++)
    m_Faces[i] = (DWORD)*Indices++;
  Mesh->UnlockIndexBuffer();

  // Get the # of points in mesh and allocate structures
  m_NumPoints = Mesh->GetNumVertices();
  m_Points = new cClothPoint[m_NumPoints]();

  // Lock vertex buffer and stuff data into cloth points
  char *Vertices;
  Mesh->LockVertexBuffer(0, (void**)&Vertices);
  for(i=0;i<m_NumPoints;i++) {

    // Get pointer to vertex coordinates
    sClothVertexPos *Vertex = (sClothVertexPos*)Vertices;

    // Store position, velocity, force, and mass
    m_Points[i].m_vecOriginalPos = Vertex->vecPos;
    m_Points[i].m_vecPos      = m_Points[i].m_vecOriginalPos;
    m_Points[i].m_Mass        = 1.0f;
    m_Points[i].m_OneOverMass = 1.0f;

    // Setup point's states
    m_Points[i].m_vecVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_Points[i].m_vecForce =    D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    // Go to next vertex
    Vertices += m_VertexStride;
  }
  Mesh->UnlockVertexBuffer();

  // Build list of springs from face vertices
  for(i=0;i<m_NumFaces;i++) {

    // Get vertices that construct a face
    DWORD Vertex1 = m_Faces[i*3];
    DWORD Vertex2 = m_Faces[i*3+1];
    DWORD Vertex3 = m_Faces[i*3+2];

    // Add springs from 1->2, 2->3, and 3->1
    AddSpring(Vertex1, Vertex2);
    AddSpring(Vertex2, Vertex3);
    AddSpring(Vertex3, Vertex1);
  }

  // Parse cloth masses and springs from file
  if(PointSpringXFile)
    Parse(PointSpringXFile);

  return TRUE;
}

void cClothMesh::Free()
{
  m_NumPoints = 0;     // Delete cloth points array
  delete [] m_Points;
  m_Points = NULL;
  
  m_NumSprings = 0;    // Delete spring linked list
  delete m_Springs;
  m_Springs = NULL;

  m_NumFaces = 0;      // Delete faces array
  delete [] m_Faces;
  m_Faces = NULL;

  m_VertexStride = 0;   // Reset values
}

void cClothMesh::Reset()
{
  // Error checking
  if(!m_NumPoints || !m_Points)
    return;

  // Go through each point
  for(DWORD i=0;i<m_NumPoints;i++) {

    // Copy original points
    m_Points[i].m_vecPos = m_Points[i].m_vecOriginalPos;

    // Clear forces
    m_Points[i].m_vecVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
  }
}

void cClothMesh::AddSpring(DWORD Point1, DWORD Point2, float Ks, float Kd)
{
  cClothSpring *Spring = m_Springs;

  // Don't add springs that share the same point
  if(Point1 == Point2)
    return;

  // Search for a duplicate spring already in list
  while(Spring != NULL) {

    // Check if a match and return if so (no duplicates)
    if(Point1 == Spring->m_Point1 && Point2 == Spring->m_Point2)
      return;
    if(Point1 == Spring->m_Point2 && Point2 == Spring->m_Point1)
      return;

    // Make sure the two points are not the same
    if(Point1 == Point2)
      return;

    // Go to next spring
    Spring = Spring->m_Next;
  }

  // Allocate a spring and link to list (increasing count)
  Spring = new cClothSpring();
  Spring->m_Next = m_Springs;
  m_Springs = Spring;
  m_NumSprings++;

  // Set spring's data
  Spring->m_Point1 = Point1;
  Spring->m_Point2 = Point2;
  Spring->m_Ks     = Ks;
  Spring->m_Kd     = Kd;

  // Calculate resting length of spring
  D3DXVECTOR3 vecDist = m_Points[Point2].m_vecPos - 
                        m_Points[Point1].m_vecPos;
  Spring->m_RestingLength = D3DXVec3Length(&vecDist);
}

void cClothMesh::SetMass(DWORD Point, float Mass)
{
  // Only set mass if it's a valid point value
  if(Point < m_NumPoints) {
    m_Points[Point].m_Mass = Mass;
    m_Points[Point].m_OneOverMass = (Mass==0.0f)?0.0f:1.0f/Mass;
  }
}

DWORD cClothMesh::GetNumPoints()
{
  return m_NumPoints;
}

cClothPoint *cClothMesh::GetPoints()
{
  return m_Points;
}

DWORD cClothMesh::GetNumSprings()
{
  return m_NumSprings;
}

cClothSpring *cClothMesh::GetSprings()
{
  return m_Springs;
}

DWORD cClothMesh::GetNumFaces()
{
  return m_NumFaces;
}

DWORD *cClothMesh::GetFaces()
{
  return m_Faces;
}
