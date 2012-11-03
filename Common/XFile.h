/*
  This file allows you to access D3DRM_XTEMPLATES
  without all the nasty 'already defined' errors
  for duplicate definitions. Just include XFile.h
  in any file that needs to access D3DRM_XTEMPLATES.

  If rmxftmpl.h is every updated, then you'll need to
  manually change the D3DRM_XTEMPLATE_BYTES value to match.
*/
#include "rmxfguid.h"
extern unsigned char D3DRM_XTEMPLATES[];
#define D3DRM_XTEMPLATE_BYTES 3278
