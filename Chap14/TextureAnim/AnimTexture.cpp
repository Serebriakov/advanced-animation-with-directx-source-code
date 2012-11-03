#include "AnimTexture.h"

cTextureFilter::cTextureFilter(IDirect3DDevice9 *pD3DDevice, 
                               LPUNKNOWN pUnk, HRESULT *phr) 
               : CBaseVideoRenderer(__uuidof(CLSID_AnimatedTexture), 
                 NAME("ANIMATEDTEXTURE"), pUnk, phr)
{
  // Save device pointer
  m_pD3DDevice  = pD3DDevice;

  // Return success
  *phr = S_OK;
}

HRESULT cTextureFilter::CheckMediaType(const CMediaType *pMediaType)
{
  // Only accept video type media
  if(*pMediaType->FormatType() != FORMAT_VideoInfo)
    return E_INVALIDARG;

  // Make sure media data is using RGB 24-bit color format
  if(IsEqualGUID(*pMediaType->Type(),    MEDIATYPE_Video) &&
     IsEqualGUID(*pMediaType->Subtype(), MEDIASUBTYPE_RGB24))
    return S_OK;

  return E_FAIL;
}

HRESULT cTextureFilter::SetMediaType(const CMediaType *pMediaType)
{
  HRESULT          hr;
  VIDEOINFO       *pVideoInfo;
  D3DSURFACE_DESC  ddsd;

  // Retrive the size of this media type
  pVideoInfo     = (VIDEOINFO *)pMediaType->Format();
  m_lVideoWidth  = pVideoInfo->bmiHeader.biWidth;
  m_lVideoHeight = abs(pVideoInfo->bmiHeader.biHeight);
  m_lVideoPitch  = (m_lVideoWidth * 3 + 3) & ~(3);

  // Create the texture that maps to this media type
  if(FAILED(hr = D3DXCreateTexture(m_pD3DDevice,
                 m_lVideoWidth, m_lVideoHeight,
                 1, 0, D3DFMT_A8R8G8B8, 
                 D3DPOOL_MANAGED, &m_pTexture)))
    return hr;

  // Get texture description and verify settings
  if(FAILED(hr = m_pTexture->GetLevelDesc(0, &ddsd)))
    return hr;
  m_Format = ddsd.Format;
  if(m_Format != D3DFMT_A8R8G8B8 && m_Format != D3DFMT_A1R5G5B5)
    return VFW_E_TYPE_NOT_ACCEPTED;

  return S_OK;
}

HRESULT cTextureFilter::DoRenderSample(IMediaSample *pMediaSample)
{
  // Get a pointer to video sample buffer
  BYTE *pSamplePtr;
  pMediaSample->GetPointer(&pSamplePtr);

  // Lock the texture surface
  D3DLOCKED_RECT d3dlr;
  if(FAILED(m_pTexture->LockRect(0, &d3dlr, 0, 0)))
    return E_FAIL;

  // Get texture pitch and pointer to texture data
  BYTE *pTexturePtr  = (BYTE*)d3dlr.pBits;
  LONG lTexturePitch = d3dlr.Pitch;

  // Offset texture to bottom line, since video 
  // is stored upside down in buffer
  pTexturePtr += (lTexturePitch * (m_lVideoHeight-1));
  
  // Copy the bits using specified video format
  int x, y, SrcPos, DestPos;
  switch(m_Format) {
    case D3DFMT_A8R8G8B8:  // 32-bit

      // Loop through each row, copying bytes as you go
      for(y=0;y<m_lVideoHeight;y++) {

        // Copy each column
        SrcPos = DestPos = 0;
        for(x=0;x<m_lVideoWidth;x++) {
          pTexturePtr[DestPos++] = pSamplePtr[SrcPos++];
          pTexturePtr[DestPos++] = pSamplePtr[SrcPos++];
          pTexturePtr[DestPos++] = pSamplePtr[SrcPos++];
          pTexturePtr[DestPos++] = 0xff;
        }

        // Move pointers to next line
        pSamplePtr  += m_lVideoPitch;
        pTexturePtr -= lTexturePitch;
      }
      break;

    case D3DFMT_A1R5G5B5:  // 16-bit

      // Loop through each row, copying bytes as you go
      for(y=0;y<m_lVideoHeight;y++) {

        // Copy each column
        SrcPos = DestPos = 0;
        for(x=0;x<m_lVideoWidth;x++) {
          *(WORD*)pTexturePtr[DestPos++] = 0x8000 + 
                   ((pSamplePtr[SrcPos+2] & 0xF8) << 7) +
                   ((pSamplePtr[SrcPos+1] & 0xF8) << 2) +
                   (pSamplePtr[SrcPos] >> 3);
          SrcPos += 3;
        }
         
        // Move pointers to next line
        pSamplePtr  += m_lVideoPitch;
        pTexturePtr -= lTexturePitch;
      }
      break;
  }
      
  // Unlock the Texture
  if(FAILED(m_pTexture->UnlockRect(0)))
    return E_FAIL;

  return S_OK;
}

