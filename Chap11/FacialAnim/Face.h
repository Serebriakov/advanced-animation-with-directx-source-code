#ifndef _FACE_H_
#define _FACE_H_

#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"
#include "XParser.h"
#include "initguid.h"

// Morphing mesh FVF
#define BLENDFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// Load and free vertex shader used in rendering
HRESULT LoadBlendingShader(IDirect3DDevice9 *pDevice);
void FreeBlendingShader();

void DrawBlendedMesh(D3DXMESHCONTAINER_EX *BaseMesh,
                     D3DXMESHCONTAINER_EX *Mesh1, float Blend1,
                     D3DXMESHCONTAINER_EX *Mesh2, float Blend2,
                     D3DXMESHCONTAINER_EX *Mesh3, float Blend3,
                     D3DXMESHCONTAINER_EX *Mesh4, float Blend4);

// Phoneme structure
typedef struct {
  DWORD Code;       // Phoneme #
  DWORD StartTime;  // Start time in milliseconds
  DWORD EndTime;    // End time in milliseconds
} sPhoneme;

// Phoneme parser class
class cXPhonemeParser : public cXParser
{
  protected:
     BOOL ParseObject(IDirectXFileData *pDataObj,            \
                       IDirectXFileData *pParentDataObj,      \
                       DWORD Depth,                           \
                       void **Data, BOOL Reference);
  public:
    char     *m_Name;         // Name of sequence
    DWORD     m_NumPhonemes;  // # phonemes in sequence
    sPhoneme *m_Phonemes;     // Array of phonemes
    DWORD     m_Length;       // Length (milliseconds) of sequence

  public:
    cXPhonemeParser();
    ~cXPhonemeParser();
  
    // Free loaded resources
    void Free();

    // Find the phoneme at specific time
    DWORD FindPhoneme(DWORD Time);

    // Get mesh #'s and time scale values
    void  GetAnimData(DWORD Time,                             \
                     DWORD *Phoneme1, float *Phoneme1Time,    \
                     DWORD *Phoneme2, float *Phoneme2Time);
};

#endif
