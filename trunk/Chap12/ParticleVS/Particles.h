#ifndef _PARTICLES_H_
#define _PARTICLES_H_

#include <windows.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "Direct3D.h"

// Particle emitter types
#define EMITTER_CLOUD       0
#define EMITTER_TREE        1
#define EMITTER_PEOPLE      2

// Particle types and # of particle types
#define PARTICLE_SMOKE      0
#define PARTICLE_TREE1      1
#define PARTICLE_TREE2      2
#define PARTICLE_TREE3      3
#define PARTICLE_PEOPLE1    4
#define PARTICLE_PEOPLE2    5
#define NUM_PARTICLE_TYPES  6

// Vertex structure for vertex-shader particles
typedef struct {
  D3DXVECTOR3 vecPos;     // Coordinates of particle
  D3DXVECTOR2 vecOffset;  // Corner vertex offset coordinates
  DWORD       Diffuse;    // Diffuse color of particle
  float       u, v;       // Texture coordinates
} sShaderVertex;

// Particle container class
class cParticle
{
  public:
    DWORD       m_Type;         // Type of particle

    D3DXVECTOR3 m_vecPos;       // Position of particle
    D3DXVECTOR3 m_vecVelocity;  // Velocity of particle

    DWORD       m_Life;         // Life of particle in ms
    float       m_Size;         // Size of particle

    DWORD       m_Color;        // Diffuse color of particle

    cParticle *m_Prev;          // Prev particle in linked list
    cParticle *m_Next;          // Next particle in linked list

  public:
    // Constructor and destructor to clear/release data
    cParticle()  { m_Prev = NULL; m_Next = NULL;                }
    ~cParticle() { delete m_Next; m_Next = NULL; m_Prev = NULL; }
};

// Particle emitter class
class cParticleEmitter
{
  protected:
    IDirect3DDevice9       *m_pDevice;       // Parent 3-D device
    
    DWORD                   m_EmitterType;   // Type of emitter

    IDirect3DVertexBuffer9 *m_VB;            // Particles' vertex buffer
    IDirect3DIndexBuffer9  *m_IB;            // Particles' index buffer

    D3DXVECTOR3             m_vecPosition;   // Position of emitter
    DWORD                   m_NumParticles;  // Max # particles in buffer
    cParticle              *m_Particles;     // Particle list

    static DWORD            m_RefCount;      // Class reference count

    static IDirect3DVertexShader9 *m_pShader;    // Vertex shader
    static IDirect3DVertexDeclaration9 *m_pDecl; // Vertex decl
    static IDirect3DTexture9 **m_pTextures;      // Textures

  public:
    cParticleEmitter();
    ~cParticleEmitter();

    BOOL Create(IDirect3DDevice9 *pDevice, 
                D3DXVECTOR3 *vecPosition,
                DWORD EmitterType,                              
                DWORD NumParticlesPerBuffer = 32);
    void Free();

    void Add(DWORD Type, D3DXVECTOR3 *vecPos, float Size, 
             DWORD Color, DWORD Life, 
             D3DXVECTOR3 *vecVelocity);
    void ClearAll();
    void Process(DWORD Elapsed);

    // Functions to prepare for particle rendering, wrap up rendering,
    // and to render a batch of particles.
    BOOL Begin(D3DXMATRIX *matView, D3DXMATRIX *matProj);
    void End();
    void Render();

    // Specialized functions for various particle types
    void HandleSmoke(D3DXVECTOR3 *vecPos, DWORD Elapsed);
    void HandleDucking(D3DXVECTOR3 *vecPos);
};

#endif