IDirect3DTexture9 *cTextureFilter::GetTexture()
{
  return m_pTexture;
}

cAnimatedTexture::cAnimatedTexture()
{
  // Clear class data
  m_pGraph         = NULL;
  m_pMediaControl  = NULL;
  m_pMediaPosition = NULL;
  m_pMediaEvent    = NULL;

  m_pD3DDevice     = NULL;
  m_pTexture       = NULL;
}

cAnimatedTexture::~cAnimatedTexture()
{
  Free();
}

BOOL cAnimatedTexture::Load(IDirect3DDevice9 *pDevice, char *Filename)
{
  cTextureFilter *pTextureFilter = NULL;
  IBaseFilter    *pFilter        = NULL;
  IPin           *pFilterPinIn   = NULL;
  IBaseFilter    *pSourceFilter  = NULL;
  IPin           *pSourcePinOut  = NULL;

  WCHAR           wFilename[MAX_PATH];
  HRESULT         hr;

  // Free prior instance
  Free();

  // Store device
  if((m_pD3DDevice = pDevice) == NULL)
    return FALSE;

  // Create the filter graph manager
  CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
                   IID_IGraphBuilder, (void **)&m_pGraph);

  // Add filter
  pTextureFilter = new cTextureFilter(m_pD3DDevice, NULL, &hr);
  pFilter = pTextureFilter;
  m_pGraph->AddFilter(pFilter, L"ANIMATEDTEXTURE");

  // Add source filter
  mbstowcs(wFilename, Filename, MAX_PATH);
  m_pGraph->AddSourceFilter(wFilename, L"SOURCE", &pSourceFilter);

  // Find the input and out pins and connect together
  pFilter->FindPin(L"In", &pFilterPinIn);
  pSourceFilter->FindPin(L"Output", &pSourcePinOut);
  m_pGraph->Connect(pSourcePinOut, pFilterPinIn);

  // Query for interfaces
  m_pGraph->QueryInterface(IID_IMediaControl,  (void **)&m_pMediaControl);
  m_pGraph->QueryInterface(IID_IMediaPosition, (void **)&m_pMediaPosition);
  m_pGraph->QueryInterface(IID_IMediaEvent,    (void **)&m_pMediaEvent);

  // Get pointer to texture
  m_pTexture = pTextureFilter->GetTexture();

  // Start the graph running
  Play();

  // Free interfaces used here
  ReleaseCOM(pFilterPinIn);
  ReleaseCOM(pSourceFilter);
  ReleaseCOM(pSourcePinOut);

  return TRUE;
}

BOOL cAnimatedTexture::Free()
{
  // Stop any playback
  Stop();

  // Release all COM objects
  ReleaseCOM(m_pMediaControl);
  ReleaseCOM(m_pMediaEvent);
  ReleaseCOM(m_pMediaPosition);
  ReleaseCOM(m_pGraph);

  // Release the texture
  ReleaseCOM(m_pTexture);

  // Clear Direct3D object pointer
  m_pD3DDevice = NULL;

  return TRUE;
}

BOOL cAnimatedTexture::Update()
{
  long lEventCode;
  long lParam1;
  long lParam2;

  // Error checking
  if(!m_pMediaEvent)
    return FALSE;

  // Process all waiting events
  while(1) {

    // Get the event
    if(FAILED(m_pMediaEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 1)))
      break;

    // Call the end of animation function if playback complete
    if(lEventCode == EC_COMPLETE)
      EndOfAnimation();

    // Free the event resources
    m_pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);
  }
  
  return TRUE;
}

BOOL cAnimatedTexture::EndOfAnimation()
{
  return Restart();
}

BOOL cAnimatedTexture::Play()
{
  if(m_pMediaControl != NULL)
    m_pMediaControl->Run();

  return TRUE;
}

BOOL cAnimatedTexture::Stop()
{
  if(m_pMediaControl != NULL)
    m_pMediaControl->Stop();
  return TRUE;
}

BOOL cAnimatedTexture::Restart()
{
  return GotoTime(0);
}

BOOL cAnimatedTexture::GotoTime(REFTIME Time)
{
  if(m_pMediaPosition != NULL)
    m_pMediaPosition->put_CurrentPosition(Time);
  return TRUE;
}

IDirect3DTexture9 *cAnimatedTexture::GetTexture()
{
  return m_pTexture;
}
