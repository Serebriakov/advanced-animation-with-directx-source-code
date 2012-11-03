#include "XParser.h"

BOOL cXParser::Parse(char *Filename, void **Data)
{
  IDirectXFile           *pDXFile = NULL;
  IDirectXFileEnumObject *pDXEnum = NULL;
  IDirectXFileData       *pDXData = NULL;

  // Error checking
  if(Filename == NULL)
    return FALSE;

  // Create the file object
  if(FAILED(DirectXFileCreate(&pDXFile)))
    return FALSE;

  // Register the common templates
  if(FAILED(pDXFile->RegisterTemplates(                       \
                     (LPVOID)D3DRM_XTEMPLATES,                \
                     D3DRM_XTEMPLATE_BYTES))) {
    pDXFile->Release();
    return FALSE;
  }

  // Create an enumeration object
  if(FAILED(pDXFile->CreateEnumObject((LPVOID)Filename,       \
                                      DXFILELOAD_FROMFILE,    \
                                      &pDXEnum))) {
    pDXFile->Release();
    return FALSE;
  }
  
  // Call the begin parse function, continuing if allowed
  if(BeginParse(Data) == TRUE) {

    // Loop through all top-level objects, breaking on errors
    BOOL ParseResult;
    while(SUCCEEDED(pDXEnum->GetNextDataObject(&pDXData))) {
      ParseResult = ParseObject(pDXData, NULL, 0, Data, FALSE);
      XPReleaseCOM(pDXData);
      if(ParseResult == FALSE)
        break;
    }
  }

  // Call end parse function
  EndParse(Data);

  // Release used COM objects
  XPReleaseCOM(pDXEnum);
  XPReleaseCOM(pDXFile);

  return TRUE;
}

BOOL cXParser::ParseChildObjects(                           \
                         IDirectXFileData *pDataObj,          \
                         DWORD Depth, void **Data,            \
                         BOOL ForceReference)
{
  IDirectXFileObject        *pSubObj  = NULL;
  IDirectXFileData          *pSubData = NULL;
  IDirectXFileDataReference *pDataRef = NULL;
  BOOL                       ParseResult = TRUE;

  // Scan for embedded templates
  while(SUCCEEDED(pDataObj->GetNextObject(&pSubObj))) {

    // Process embedded references
    if(SUCCEEDED(pSubObj->QueryInterface(                     \
                              IID_IDirectXFileDataReference,  \
                              (void**)&pDataRef))) {

      // Resolve the data object
      if(SUCCEEDED(pDataRef->Resolve(&pSubData))) {

        // Parse the object, remembering the return code
        ParseResult = ParseObject(pSubData, pDataObj,       \
                                    Depth+1, Data, TRUE);
        XPReleaseCOM(pSubData);
      }
      XPReleaseCOM(pDataRef);

      // Return on parsing failure
      if(ParseResult == FALSE)
        return FALSE;
    } else

    // Process non-referenced embedded templates
    if(SUCCEEDED(pSubObj->QueryInterface(                     \
                              IID_IDirectXFileData,           \
                              (void**)&pSubData))) {

      // Parse the object, remembering the return code
      ParseResult = ParseObject(pSubData, pDataObj,         \
                                  Depth+1, Data,              \
                                  ForceReference);
      XPReleaseCOM(pSubData);
    }

    // Release the data object
    XPReleaseCOM(pSubObj);

    // Return on parsing failure
    if(ParseResult == FALSE)
      return FALSE;
  }

  return TRUE;
}

const GUID *cXParser::GetObjectGUID(                        \
                        IDirectXFileData *pDataObj)
{
  const GUID *Type = NULL;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get the template type
  if(FAILED(pDataObj->GetType(&Type)))
    return NULL;

  return Type;
}

char *cXParser::GetObjectName(IDirectXFileData *pDataObj)
{
  char  *Name = NULL;
  DWORD  Size = 0;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get the template name (if any)
  if(FAILED(pDataObj->GetName(NULL, &Size)))
    return NULL;

  // Allocate a name buffer and retrieve name
  if(Size) {
    if((Name = new char[Size]) != NULL)
      pDataObj->GetName(Name, &Size);
  }

  return Name;
}

void *cXParser::GetObjectData(                              \
                  IDirectXFileData *pDataObj,                 \
                  DWORD *Size)
{
  void *TemplateData = NULL;
  DWORD TemplateSize = 0;

  // Error checking
  if(pDataObj == NULL)
    return NULL;

  // Get a data pointer to template
  pDataObj->GetData(NULL,&TemplateSize,(PVOID*)&TemplateData);

  // Save size if needed
  if(Size != NULL)
    *Size = TemplateSize;

  return TemplateData;
}
