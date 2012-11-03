#ifndef _XPARSER_H_
#define _XPARSER_H_

#include <windows.h>
#include "dxfile.h"
#include "XFile.h"

// A macro to quickly release and NULL a COM interface
#define XPReleaseCOM(x) { if(x) { x->Release(); x= NULL; } }

class cXParser
{
  protected:
    // Functions called when parsing begins and end
    virtual BOOL BeginParse(void **Data) { return TRUE; }
    virtual BOOL EndParse(void **Data)   { return TRUE; }

    // Function called for every template found
    virtual BOOL ParseObject(                               \
                   IDirectXFileData *pDataObj,                \
                   IDirectXFileData *pParentDataObj,          \
                   DWORD Depth,                               \
                   void **Data, BOOL Reference)
            { 
              return ParseChildObjects(pDataObj, Depth,     \
                                         Data, Reference);
            }

    // Function called to enumerate child templates
    BOOL ParseChildObjects(IDirectXFileData *pDataObj,      \
                             DWORD Depth, void **Data,        \
                             BOOL ForceReference = FALSE);


  public:
    // Function to start parsing an .X file
    BOOL Parse(char *Filename, void **Data = NULL);

    // Functions to help retrieve template information
    const GUID *GetObjectGUID(IDirectXFileData *pDataObj);
    char *GetObjectName(IDirectXFileData *pDataObj);
    void *GetObjectData(IDirectXFileData *pDataObj,         \
                          DWORD *Size);
};

#endif
