#ifndef _ANIMTEXTURE_H_
#define _ANIMTEXTURE_H_

#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "dshow.h"
#include "streams.h"

// Define a macro to help release COM objects
#ifndef ReleaseCOM
#define ReleaseCOM(x) { if(x!=NULL) x->Release(); x=NULL; }
#endif

struct __declspec(uuid("{61DA4980-0487-11d6-9089-00400536B95F}")) CLSID_AnimatedTexture;

class cTextureFilter : public CBaseVideoRenderer
{
  public:
    IDirect3DDevice9  *m_pD3DDevice;    // 3-D device
    IDirect3DTexture9 *m_pTexture;      // Video texture storage
    D3DFORMAT          m_Format;

    LONG               m_lVideoWidth;   // Width of video surface
    LONG               m_lVideoHeight;  // Height of video surface
    LONG               m_lVideoPitch;   // Pitch of video surface

  public:
    cTextureFilter(IDirect3DDevice9 *pD3DDevice,
                   LPUNKNOWN pUnk = NULL, 
                   HRESULT *phr = NULL);

    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT SetMediaType(const CMediaType *pMediaType);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);

    IDirect3DTexture9 *GetTexture();
};

class cAnimatedTexture
{
  protected:
    IGraphBuilder     *m_pGraph;          // Filter graph
    IMediaControl     *m_pMediaControl;   // Playback control
    IMediaPosition    *m_pMediaPosition;  // Positioning control
    IMediaEvent       *m_pMediaEvent;     // Event control

    IDirect3DDevice9  *m_pD3DDevice;      // 3-D device
    IDirect3DTexture9 *m_pTexture;        // Texture object

  public:
    cAnimatedTexture();
    ~cAnimatedTexture();

    // Load and free an animated texture object
    BOOL Load(IDirect3DDevice9 *pDevice, char *Filename);
    BOOL Free();

    // Update the texture and check for looping
    BOOL Update();

    // Called at end of animation playback
    virtual BOOL EndOfAnimation();

    // Play and stop functions
    BOOL Play();
    BOOL Stop();

    // Restart animation or go to specific time
    BOOL Restart();
    BOOL GotoTime(REFTIME Time);

    // Return texture object pointer
    IDirect3DTexture9 *GetTexture();
};

#endif
