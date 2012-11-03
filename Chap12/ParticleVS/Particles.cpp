#include "Particles.h"

inline DWORD FLOAT2DWORD(FLOAT f) { return *((DWORD*)&f); }

// Vertex shader declaration
D3DVERTEXELEMENT9 g_VSParticleDecl[] =
{
  { 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
  { 0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
  { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
  { 0, 24, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
  D3DDECL_END()
};

// Clear static variables to their defaults
DWORD                        cParticleEmitter::m_RefCount  = 0;
IDirect3DVertexShader9      *cParticleEmitter::m_pShader   = NULL;
IDirect3DVertexDeclaration9 *cParticleEmitter::m_pDecl     = NULL;
IDirect3DTexture9          **cParticleEmitter::m_pTextures = NULL;

cParticleEmitter::cParticleEmitter()
{
  // Clear object pointers
  m_pDevice      = NULL;
  m_VB           = NULL;
  m_IB           = NULL;

  // Clear particle data
  m_NumParticles = 0;
  m_Particles    = NULL;
}

cParticleEmitter::~cParticleEmitter()
{
  Free();  // Free particles
}

BOOL cParticleEmitter::Create(IDirect3DDevice9 *pDevice,
                              D3DXVECTOR3 *vecPosition,
                              DWORD EmitterType,
                              DWORD NumParticlesPerBuffer)
{
  // Error checking
  if(!(m_pDevice = pDevice))
    return FALSE;

  // Save emitter's position
  m_vecPosition = (*vecPosition);

  // Save type of emitter
  m_EmitterType = EmitterType;

  // Save # of particles in buffer
  m_NumParticles = NumParticlesPerBuffer;

  // Load textures if none already loaded
  if(m_pTextures == NULL) {

    m_pTextures = new IDirect3DTexture9*[NUM_PARTICLE_TYPES];
    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_Smoke.bmp",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[0]);

    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_Tree1.dds",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[1]);

    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_Tree2.dds",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[2]);

    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_Tree3.dds",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[3]);

    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_People1.bmp",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[4]);

    D3DXCreateTextureFromFileEx(m_pDevice, 
                                "..\\Data\\Particle_People2.bmp",
                                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                0, D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                D3DX_DEFAULT, D3DX_DEFAULT,
                                D3DCOLOR_RGBA(0,0,0,255), NULL, NULL, 
                                &m_pTextures[5]);
  }
  
  // Create the vertex buffer
  m_pDevice->CreateVertexBuffer(m_NumParticles * 4 * sizeof(sShaderVertex),
                                D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, 
                                D3DPOOL_DEFAULT, &m_VB, 0);

  // Create the index buffer and fill with face data
  m_pDevice->CreateIndexBuffer(m_NumParticles * 6 * sizeof(short),
                               D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                               D3DPOOL_DEFAULT, &m_IB, 0);
  unsigned short *Ptr;
  m_IB->Lock(0, 0, (void**)&Ptr, 0);
  for(unsigned short i=0;i<m_NumParticles;i++) {
    Ptr[i*6+0] = i * 4 + 0;
    Ptr[i*6+1] = i * 4 + 1;
    Ptr[i*6+2] = i * 4 + 2;
    Ptr[i*6+3] = i * 4 + 3;
    Ptr[i*6+4] = i * 4 + 2;
    Ptr[i*6+5] = i * 4 + 1;
  }
  m_IB->Unlock();

  // Load the vertex shader if not already done
  if(!m_pShader) {
    LoadVertexShader(&m_pShader, m_pDevice, "Particle.vsh", g_VSParticleDecl, &m_pDecl);
    if(!m_pShader) {
      ReleaseCOM(m_pDecl);
      return FALSE;
    }
  }

  // Increase class reference count
  m_RefCount++;

  // Return success
  return TRUE;
}

void cParticleEmitter::Free()
{
  // Decrease reference count and free resources if needed
  if(m_RefCount)
    m_RefCount--;
  if(!m_RefCount) {

    // Release textures
    if(m_pTextures) {
      for(DWORD i=0;i<NUM_PARTICLE_TYPES;i++)
        ReleaseCOM(m_pTextures[i]);
      delete [] m_pTextures;
      m_pTextures = NULL;
    }

    // Release shader data
    ReleaseCOM(m_pShader);
    ReleaseCOM(m_pDecl);
  }

  // Clear object pointers
  m_pDevice = NULL;
  ReleaseCOM(m_VB);
  ReleaseCOM(m_IB);

  // Clear particle data
  m_NumParticles = 0;
  delete m_Particles;
  m_Particles = NULL;
}

void cParticleEmitter::Add(DWORD Type, D3DXVECTOR3 *vecPos, float Size, DWORD Color, DWORD Life, D3DXVECTOR3 *vecVelocity)
{
  // Allocate a particle object and add to head of list
  cParticle *Particle = new cParticle();
  Particle->m_Prev = NULL;
  if((Particle->m_Next = m_Particles))
    m_Particles->m_Prev = Particle;
  m_Particles = Particle;

  // Set particle data
  Particle->m_Type   = Type;
  Particle->m_vecPos = (*vecPos);
  Particle->m_Size   = Size;
  Particle->m_Color  = Color;
  Particle->m_Life   = Life;
  Particle->m_vecVelocity = (*vecVelocity);
}

void cParticleEmitter::ClearAll()
{
  // Clear particle data
  delete m_Particles;
  m_Particles = NULL;
}

BOOL cParticleEmitter::Begin(D3DXMATRIX *matView, D3DXMATRIX *matProj)
{ 
  // Error checking
  if(!m_pDevice || !m_pShader || !m_pDecl || !matView || !matProj)
    return FALSE;

  // Set vertex shader, declaration, and stream sources
  m_pDevice->SetFVF(NULL);
  m_pDevice->SetVertexShader(m_pShader);
  m_pDevice->SetVertexDeclaration(m_pDecl);

  // Get view * proj transformation, transpose, and store it
  D3DXMATRIX matVP = (*matView) * (*matProj);
  D3DXMatrixTranspose(&matVP, &matVP);
  m_pDevice->SetVertexShaderConstantF(0, (float*)&matVP,  4);

  // Get right and up vectors from view transformation and store as constants
  D3DXVECTOR4 vecRight, vecUp;
  D3DXVec4Normalize(&vecRight, &D3DXVECTOR4(matView->_11, matView->_21, matView->_31, 0.0f));
  D3DXVec4Normalize(&vecUp,    &D3DXVECTOR4(matView->_12, matView->_22, matView->_32, 0.0f));
  m_pDevice->SetVertexShaderConstantF(4, (float*)&vecRight, 1);
  m_pDevice->SetVertexShaderConstantF(5, (float*)&vecUp,    1);

  return TRUE;
}

void cParticleEmitter::Render()
{
  DWORD LastTexture = -1;

  // Error checking
  if(!m_pDevice || !m_VB || !m_Particles)
    return;

  // MUST CALL BEGIN BEFORE CALLING RENDER!!!

  // Set the stream sources
  m_pDevice->SetStreamSource(0, m_VB, 0, sizeof(sShaderVertex));
  m_pDevice->SetIndices(m_IB);

  // Go through each type of particle to draw
  // Chance to optimize - speed this up
  for(DWORD i=0;i<NUM_PARTICLE_TYPES;i++) {

    // Start at first particle
    cParticle *Particle = m_Particles;

    // Set a count for flushing buffer
    DWORD Num = 0;

    // Lock the vertex buffer for use
    sShaderVertex *Ptr;
    m_VB->Lock(0, 0, (void**)&Ptr, D3DLOCK_DISCARD);

    // Loop for all particles
    while(Particle != NULL) {

      // Do types match?
      if(Particle->m_Type == i) {

        // Set the type's texture (if not done already)
        if(i != LastTexture) {
          LastTexture = i;
          m_pDevice->SetTexture(0, m_pTextures[i]);
        }

        // Copy over particle data
        float HalfSize = Particle->m_Size * 0.5f;
        Ptr[0].vecPos    = Particle->m_vecPos;
        Ptr[0].vecOffset = D3DXVECTOR2(-HalfSize, HalfSize);
        Ptr[0].Diffuse   = Particle->m_Color;
        Ptr[0].u         = 0.0f;
        Ptr[0].v         = 0.0f;
        Ptr[1].vecPos    = Particle->m_vecPos;
        Ptr[1].vecOffset = D3DXVECTOR2(HalfSize, HalfSize);
        Ptr[1].Diffuse   = Particle->m_Color;
        Ptr[1].u         = 1.0f;
        Ptr[1].v         = 0.0f;
        Ptr[2].vecPos    = Particle->m_vecPos;
        Ptr[2].vecOffset = D3DXVECTOR2(-HalfSize, -HalfSize);
        Ptr[2].Diffuse   = Particle->m_Color;
        Ptr[2].u         = 0.0f;
        Ptr[2].v         = 1.0f;
        Ptr[3].vecPos    = Particle->m_vecPos;
        Ptr[3].vecOffset = D3DXVECTOR2(HalfSize, -HalfSize);
        Ptr[3].Diffuse   = Particle->m_Color;
        Ptr[3].u         = 1.0f;
        Ptr[3].v         = 1.0f;
        Ptr+=4;

        // Increase vertex count and flush buffer if full
        Num++;
        if(Num >= m_NumParticles) {

          // Unlock buffer and render polygons
          m_VB->Unlock();
          m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, Num*4, 0, Num*2);

          // Lock vertex buffer again
          m_VB->Lock(0, 0, (void**)&Ptr, D3DLOCK_DISCARD);

          // Clear vertex count
          Num=0;
        }
      }

      // Go to next particle
      Particle = Particle->m_Next;
    }

    // Unlock vertex buffer
    m_VB->Unlock();

    // Render any polygons left
    if(Num)
      m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, Num*4, 0, Num*2);
  }
}

void cParticleEmitter::End()
{
  m_pDevice->SetVertexShader(NULL);
  m_pDevice->SetVertexDeclaration(NULL);
}

void cParticleEmitter::Process(DWORD Elapsed)
{
  cParticle *Particle = m_Particles;

  // Loop through all particles
  while(Particle != NULL) {

    // Remember next particle
    cParticle *NextParticle = Particle->m_Next;
    
    // Set flag to remove from list
    BOOL RemoveFromList = FALSE;

    // Decrease life of particle, but not if life == 0
    if(Particle->m_Life) {
      if(Particle->m_Life <= Elapsed)
        RemoveFromList = TRUE;
      else
        Particle->m_Life -= Elapsed;
    }

    // Calculate scalar to use for velocity
    float Scalar = (float)Elapsed / 1000.0f;

    // Add velocity to particle positions
    Particle->m_vecPos += (Particle->m_vecVelocity * Scalar);

    // Remove particle from list if flagged
    if(RemoveFromList == TRUE) {

      // Have previous particle skip past one being deleted
      // or set new root if particle to be removed is the root
      if(Particle->m_Prev)
        Particle->m_Prev->m_Next = Particle->m_Next;
      else
        m_Particles = Particle->m_Next;

      // Set next particle's previous pointer
      if(Particle->m_Next)
        Particle->m_Next->m_Prev = Particle->m_Prev;

      // Delete particle
      Particle->m_Prev = NULL;
      Particle->m_Next = NULL;
      delete Particle;
    }

    // Go to next particle
    Particle = NextParticle;
  }
}


////////////////////////////////////////////////////////////////////////////
//
// Specialized particle functions
//
////////////////////////////////////////////////////////////////////////////
void cParticleEmitter::HandleSmoke(D3DXVECTOR3 *vecPos, DWORD Elapsed)
{
  static DWORD Timer = 0;

  // Update smoke timer and add a smoke particle as needed
  Timer += Elapsed;
  if(Timer > 66) {
    Timer = 0;

    // Pick a random direction to move particle
    float rot = (float)((rand() % 628) / 100.0f);
    Add(PARTICLE_SMOKE, &D3DXVECTOR3(vecPos->x, 5.0f, vecPos->z),
        10.0f, 0xFF222222, 1000, 
        &D3DXVECTOR3((float)cos(rot)*20.0f, 0.0f, (float)sin(rot)*20.0f));
  }
}

void cParticleEmitter::HandleDucking(D3DXVECTOR3 *vecPos)
{
  // Change people's particle types if chopper is close to them
  cParticle *Particle = m_Particles;
  while(Particle) {

    // Calculate distance from position to person (only on x/z axes)
    D3DXVECTOR3 vecDiff = Particle->m_vecPos - (*vecPos);
    float Dist = vecDiff.x*vecDiff.x + vecDiff.z*vecDiff.z;

    // If too close, make person duck, otherwise stand them up
    if(Dist < (40.0f * 40.0f))
      Particle->m_Type = PARTICLE_PEOPLE2;
    else
      Particle->m_Type = PARTICLE_PEOPLE1;

    // Go to next particle
    Particle = Particle->m_Next;
  }
}
