#include "Route.h"

// {F8569BED-53B6-4923-AF0B-59A09271D556}
DEFINE_GUID(Path,
            0xf8569bed, 0x53b6, 0x4923, 
            0xaf, 0xb, 0x59, 0xa0, 0x92, 0x71, 0xd5, 0x56);

// {18AA1C92-16AB-47a3-B002-6178F9D2D12F}
DEFINE_GUID(Route,
            0x18aa1c92, 0x16ab, 0x47a3, 
            0xb0, 0x2, 0x61, 0x78, 0xf9, 0xd2, 0xd1, 0x2f);


BOOL cXRouteParser::ParseObject(IDirectXFileData *pDataObj, \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference)
{
  const GUID *Type = GetObjectGUID(pDataObj);

  // Only process Route templates
  if(*Type == Route) {


    // Get pointer to data
    DWORD *DataPtr = (DWORD*)GetObjectData(pDataObj, NULL);

    // Allocate and link in a route
    cRoute *Route = new cRoute();
    Route->m_Next = m_Route;
    m_Route = Route;

    // Get name of route
    Route->m_Name = GetObjectName(pDataObj);

    // Get # paths in route and allocate list
    Route->m_NumPaths = *DataPtr++;
    Route->m_Paths = new sPath[Route->m_NumPaths];

    // Get path data
    for(DWORD i=0;i<Route->m_NumPaths;i++) {
      // Get path type
      Route->m_Paths[i].Type = *DataPtr++;

      // Get vectors
      D3DXVECTOR3 *vecPtr = (D3DXVECTOR3*)DataPtr;
      DataPtr+=12; // skip ptr ahead
      Route->m_Paths[i].vecStart  = *vecPtr++;
      Route->m_Paths[i].vecPoint1 = *vecPtr++;
      Route->m_Paths[i].vecPoint2 = *vecPtr++;
      Route->m_Paths[i].vecEnd    = *vecPtr++;

      // Calculate path length based on type
      if(Route->m_Paths[i].Type == PATH_STRAIGHT) {
        Route->m_Paths[i].Length = D3DXVec3Length(            \
                  &(Route->m_Paths[i].vecEnd -                \
                    Route->m_Paths[i].vecStart));
      } else {
        float Length01 = D3DXVec3Length(                      \
                  &(Route->m_Paths[i].vecPoint1 -            \
                    Route->m_Paths[i].vecStart));
        float Length12 = D3DXVec3Length(                      \
                  &(Route->m_Paths[i].vecPoint2 -            \
                    Route->m_Paths[i].vecPoint1));
        float Length23 = D3DXVec3Length(                      \
                  &(Route->m_Paths[i].vecEnd -                \
                    Route->m_Paths[i].vecPoint2));
        float Length03 = D3DXVec3Length(                      \
                  &(Route->m_Paths[i].vecEnd -                \
                    Route->m_Paths[i].vecStart));
        Route->m_Paths[i].Length = (Length01+Length12+         \
                               Length23)*0.5f+Length03*0.5f;
      }
      // Store starting position of path
      if(i)
        Route->m_Paths[i].Start = Route->m_Paths[i-1].Start + \
                                  Route->m_Paths[i-1].Length;
      else
        Route->m_Paths[i].Start = 0.0f;
    }
  }

  // Parse child objects
  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}

void cXRouteParser::Load(char *Filename)
{
  Free(); // Free loaded routes
  Parse(Filename);
}

void cXRouteParser::Locate(char *Name, float Distance, D3DXVECTOR3 *vecPos)
{
  // Find route in list
  cRoute *Route = m_Route->Find(Name);
  if(!Route)
    return;

  // Scan through each path in route
  for(DWORD i=0;i<Route->m_NumPaths;i++) {

    // See if distance falls into current path
    if(Distance >= Route->m_Paths[i].Start &&                  \
       Distance < Route->m_Paths[i].Start +                    \
                  Route->m_Paths[i].Length) {
      // Distance is within current path, use that
      // Get offset into path using start
      Distance -= Route->m_Paths[i].Start;
      
      // Calculate the scalar value to use
      float Scalar = (float)Distance/Route->m_Paths[i].Length;
      
      // Calculate coordinate based on path type
      if(Route->m_Paths[i].Type == PATH_STRAIGHT) {
        *vecPos = (Route->m_Paths[i].vecEnd -                 \
                   Route->m_Paths[i].vecStart) *              \
                   Scalar + Route->m_Paths[i].vecStart;
      } else {
        CubicBezierCurve(&Route->m_Paths[i].vecStart,         \
                         &Route->m_Paths[i].vecPoint1,       \
                         &Route->m_Paths[i].vecPoint2,       \
                         &Route->m_Paths[i].vecEnd,           \
                          Scalar, vecPos);
      }
    }
  }
}

float cXRouteParser::GetLength(char *Name)
{
  // Find the pointer to the route in question
  cRoute *RoutePtr = m_Route->Find(Name);
  if(!RoutePtr)
    return 0.0f;

  // Compute the total length of all paths
  float Length = 0.0f;
  for(DWORD i=0;i<RoutePtr->m_NumPaths;i++)
    Length += RoutePtr->m_Paths[i].Length;

  // Return length 
  return Length;
}

void CubicBezierCurve(D3DXVECTOR3 *vecPoint1,
                      D3DXVECTOR3 *vecPoint2,
                      D3DXVECTOR3 *vecPoint3,
                      D3DXVECTOR3 *vecPoint4,
                      float Scalar, 
                      D3DXVECTOR3 *vecOut)
{
  // C(s) =
  *vecOut =
    // P0 * (1 - s)3 +
    (*vecPoint1)*(1.0f-Scalar)*(1.0f-Scalar)*(1.0f-Scalar) +
    // P1 * 3 * s * (1 - s)2 +
    (*vecPoint2)*3.0f*Scalar*(1.0f-Scalar)*(1.0f-Scalar) +
    // P2 * 3 * s2 * (1 - s) +
    (*vecPoint3)*3.0f*Scalar*Scalar*(1.0f-Scalar) +
    // P3 * s3 
    (*vecPoint4)*Scalar*Scalar*Scalar;
}
