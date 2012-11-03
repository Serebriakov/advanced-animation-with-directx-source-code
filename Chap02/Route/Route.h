#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <windows.h>
#include "Direct3D.h"
#include "dxfile.h"
#include "XParser.h"
#include "XFile.h"
#include "initguid.h"

enum { PATH_STRAIGHT = 0, PATH_CURVED };

typedef struct {
  DWORD       Type;
  D3DXVECTOR3 vecStart,  vecEnd;
  D3DXVECTOR3 vecPoint1, vecPoint2;

  float Start;    // Starting position
  float Length;   // Length of path
} sPath;

class cRoute
{
  public:
    char   *m_Name;      // Name of route
    DWORD   m_NumPaths;  // # paths in list
    sPath  *m_Paths;     // List of paths
    cRoute *m_Next;      // Next route in linked list

  public:
    cRoute() { m_Name = NULL; m_Paths = NULL; m_Next = NULL; }
    ~cRoute() { delete [] m_Name; m_Name = NULL; delete [] m_Paths; delete m_Next; }
    
    // Find a route by name
    cRoute *Find(char *Name)
    {
      if(!Name)
        return this;
      if(!stricmp(Name, m_Name))
        return this;
      if(m_Next) {
        cRoute *Ptr = m_Next->Find(Name);
        if(Ptr)
          return Ptr;
      }
      return NULL;
    }
};

class cXRouteParser : public cXParser
{
  protected:
    BOOL ParseObject(IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference);
  public:
    cRoute *m_Route;

  public:
    cXRouteParser() { m_Route = NULL; }
    ~ cXRouteParser () { Free(); }
    void Free() { delete m_Route; m_Route = NULL; }
    void Load(char *Filename);
    void Locate(char *Name, float Distance, D3DXVECTOR3 *vecPos);
    float GetLength(char *Name);
};

void CubicBezierCurve(D3DXVECTOR3 *vecPoint1,
                      D3DXVECTOR3 *vecPoint2,
                      D3DXVECTOR3 *vecPoint3,
                      D3DXVECTOR3 *vecPoint4,
                      float Scalar, 
                      D3DXVECTOR3 *vecOut);

#endif
